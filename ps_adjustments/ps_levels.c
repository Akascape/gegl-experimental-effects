/*
Plugin Name: PS Levels
Author: Akascape
Description: Photoshop-style Levels adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_ps_levels_channel)
    enum_value (GEGL_PS_LEVELS_RGB,   "rgb",   N_("RGB"))
    enum_value (GEGL_PS_LEVELS_RED,   "red",   N_("Red"))
    enum_value (GEGL_PS_LEVELS_GREEN, "green", N_("Green"))
    enum_value (GEGL_PS_LEVELS_BLUE,  "blue",  N_("Blue"))
enum_end (GeglPsLevelsChannel)

property_enum (channel, "Channel", GeglPsLevelsChannel, gegl_ps_levels_channel,
               GEGL_PS_LEVELS_RGB)
    description ("Channel to adjust")

property_int (input_black, "Input Black", 0)
    description ("Input black point")
    value_range (0, 253)
    ui_range    (0, 253)

property_int (input_white, "Input White", 255)
    description ("Input white point")
    value_range (2, 255)
    ui_range    (2, 255)

property_double (gamma, "Gamma", 1.0)
    description ("Midtone gamma correction")
    value_range (0.10, 9.99)
    ui_range    (0.10, 9.99)

property_int (output_black, "Output Black", 0)
    description ("Output black point")
    value_range (0, 253)
    ui_range    (0, 253)

property_int (output_white, "Output White", 255)
    description ("Output white point")
    value_range (2, 255)
    ui_range    (2, 255)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_levels
#define GEGL_OP_C_FILE   "ps_levels.c"

#include <gegl-op.h>

/* ---------------- Levels Algorithm ---------------- */

static inline gfloat
apply_levels (gfloat x, 
              gfloat in_black, 
              gfloat in_white, 
              gfloat gamma, 
              gfloat out_black, 
              gfloat out_white)
{
    // Clamp input
    x = CLAMP (x, 0.0f, 1.0f);
    
    // Step 1: Map input range [in_black, in_white] to [0, 1]
    gfloat range = in_white - in_black;
    if (range < 0.001f)
        range = 0.001f; 
    
    x = (x - in_black) / range;
    x = CLAMP (x, 0.0f, 1.0f);
    
    // Step 2: Apply gamma correction
    if (fabsf (gamma - 1.0f) > 0.001f)
        x = powf (x, 1.0f / gamma);
    
    // Step 3: Map to output range [out_black, out_white]
    x = out_black + x * (out_white - out_black);
    
    return CLAMP (x, 0.0f, 1.0f);
}

/* ---------------- GEGL ---------------- */

static void
prepare (GeglOperation *operation)
{
    const Babl *space = gegl_operation_get_source_space (operation, "input");

    gegl_operation_set_format (operation, "input",
        babl_format_with_space ("RGBA float", space));
    gegl_operation_set_format (operation, "output",
        babl_format_with_space ("RGBA float", space));
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
    GeglProperties *o = GEGL_PROPERTIES (op);
    const Babl *format = babl_format ("RGBA float");

    // Convert from 0-255 range to 0.0-1.0 range
    const gfloat in_black  = (gfloat)o->input_black / 255.0f;
    const gfloat in_white  = (gfloat)o->input_white / 255.0f;
    const gfloat gamma_val = (gfloat)o->gamma;
    const gfloat out_black = (gfloat)o->output_black / 255.0f;
    const gfloat out_white = (gfloat)o->output_white / 255.0f;
    const GeglPsLevelsChannel channel = o->channel;

    gfloat *data = g_new (gfloat, (glong)roi->width * roi->height * 4);

    gegl_buffer_get (input, roi, 1.0, format, data,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    const glong pixels = (glong)roi->width * roi->height;


    for (glong i = 0; i < pixels; i++)
    {
        gfloat r = data[i*4+0];
        gfloat g = data[i*4+1];
        gfloat b = data[i*4+2];
        gfloat a = data[i*4+3];

        // Convert from linear to perceptual (gamma correction)
        r = powf (MAX (r, 0.0f), 1.0f / 2.2f);
        g = powf (MAX (g, 0.0f), 1.0f / 2.2f);
        b = powf (MAX (b, 0.0f), 1.0f / 2.2f);

        // Apply levels adjustment based on selected channel
        switch (channel)
        {
            case GEGL_PS_LEVELS_RGB: 
                // Apply to all channels
                r = apply_levels (r, in_black, in_white, gamma_val, out_black, out_white);
                g = apply_levels (g, in_black, in_white, gamma_val, out_black, out_white);
                b = apply_levels (b, in_black, in_white, gamma_val, out_black, out_white);
                break;
            
            case GEGL_PS_LEVELS_RED:
                // Apply only to red channel
                r = apply_levels (r, in_black, in_white, gamma_val, out_black, out_white);
                break;
            
            case GEGL_PS_LEVELS_GREEN:
                // Apply only to green channel
                g = apply_levels (g, in_black, in_white, gamma_val, out_black, out_white);
                break;
            
            case GEGL_PS_LEVELS_BLUE: 
                // Apply only to blue channel
                b = apply_levels (b, in_black, in_white, gamma_val, out_black, out_white);
                break;
        }

        // Convert back from perceptual to linear
        r = powf (r, 2.2f);
        g = powf (g, 2.2f);
        b = powf (b, 2.2f);

        data[i*4+0] = r;
        data[i*4+1] = g;
        data[i*4+2] = b;
        data[i*4+3] = a;
    }

    gegl_buffer_set (output, roi, 0, format, data, GEGL_AUTO_ROWSTRIDE);
    g_free (data);
    return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
    GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
    GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

    operation_class->prepare = prepare;
    filter_class->process    = process;

    gegl_operation_class_set_keys (operation_class,
        "name",        "akascape:ps-levels",
        "title",       "PS Levels",
        "categories",  "Color",
        "description", "Photoshop-style Levels adjustment with channel selection\nMade By Akascape",
        NULL);
}

#endif
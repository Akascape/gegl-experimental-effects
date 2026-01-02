/*
Plugin Name: PS Brightness / Contrast
Author: Akascape
Description: Photoshop-style Brightness and Contrast adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (brightness, "Brightness", 0.0)
    description ("Brightness amount")
    value_range (-150.0, 150.0)
    ui_range    (-150.0, 150.0)

property_double (contrast, "Contrast", 0.0)
    description ("Contrast amount")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_boolean (legacy, "Use Legacy", FALSE)
    description ("Use legacy algorithm (Linear Shift)")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_brightness_contrast
#define GEGL_OP_C_FILE   "ps_brightness_contrast.c"

#include <gegl-op.h>

#ifndef G_PI
#define G_PI 3.14159265358979323846
#endif

/* Contrast Adjustment */
static inline gfloat
sigmoid_contrast (gfloat x, gfloat k)
{
    x = CLAMP (x, 0.0f, 1.0f);
    if (fabsf (k - 1.0f) < 0.001f)
        return x;

    if (x < 0.5f)
        return 0.5f * powf (2.0f * x, k);
    else
        return 1.0f - 0.5f * powf (2.0f * (1.0f - x), k);
}

/* Brightness Adjustment */
static inline gfloat
ps_brightness (gfloat x, gfloat b)
{
    
    x = CLAMP (x, 0.0f, 1.0f);

    if (fabsf(b) < 0.001f)
        return x;

    if (b > 0.0f)
    {
        // For positive brightness, compress range and brighten highlights
        gfloat threshold = 1.0f - b;
        if (threshold < 0.001f)
            return 1.0f; 

        if (x < threshold)
            return x / threshold;
        else
            return 1.0f;
    }
    else
    {
        // For negative brightness, darken shadows and compress range
        gfloat threshold = 1.0f + b; 
        if (threshold < 0.001f)
            return 0.0f; 

        if (x < threshold)
            return x;
        else
            return threshold + (x - threshold) * threshold / (1.0f - threshold);
    }
}

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

    const gfloat brightness = (gfloat)o->brightness;
    const gfloat contrast   = (gfloat)o->contrast;
    const gboolean legacy   = o->legacy;

    gfloat *data = g_new (gfloat, (glong)roi->width * roi->height * 4);

    gegl_buffer_get (input, roi, 1.0, format, data,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    const glong pixels = (glong)roi->width * roi->height;


    // Legacy: linear brightness/contrast
    gfloat leg_slope = 1.0f;
    gfloat leg_shift = 0.0f;
    if (legacy)
    {
        leg_shift = brightness / 255.0f;
        gfloat c = contrast;
        if (c > 99.9f)  c = 99.9f;
        if (c < -99.9f) c = -99.9f;
        leg_slope = tanf ((c + 100.0f) * G_PI / 400.0f);
    }

    // Non-legacy: normalized brightness and contrast exponent
    const gfloat bval = brightness / 255.0f;
    gfloat sm_c_exp = 1.0f;
    if (!legacy)
    {
        if (contrast > 0.0f)
            sm_c_exp = 1.0f + (contrast / 80.0f);
        else
            sm_c_exp = 1.0f / (1.0f + fabsf (contrast) / 100.0f);
    }

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

        if (legacy)
        {
            // Step 1: Apply brightness shift
            r = r + leg_shift;
            g = g + leg_shift;
            b = b + leg_shift;
            
            // Step 2: Apply contrast around midpoint
            r = (r - 0.5f) * leg_slope + 0.5f;
            g = (g - 0.5f) * leg_slope + 0.5f;
            b = (b - 0.5f) * leg_slope + 0.5f;
        }
        else
        {
            // Apply Photoshop-style brightness
            r = ps_brightness (r, bval);
            g = ps_brightness (g, bval);
            b = ps_brightness (b, bval);

            // Apply sigmoid contrast
            r = sigmoid_contrast (r, sm_c_exp);
            g = sigmoid_contrast (g, sm_c_exp);
            b = sigmoid_contrast (b, sm_c_exp);
        }

        r = CLAMP (r, 0.0f, 1.0f);
        g = CLAMP (g, 0.0f, 1.0f);
        b = CLAMP (b, 0.0f, 1.0f);


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
        "name",        "akascape:ps-brightness-contrast",
        "title",       "PS Brightness / Contrast",
        "categories",  "Color",
        "description", "Photoshop-style Brightness and Contrast adjustment\nMade By Akascape",
        NULL);
}

#endif
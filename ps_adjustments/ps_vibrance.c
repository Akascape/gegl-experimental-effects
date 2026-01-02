/*
Plugin Name: PS Vibrance
Author: Akascape
Description: Photoshop-style Vibrance adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (vibrance, "Vibrance", 0.0)
    description ("Vibrance adjustment (smart saturation)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (saturation, "Saturation", 0.0)
    description ("Saturation adjustment")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_vibrance
#define GEGL_OP_C_FILE   "ps_vibrance.c"

#include <gegl-op.h>

static inline float
get_luma (float r, float g, float b)
{
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

static inline float
get_max_component (float r, float g, float b)
{
    return fmaxf(r, fmaxf(g, b));
}

static inline float
get_min_component (float r, float g, float b)
{
    return fminf(r, fminf(g, b));
}

/* ---------------- GEGL ---------------- */

static void
prepare (GeglOperation *operation)
{
    const Babl *space = gegl_operation_get_source_space (operation, "input");
    const Babl *format = babl_format_with_space ("RGBA float", space);

    gegl_operation_set_format (operation, "input", format);
    gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
    GeglProperties *o = GEGL_PROPERTIES (op);
    const Babl *format = gegl_operation_get_format (op, "input");

    const float vibrance_val = (float)o->vibrance;
    const float saturation_val = (float)o->saturation;

    // 1. Calculate the Saturation Multiplier
    const float sat_mult = 1.0f + (saturation_val / 100.0f);

    // 2. Calculate Vibrance Coefficient 
    const float vib_coeff = vibrance_val / 100.0f;

    gfloat *data = g_new (gfloat, (glong)roi->width * roi->height * 4);

    gegl_buffer_get (input, roi, 1.0, format, data,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    const glong pixels = (glong)roi->width * roi->height;


    for (glong i = 0; i < pixels; i++)
    {
        float r = data[i*4+0];
        float g = data[i*4+1];
        float b = data[i*4+2];
        float a = data[i*4+3];

        float luma = get_luma(r, g, b);
        float mx = get_max_component(r, g, b);
        float mn = get_min_component(r, g, b);
        
        // Current saturation (chroma)
        float sat = mx - mn;

        // Calculate Vibrance scale factor
        float vib_factor = 1.0f;

        if (vib_coeff >= 0.0f) {
            // Positive Vibrance
            vib_factor = 1.0f + (vib_coeff * (1.0f - sat));
        } else {
            // Negative Vibrance
            vib_factor = 1.0f + (vib_coeff * 0.5f);
        }

        float final_scale = vib_factor * sat_mult;

        r = luma + (r - luma) * final_scale;
        g = luma + (g - luma) * final_scale;
        b = luma + (b - luma) * final_scale;

        data[i*4+0] = CLAMP (r, 0.0f, 1.0f);
        data[i*4+1] = CLAMP (g, 0.0f, 1.0f);
        data[i*4+2] = CLAMP (b, 0.0f, 1.0f);
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
        "name",        "akascape:ps-vibrance",
        "title",       "PS Vibrance",
        "categories",  "Color",
        "description", "Photoshop-style Vibrance adjustment\nMade By Akascape",
        NULL);
}

#endif
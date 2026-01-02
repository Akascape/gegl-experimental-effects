/*
Plugin Name: PS Exposure
Author: Akascape
Description: Photoshop-style Exposure adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>

#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (exposure, "Exposure", 0.0)
    description ("Exposure adjustment in stops")
    value_range (-20.0, 20.0)
    ui_range    (-20.0, 20.0)

property_double (offset, "Offset", 0.0)
    description ("Offset adjustment")
    value_range (-0.5, 0.5)
    ui_range    (-0.5, 0.5)

property_double (gamma_correction, "Gamma Correction", 1.0)
    description ("Gamma correction")
    value_range (0.01, 9.99)
    ui_range    (0.01, 9.99)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_exposure
#define GEGL_OP_C_FILE   "ps_exposure.c"

#include <gegl-op.h>

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

    const gfloat exposure = (gfloat)o->exposure;
    const gfloat offset = (gfloat)o->offset;
    const gfloat gamma = (gfloat)o->gamma_correction;

    gfloat *data = g_new (gfloat, (glong)roi->width * roi->height * 4);

    gegl_buffer_get (input, roi, 1.0, format, data,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    const glong pixels = (glong)roi->width * roi->height;

    // Pre-calculate exposure multiplier
    const gfloat exposure_mult = powf (2.0f, exposure);

    for (glong i = 0; i < pixels; i++)
    {
        gfloat r = data[i*4+0];
        gfloat g = data[i*4+1];
        gfloat b = data[i*4+2];
        gfloat a = data[i*4+3];

        // Step 1: Apply exposure in LINEAR space
        r = r * exposure_mult;
        g = g * exposure_mult;
        b = b * exposure_mult;

        // Step 2: Apply offset in LINEAR space
        r = r + offset;
        g = g + offset;
        b = b + offset;

        // Clamp after offset
        r = CLAMP (r, 0.0f, 1.0f);
        g = CLAMP (g, 0.0f, 1.0f);
        b = CLAMP (b, 0.0f, 1.0f);

        // Convert from linear to perceptual 
        r = powf (r, 1.0f / 2.2f);
        g = powf (g, 1.0f / 2.2f);
        b = powf (b, 1.0f / 2.2f);

        // Step 3: Apply gamma correction in PERCEPTUAL space
        if (fabsf (gamma - 1.0f) > 0.001f)
        {
            r = powf (r, 1.0f / gamma);
            g = powf (g, 1.0f / gamma);
            b = powf (b, 1.0f / gamma);
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
        "name",        "akascape:ps-exposure",
        "title",       "PS Exposure",
        "categories",  "Color",
        "description", "Photoshop-style Exposure adjustment\nMade By Akascape",
        NULL);
}

#endif
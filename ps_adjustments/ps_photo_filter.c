/*
Plugin Name: PS Photo Filter
Author: Akascape
Description: Photoshop-style Photo Filter effect
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_color (filter_color, "Filter Color", "#ec8a00")
    description ("The color to tint the image with.")

property_double (density, "Density", 0.8)
    description ("The strength of the photo filter effect.")
    value_range (0.0, 1.0)
    ui_range    (0.0, 1.0)
    ui_meta     ("unit", "percent")

property_boolean (preserve_luminosity, "Preserve Luminosity", TRUE)
    description ("If checked, the effect tints the color without changing the overall brightness.")

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     ps_photo_filter
#define GEGL_OP_C_FILE   "ps_photo_filter.c"

#include <gegl-op.h>

static inline float clampf (float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static inline float get_luminance (float r, float g, float b)
{
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

static void clip_color (float *r, float *g, float *b, float lum)
{
    float min_val = (*r < *g) ? ((*r < *b) ? *r : *b) : ((*g < *b) ? *g : *b);
    float max_val = (*r > *g) ? ((*r > *b) ? *r : *b) : ((*g > *b) ? *g : *b);

    if (min_val < 0.0f)
    {
        float scale = lum / (lum - min_val);
        *r = lum + (*r - lum) * scale;
        *g = lum + (*g - lum) * scale;
        *b = lum + (*b - lum) * scale;
    }
    if (max_val > 1.0f)
    {
        float scale = (1.0f - lum) / (max_val - lum);
        *r = lum + (*r - lum) * scale;
        *g = lum + (*g - lum) * scale;
        *b = lum + (*b - lum) * scale;
    }
    
    *r = clampf(*r, 0.0f, 1.0f);
    *g = clampf(*g, 0.0f, 1.0f);
    *b = clampf(*b, 0.0f, 1.0f);
}

/* ---------------- GEGL Operation ---------------- */

static void
prepare (GeglOperation *operation)
{
    const Babl *space = gegl_operation_get_source_space (operation, "input");
    
    const Babl *format = babl_format_with_space ("R'G'B'A float", space);

    gegl_operation_set_format (operation, "input", format);
    gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
    GeglProperties *o = GEGL_PROPERTIES (op);
    gfloat *in  = in_buf;
    gfloat *out = out_buf;

    gfloat filter_rgb[4];
    gfloat density = (gfloat)o->density;
    gboolean preserve_lum = o->preserve_luminosity;

    gegl_color_get_pixel (o->filter_color, 
                          gegl_operation_get_format (op, "input"), 
                          filter_rgb);

    gfloat inv_density = 1.0f - density;

    for (glong i = 0; i < n_pixels; i++)
    {
        float r_in = in[0];
        float g_in = in[1];
        float b_in = in[2];
        float a_in = in[3];

        /* Step 1: Apply Tint */
        
        float r_multiply = r_in * filter_rgb[0];
        float g_multiply = g_in * filter_rgb[1];
        float b_multiply = b_in * filter_rgb[2];

        float r_tinted = r_in * inv_density + r_multiply * density;
        float g_tinted = g_in * inv_density + g_multiply * density;
        float b_tinted = b_in * inv_density + b_multiply * density;

        float r_final, g_final, b_final;

        if (preserve_lum)
        {
            /* Step 2: Luma Lock */
            
            float lum_original = get_luminance (r_in, g_in, b_in);
            float lum_tinted   = get_luminance (r_tinted, g_tinted, b_tinted);
            
            float lum_diff = lum_original - lum_tinted;
            
            r_final = r_tinted + lum_diff;
            g_final = g_tinted + lum_diff;
            b_final = b_tinted + lum_diff;

            /* Check for clipping */
            clip_color (&r_final, &g_final, &b_final, lum_original);
        }
        else
        {
            r_final = r_tinted;
            g_final = g_tinted;
            b_final = b_tinted;
        }

        out[0] = clampf (r_final, 0.0f, 1.0f);
        out[1] = clampf (g_final, 0.0f, 1.0f);
        out[2] = clampf (b_final, 0.0f, 1.0f);
        out[3] = a_in;

        in  += 4;
        out += 4;
    }

    return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
    GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
    GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

    operation_class->prepare = prepare;
    point_filter_class->process = process;

    gegl_operation_class_set_keys (operation_class,
        "name",        "akascape:ps-photo-filter",
        "title",       "PS Photo Filter",
        "categories",  "Color",
        "description", "Photoshop-style Photo Filter effect\nMade By Akascape",
        NULL);
}

#endif
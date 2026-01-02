/*
Plugin Name: PS Black & White
Author: Akascape
Description: Photoshop-style Black & White adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

property_double (reds, "Reds", 40.0)
    description ("Red channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_double (yellows, "Yellows", 60.0)
    description ("Yellow channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_double (greens, "Greens", 40.0)
    description ("Green channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_double (cyans, "Cyans", 60.0)
    description ("Cyan channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_double (blues, "Blues", 20.0)
    description ("Blue channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_double (magentas, "Magentas", 80.0)
    description ("Magenta channel contribution")
    value_range (-200.0, 300.0)
    ui_range    (-200.0, 300.0)
    ui_meta     ("unit", "%")

property_boolean (tint, "Tint", FALSE)
    description ("Apply a color tint to the image")

property_color (tint_color, "Tint Color", "#e1d2c2")
    description ("Color used for tinting")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_black_and_white
#define GEGL_OP_C_FILE   "ps_black_and_white.c"

#include <gegl-op.h>

/* Calculate max of 3 floats */
static inline gfloat max3 (gfloat a, gfloat b, gfloat c) {
    return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
}

/* Calculate min of 3 floats */
static inline gfloat min3 (gfloat a, gfloat b, gfloat c) {
    return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

/* RGB to HSL conversion */
static void rgb_to_hsl(gfloat r, gfloat g, gfloat b, gfloat *h, gfloat *s, gfloat *l)
{
    gfloat max = max3(r, g, b);
    gfloat min = min3(r, g, b);
    gfloat d = max - min;

    *l = (max + min) / 2.0f;

    if (d < 1e-5f) {
        *h = 0.0f;
        *s = 0.0f;
    } else {
        *s = (*l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

        if (max == r) {
            *h = (g - b) / d + (g < b ? 6.0f : 0.0f);
        } else if (max == g) {
            *h = (b - r) / d + 2.0f;
        } else {
            *h = (r - g) / d + 4.0f;
        }
        *h /= 6.0f;
    }
}

/* HSL to RGB conversion */
static gfloat hue_to_rgb(gfloat p, gfloat q, gfloat t)
{
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

static void hsl_to_rgb(gfloat h, gfloat s, gfloat l, gfloat *r, gfloat *g, gfloat *b)
{
    if (s < 1e-5f) {
        *r = *g = *b = l;
    } else {
        gfloat q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
        gfloat p = 2.0f * l - q;
        *r = hue_to_rgb(p, q, h + 1.0f/3.0f);
        *g = hue_to_rgb(p, q, h);
        *b = hue_to_rgb(p, q, h - 1.0f/3.0f);
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

    /* Normalize percentages to 0.0-1.0 range */
    const gfloat w_r = (gfloat)o->reds / 100.0f;
    const gfloat w_y = (gfloat)o->yellows / 100.0f;
    const gfloat w_g = (gfloat)o->greens / 100.0f;
    const gfloat w_c = (gfloat)o->cyans / 100.0f;
    const gfloat w_b = (gfloat)o->blues / 100.0f;
    const gfloat w_m = (gfloat)o->magentas / 100.0f;

    const gboolean do_tint = o->tint;
    gfloat tint_h = 0.0f, tint_s = 0.0f, tint_l_dummy = 0.0f;

    if (do_tint) {
        gfloat t_rgba[4];
        gegl_color_get_pixel(o->tint_color, babl_format("RGBA float"), t_rgba);
        rgb_to_hsl(t_rgba[0], t_rgba[1], t_rgba[2], &tint_h, &tint_s, &tint_l_dummy);
    }

    const Babl *format = babl_format ("RGBA float");
    const GeglRectangle *extent = gegl_buffer_get_extent (input);

    const gint W = extent->width;
    const gint H = extent->height;

    gfloat *src = g_new (gfloat, (glong)W * H * 4);
    gegl_buffer_get (input, extent, 1.0, format, src,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    for (gint y = roi->y; y < roi->y + roi->height; y++) {
        for (gint x = roi->x; x < roi->x + roi->width; x++) {
            gint ix = x - extent->x;
            gint iy = y - extent->y;
            glong idx = (glong)(iy * W + ix) * 4;

            gfloat in_r = src[idx + 0];
            gfloat in_g = src[idx + 1];
            gfloat in_b = src[idx + 2];
            gfloat in_a = src[idx + 3];

            /* Calculate differences for intermediate color ranges */
            gfloat d_r = in_r - in_g;
            gfloat d_g = in_g - in_r;

            gfloat gray = 0.0f;
            gfloat mn = min3(in_r, in_g, in_b);
            gfloat mx = max3(in_r, in_g, in_b);
            gfloat mid = in_r + in_g + in_b - mx - mn; /* The middle component value */

            /* Determine the hue segment (sextant) */
            if (in_r == mx) {
                if (in_g >= in_b) { 
                    /* Red -> Yellow sector */
                    /* Ratio moves from R (1.0) to Y. Secondary is G. */
      
                    gray = (mx - mid) * w_r + (mid - mn) * w_y;
                } else {
                    /* Magenta -> Red sector */

                    gray = (mx - mid) * w_r + (mid - mn) * w_m;
                }
            } else if (in_g == mx) {
                if (in_r >= in_b) {
                    /* Yellow -> Green sector */
                    gray = (mx - mid) * w_g + (mid - mn) * w_y;
                } else {
                    /* Green -> Cyan sector */
                    gray = (mx - mid) * w_g + (mid - mn) * w_c;
                }
            } else { /* in_b == mx */
                if (in_g >= in_r) {
                    /* Cyan -> Blue sector */
                    gray = (mx - mid) * w_b + (mid - mn) * w_c;
                } else {
                    /* Blue -> Magenta sector */
                    gray = (mx - mid) * w_b + (mid - mn) * w_m;
                }
            }

            /* 
               The formula derived for B&W Adjustment:
               Gray = (Max - Mid) * Weight_Primary + (Mid - Min) * Weight_Secondary + Min * Weight_Neutral?
            */
            gray += mn; 

            gray = CLAMP(gray, 0.0f, 1.0f);
            
            gfloat out_r = gray;
            gfloat out_g = gray;
            gfloat out_b = gray;

            if (do_tint) {
                /* HSL approach: L = gray, H = tint_h, S = tint_s */
                hsl_to_rgb(tint_h, tint_s, gray, &out_r, &out_g, &out_b);
            }

            gfloat out_px[4] = { out_r, out_g, out_b, in_a };
            gegl_buffer_set (output, GEGL_RECTANGLE (x, y, 1, 1), 0, format, out_px, GEGL_AUTO_ROWSTRIDE);
        }
    }

    g_free (src);
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
        "name",        "akascape:ps-black-and-white",
        "title",       "PS Black & White",
        "categories",  "Color",
        "description", "Photoshop-style Black & White adjustment with Tint\nMade By Akascape",
        NULL);
}

#endif
/*
Plugin Name: PS Selective Color
Author: Akascape
Description: Photoshop-style Selective Color adjustment.
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (selective_color_enum)
  enum_value (SELECTIVE_COLOR_RED,     "reds",     N_("Reds"))
  enum_value (SELECTIVE_COLOR_YELLOW,  "yellows",  N_("Yellows"))
  enum_value (SELECTIVE_COLOR_GREEN,   "greens",   N_("Greens"))
  enum_value (SELECTIVE_COLOR_CYAN,    "cyans",    N_("Cyans"))
  enum_value (SELECTIVE_COLOR_BLUE,    "blues",    N_("Blues"))
  enum_value (SELECTIVE_COLOR_MAGENTA, "magentas", N_("Magentas"))
  enum_value (SELECTIVE_COLOR_WHITE,   "whites",   N_("Whites"))
  enum_value (SELECTIVE_COLOR_NEUTRAL, "neutrals", N_("Neutrals"))
  enum_value (SELECTIVE_COLOR_BLACK,   "blacks",   N_("Blacks"))
enum_end (SelectiveColorEnum)

property_enum (color_range, "Colors",
               SelectiveColorEnum, selective_color_enum,
               SELECTIVE_COLOR_RED)
    description ("Select the color range to adjust")

property_double (cyan_adj, "Cyan", 0.0)
    description ("Adjust Cyan component")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)
    ui_meta     ("unit", "%")

property_double (magenta_adj, "Magenta", 0.0)
    description ("Adjust Magenta component")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)
    ui_meta     ("unit", "%")

property_double (yellow_adj, "Yellow", 0.0)
    description ("Adjust Yellow component")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)
    ui_meta     ("unit", "%")

property_double (black_adj, "Black", 0.0)
    description ("Adjust Black component (modifies CMY together)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)
    ui_meta     ("unit", "%")

property_boolean (absolute, "Absolute", FALSE)
    description ("Use Absolute calculation method (Relative is default)")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_selective_color
#define GEGL_OP_C_FILE   "ps_selective_color.c"

#include <gegl-op.h>

/* Calculate max of 3 floats */
static inline gfloat max3 (gfloat a, gfloat b, gfloat c) {
    return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
}

/* Calculate min of 3 floats */
static inline gfloat min3 (gfloat a, gfloat b, gfloat c) {
    return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

/* Calculate max of 2 floats */
static inline gfloat max2 (gfloat a, gfloat b) {
    return (a > b) ? a : b;
}

/* Calculate min of 2 floats */
static inline gfloat min2 (gfloat a, gfloat b) {
    return (a < b) ? a : b;
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

    const gint range = o->color_range;
    const gboolean absolute = o->absolute;
    
    /* Convert percentage sliders (-100 to 100) to factor (-1.0 to 1.0) */
    const gfloat adj_c = (gfloat)o->cyan_adj / 100.0f;
    const gfloat adj_m = (gfloat)o->magenta_adj / 100.0f;
    const gfloat adj_y = (gfloat)o->yellow_adj / 100.0f;
    const gfloat adj_k = (gfloat)o->black_adj / 100.0f;

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

            gfloat r = src[idx + 0];
            gfloat g = src[idx + 1];
            gfloat b = src[idx + 2];
            gfloat a = src[idx + 3];

            /* 1. Calculate the 'Selector' factor (0.0 to 1.0) based on Range */
            gfloat factor = 0.0f;
            
            gfloat mx = max3(r, g, b);
            gfloat mn = min3(r, g, b);
            
            switch (range) {
                case SELECTIVE_COLOR_RED:
                    if (mx == r) {
                        factor = r - max2(g, b);
                    }
                    break;
                case SELECTIVE_COLOR_YELLOW:
                    if (mn == b) {
                        factor = min2(r, g) - b;
                    }
                    break;
                case SELECTIVE_COLOR_GREEN:
                    if (mx == g) {
                        factor = g - max2(r, b);
                    }
                    break;
                case SELECTIVE_COLOR_CYAN:
                    if (mn == r) {
                        factor = min2(g, b) - r;
                    }
                    break;
                case SELECTIVE_COLOR_BLUE:
                    if (mx == b) {
                        factor = b - max2(r, g);
                    }
                    break;
                case SELECTIVE_COLOR_MAGENTA:
                    if (mn == g) {
                        factor = min2(r, b) - g;
                    }
                    break;
                case SELECTIVE_COLOR_WHITE:
                    factor = min3(r, g, b);
                    break;
                case SELECTIVE_COLOR_NEUTRAL:
                    factor = 1.0f - (mx - mn);
                    break;
                case SELECTIVE_COLOR_BLACK:
                    factor = 1.0f - mx;
                    break;
            }

            /* 2. Apply Adjustments if the pixel is within range */
            if (factor > 0.0f) {
                /* Conceptually convert to CMY */
                gfloat c = 1.0f - r;
                gfloat m = 1.0f - g;
                gfloat y_val = 1.0f - b;

                /* Calculate Delta for each channel */
                gfloat d_c = adj_c + adj_k;
                gfloat d_m = adj_m + adj_k;
                gfloat d_y = adj_y + adj_k;

                /* Scale deltas by the selection factor */
                gfloat scale_c = d_c * factor;
                gfloat scale_m = d_m * factor;
                gfloat scale_y = d_y * factor;

                if (absolute) {
                    /* Absolute: Simply add the scaled value */
                    c += scale_c;
                    m += scale_m;
                    y_val += scale_y;
                } else {
                    /* Relative: Add percentage of current value */
                    c += c * scale_c;
                    m += m * scale_m;
                    y_val += y_val * scale_y;
                }

                /* Convert back to RGB */
                r = 1.0f - c;
                g = 1.0f - m;
                b = 1.0f - y_val;
            }

            gfloat out_px[4] = {
                CLAMP(r, 0.0f, 1.0f),
                CLAMP(g, 0.0f, 1.0f),
                CLAMP(b, 0.0f, 1.0f),
                a
            };

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
        "name",        "akascape:ps-selective-color",
        "title",       "PS Selective Color",
        "categories",  "Color",
        "description", "Photoshop-style Selective Color Adjustment\nMade By Akascape",
        NULL);
}

#endif
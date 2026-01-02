/*
Plugin Name: PS Color Balance
Author: Akascape
Description: Photoshop-style Color Balance adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (ps_color_balance_range_enum)
  enum_value (RANGE_SHADOWS,   "shadows",   N_("Shadows"))
  enum_value (RANGE_MIDTONES,  "midtones",  N_("Midtones"))
  enum_value (RANGE_HIGHLIGHTS,"highlights",N_("Highlights"))
enum_end (PsColorBalanceRangeEnum)

property_enum (range, "Range",
               PsColorBalanceRangeEnum, ps_color_balance_range_enum,
               RANGE_MIDTONES)
    description ("Select the tonal range to adjust")

property_double (cyan_red, "Cyan - Red", 0.0)
    description ("Adjust Cyan (negative) or Red (positive)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (magenta_green, "Magenta - Green", 0.0)
    description ("Adjust Magenta (negative) or Green (positive)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (yellow_blue, "Yellow - Blue", 0.0)
    description ("Adjust Yellow (negative) or Blue (positive)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_boolean (preserve_luminosity, "Preserve Luminosity", TRUE)
    description ("Maintain the image brightness while changing colors")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_color_balance
#define GEGL_OP_C_FILE   "ps_color_balance.c"

#include <gegl-op.h>

/* Calculate Luminance  */
static inline gfloat get_luma (gfloat r, gfloat g, gfloat b) {
    return r * 0.299f + g * 0.587f + b * 0.114f;
}


/* These curves approximate the Photoshop transfer functions */
static inline gfloat get_weight (gfloat luma, gint range) {
    /* Clamp luma for safety */
    gfloat l = CLAMP(luma, 0.0f, 1.0f);
    
    switch (range) {
        case RANGE_SHADOWS:
            /* Shadows: Strong weight in dark areas, tapering off toward midtones. */
            return 4.0f * l * (1.0f - l);
            
        case RANGE_MIDTONES:
            /* Midtones: Inverse parabola favoring middle luminance values. */
            if (l < 0.5f)
                return 2.0f * l;  
            else
                return 2.0f * (1.0f - l);  
            
        case RANGE_HIGHLIGHTS:
            /* Highlights: Strong weight in bright areas. */
            return l * l * (3.0f - 2.0f * l); 
            
        default: 
            return 0.0f;
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

    const gint range = o->range;
    const gboolean preserve_lum = o->preserve_luminosity;

    const gfloat scale = 0.0025f; 
    const gfloat dr = (gfloat)o->cyan_red * scale;
    const gfloat dg = (gfloat)o->magenta_green * scale;
    const gfloat db = (gfloat)o->yellow_blue * scale;

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

            /* 1. Calculate Original Luminance */
            gfloat luma = get_luma(r, g, b);

            /* 2. Calculate Weight based on Tonal Range */
            gfloat weight = get_weight(luma, range);

            /* 3. Apply Additive Color Shift */
            gfloat r_new = r + dr * weight;
            gfloat g_new = g + dg * weight;
            gfloat b_new = b + db * weight;

            /* 4. Preserve Luminosity (Optional) */
            if (preserve_lum) {
                gfloat luma_new = get_luma(r_new, g_new, b_new);
                
                gfloat luma_diff = luma - luma_new;
                
                r_new += luma_diff;
                g_new += luma_diff;
                b_new += luma_diff;
            }

            /* Clamp to valid range */
            gfloat out_px[4] = {
                CLAMP(r_new, 0.0f, 1.0f),
                CLAMP(g_new, 0.0f, 1.0f),
                CLAMP(b_new, 0.0f, 1.0f),
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
        "name",        "akascape:ps-color-balance",
        "title",       "PS Color Balance",
        "categories",  "Color",
        "description", "Photoshop-style Color Balance adjustment\nMade By Akascape",
        NULL);
}

#endif
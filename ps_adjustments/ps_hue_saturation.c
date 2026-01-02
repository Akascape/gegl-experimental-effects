/*
Plugin Name: PS Hue/Saturation
Author: Akascape
Description: Photoshop-style Hue/Saturation adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/


#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* Master controls */
property_double (master_hue, "Master Hue", 0.0)
    description ("Master hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (master_saturation, "Master Saturation", 0.0)
    description ("Master saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (master_lightness, "Master Lightness", 0.0)
    description ("Master lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

/* Per‑range controls: Reds / Yellows / Greens / Cyans / Blues / Magentas */
property_double (reds_hue, "Reds Hue", 0.0)
    description ("Reds hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (reds_saturation, "Reds Saturation", 0.0)
    description ("Reds saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (reds_lightness, "Reds Lightness", 0.0)
    description ("Reds lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (yellows_hue, "Yellows Hue", 0.0)
    description ("Yellows hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (yellows_saturation, "Yellows Saturation", 0.0)
    description ("Yellows saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (yellows_lightness, "Yellows Lightness", 0.0)
    description ("Yellows lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (greens_hue, "Greens Hue", 0.0)
    description ("Greens hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (greens_saturation, "Greens Saturation", 0.0)
    description ("Greens saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (greens_lightness, "Greens Lightness", 0.0)
    description ("Greens lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (cyans_hue, "Cyans Hue", 0.0)
    description ("Cyans hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (cyans_saturation, "Cyans Saturation", 0.0)
    description ("Cyans saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (cyans_lightness, "Cyans Lightness", 0.0)
    description ("Cyans lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (blues_hue, "Blues Hue", 0.0)
    description ("Blues hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (blues_saturation, "Blues Saturation", 0.0)
    description ("Blues saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (blues_lightness, "Blues Lightness", 0.0)
    description ("Blues lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (magentas_hue, "Magentas Hue", 0.0)
    description ("Magentas hue adjustment (degrees)")
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)

property_double (magentas_saturation, "Magentas Saturation", 0.0)
    description ("Magentas saturation adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

property_double (magentas_lightness, "Magentas Lightness", 0.0)
    description ("Magentas lightness adjustment (%)")
    value_range (-100.0, 100.0)
    ui_range    (-100.0, 100.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_hue_saturation
#define GEGL_OP_C_FILE   "ps_hue_saturation.c"

#include <gegl-op.h>

/* ---------------- Helper Functions ---------------- */

static inline float clampf (float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static inline float wrap_deg_360 (float h)
{
    float r = fmodf (h, 360.0f);
    return (r < 0.0f) ? r + 360.0f : r;
}

static inline float smoothstep (float t) { return t * t * (3.0f - 2.0f * t); }

/* Determines how much the current pixel is affected by a color range */
static inline float hue_range_weight (float hue_deg, float center_deg, float inner, float outer)
{
    float d = fabsf (hue_deg - center_deg);
    d = fminf (d, 360.0f - d); 
    if (d <= inner) return 1.0f;
    if (d >= outer) return 0.0f;
    float t = (d - inner) / (outer - inner);
    return 1.0f - smoothstep (t);
}

/* sRGB transfer functions  */
static inline float linear_to_srgb (float x)
{
    return (x <= 0.0031308f) ? 12.92f * x : 1.055f * powf (x, 1.0f/2.4f) - 0.055f;
}
static inline float srgb_to_linear (float x)
{
    return (x <= 0.04045f) ? x / 12.92f : powf ((x + 0.055f)/1.055f, 2.4f);
}

/* RGB (Gamma encoded) to HSL */
static inline void rgb_to_hsl (float r, float g, float b, float *h, float *s, float *l)
{
    float mx = fmaxf (r, fmaxf (g, b));
    float mn = fminf (r, fminf (g, b));
    float c  = mx - mn;

    float light = 0.5f * (mx + mn);
    float sat   = 0.0f;
    float hue   = 0.0f;

    if (c > 1e-6f) {
        if (mx == r)      hue = fmodf ((g - b) / c, 6.0f);
        else if (mx == g) hue = ((b - r) / c) + 2.0f;
        else              hue = ((r - g) / c) + 4.0f;
        hue *= 60.0f;
        if (hue < 0.0f) hue += 360.0f;
        
        sat = c / (1.0f - fabsf (2.0f * light - 1.0f));
    }

    *h = hue;
    *s = sat;
    *l = light;
}

/* HSL to RGB (Gamma encoded) */
static inline void hsl_to_rgb (float h, float s, float l, float *r, float *g, float *b)
{
    float c = (1.0f - fabsf (2.0f * l - 1.0f)) * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - fabsf (fmodf (hp, 2.0f) - 1.0f));

    float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
    if (0.0f <= hp && hp < 1.0f)      { r1 = c; g1 = x; }
    else if (1.0f <= hp && hp < 2.0f) { r1 = x; g1 = c; }
    else if (2.0f <= hp && hp < 3.0f) { g1 = c; b1 = x; }
    else if (3.0f <= hp && hp < 4.0f) { g1 = x; b1 = c; }
    else if (4.0f <= hp && hp < 5.0f) { r1 = x; b1 = c; }
    else if (5.0f <= hp && hp < 6.0f) { r1 = c; b1 = x; }

    float m = l - 0.5f * c;
    *r = r1 + m;
    *g = g1 + m;
    *b = b1 + m;
}

/* ---------------- GEGL Operation ---------------- */

static void
prepare (GeglOperation *operation)
{
    const Babl *space = gegl_operation_get_source_space (operation, "input");
    const Babl *format = babl_format_with_space ("RGBA float", space);

    gegl_operation_set_format (operation, "input", format);
    gegl_operation_set_format (operation, "output", format);
}

typedef struct {
    float hue_shift;   
    float sat_scale;   
    float light_shift; 
    float center;      
} HueRangeParams;

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
    GeglProperties *o = GEGL_PROPERTIES (op);
    const Babl *format = gegl_operation_get_format (op, "input");

    /* Master adjustments */
    const float master_hue  = (float)o->master_hue;
    const float master_sat  = (float)o->master_saturation;
    const float master_lit  = (float)o->master_lightness;

    /* Per-range parameters (Center of colors) */
    HueRangeParams ranges[7] = {
        { (float)o->reds_hue,      1.0f + (float)o->reds_saturation     / 100.0f, (float)o->reds_lightness     / 100.0f,   0.0f   },
        { (float)o->yellows_hue,   1.0f + (float)o->yellows_saturation  / 100.0f, (float)o->yellows_lightness  / 100.0f,  60.0f   },
        { (float)o->greens_hue,    1.0f + (float)o->greens_saturation   / 100.0f, (float)o->greens_lightness   / 100.0f, 120.0f   },
        { (float)o->cyans_hue,     1.0f + (float)o->cyans_saturation    / 100.0f, (float)o->cyans_lightness    / 100.0f, 180.0f   },
        { (float)o->blues_hue,     1.0f + (float)o->blues_saturation    / 100.0f, (float)o->blues_lightness    / 100.0f, 240.0f   },
        { (float)o->magentas_hue,  1.0f + (float)o->magentas_saturation / 100.0f, (float)o->magentas_lightness / 100.0f, 300.0f   },
        { (float)o->reds_hue,      1.0f + (float)o->reds_saturation     / 100.0f, (float)o->reds_lightness     / 100.0f, 360.0f   }
    };

    /* Overlap ranges: Full strength within ±30°, falloff to zero at ±60° */
    const float inner_half = 30.0f;
    const float outer_half = 60.0f;

    gfloat *data = g_new (gfloat, (glong)roi->width * roi->height * 4);
    gegl_buffer_get (input, roi, 1.0, format, data,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

    const glong pixels = (glong)roi->width * roi->height;

    for (glong i = 0; i < pixels; i++)
    {
        float r_lin = data[i*4+0];
        float g_lin = data[i*4+1];
        float b_lin = data[i*4+2];
        float a     = data[i*4+3];

        /* 1. Convert Linear RGB -> Gamma Corrected sRGB  */
        float r = clampf (linear_to_srgb (clampf (r_lin, 0.0f, 1.0f)), 0.0f, 1.0f);
        float g = clampf (linear_to_srgb (clampf (g_lin, 0.0f, 1.0f)), 0.0f, 1.0f);
        float b = clampf (linear_to_srgb (clampf (b_lin, 0.0f, 1.0f)), 0.0f, 1.0f);

        /* 2. Convert to HSL (Better for saturation than HSV) */
        float h_hsl, s_hsl, l_hsl;
        rgb_to_hsl (r, g, b, &h_hsl, &s_hsl, &l_hsl);

        /* 3. Calculate dynamic adjustments based on Hue */
        float hue_shift   = master_hue;
        float sat_scale   = 1.0f + master_sat / 100.0f;
        float light_shift = master_lit / 100.0f;

        for (int k = 0; k < 7; k++) {
            float w = hue_range_weight (h_hsl, ranges[k].center, inner_half, outer_half);
            if (w > 0.0f) {
                hue_shift   += w * ranges[k].hue_shift;
                /* Accumulate saturation scalers */
                sat_scale   *= 1.0f + w * ((ranges[k].sat_scale - 1.0f));
                /* Accumulate lightness shift */
                light_shift += w * ranges[k].light_shift;
            }
        }

        /* 4. Apply Adjustments */
        
        /* Hue: Simple rotation */
        h_hsl = wrap_deg_360 (h_hsl + hue_shift);

        /* Saturation: Scale (clamped) */
        s_hsl = clampf (s_hsl * sat_scale, 0.0f, 1.0f);

        if (light_shift > 0.0f) {
            l_hsl = l_hsl + (1.0f - l_hsl) * light_shift;
        } else {
            l_hsl = l_hsl * (1.0f + light_shift);
        }
        l_hsl = clampf (l_hsl, 0.0f, 1.0f);

        /* 5. Convert back to RGB */
        hsl_to_rgb (h_hsl, s_hsl, l_hsl, &r, &g, &b);

        r_lin = srgb_to_linear (clampf (r, 0.0f, 1.0f));
        g_lin = srgb_to_linear (clampf (g, 0.0f, 1.0f));
        b_lin = srgb_to_linear (clampf (b, 0.0f, 1.0f));

        data[i*4+0] = clampf (r_lin, 0.0f, 1.0f);
        data[i*4+1] = clampf (g_lin, 0.0f, 1.0f);
        data[i*4+2] = clampf (b_lin, 0.0f, 1.0f);
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
        "name",        "akascape:ps-hue-saturation",
        "title",       "PS Hue/Saturation",
        "categories",  "Color",
        "description", "Photoshop-style Hue/Saturation.\nBy Akascape",
        NULL);
}

#endif
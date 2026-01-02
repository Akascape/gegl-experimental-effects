/*
Plugin Name: PS Gradient Map
Author: Akascape
Description: Photoshop-style gradient map with 2/3-Point modes
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

property_color (shadow_color, "Shadow Color", "black")
    description ("Color for the shadows (0.0)")

property_color (midtone_color, "Midtone Color", "gray")
    description ("Color for the midtones (Ignored in 2-Color Mode)")

property_color (highlight_color, "Highlight Color", "white")
    description ("Color for the highlights (1.0)")

property_double (midpoint, "Midpoint Position", 50.0)
    description ("Position of the Midtone Color (%)")
    value_range (0.0, 100.0)
    ui_range    (0.0, 100.0)

property_boolean (two_color, "2-Color Mode", FALSE)
    description ("Use only Shadow and Highlight colors (ignores Midtone)")

property_boolean (reverse, "Reverse", FALSE)
    description ("Reverse the gradient direction")

property_boolean (dither, "Dither", TRUE)
    description ("Add noise to reduce banding (Dithering)")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_gradient_map
#define GEGL_OP_C_FILE   "ps_gradientmap.c"

#include <gegl-op.h>

/* Linear mix */
static inline void
mix_rgb (gfloat *out, const gfloat *c1, const gfloat *c2, gfloat t)
{
  out[0] = c1[0] + (c2[0] - c1[0]) * t;
  out[1] = c1[1] + (c2[1] - c1[1]) * t;
  out[2] = c1[2] + (c2[2] - c1[2]) * t;
  out[3] = c1[3] + (c2[3] - c1[3]) * t;
}

/* sRGB to Linear */
static inline gfloat
srgb_to_lin (gfloat c)
{
  if (c <= 0.04045f)
    return c / 12.92f;
  else
    return powf ((c + 0.055f) / 1.055f, 2.4f);
}

/* Linear to sRGB */
static inline gfloat
lin_to_srgb (gfloat c)
{
  if (c <= 0.0031308f)
    return 12.92f * c;
  else
    return 1.055f * powf (c, 1.0f / 2.4f) - 0.055f;
}

/* Simple pseudo-random noise for dithering */
static inline gfloat
random_noise (gint x, gint y)
{
  gfloat dt = (gfloat)x * 12.9898f + (gfloat)y * 78.233f;
  gfloat sn = sinf (dt) * 43758.5453f;
  return sn - floorf (sn);
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

  /* Get Colors as Linear Float */
  gfloat col_shadow[4], col_mid[4], col_high[4];
  gegl_color_get_pixel (o->shadow_color, babl_format ("RGBA float"), col_shadow);
  gegl_color_get_pixel (o->midtone_color, babl_format ("RGBA float"), col_mid);
  gegl_color_get_pixel (o->highlight_color, babl_format ("RGBA float"), col_high);

  gfloat mid_pos   = (gfloat)o->midpoint / 100.0f;
  gboolean two_pt  = o->two_color;
  gboolean reverse = o->reverse;
  gboolean dither  = o->dither;

  /* Clamp mid_pos to avoid division by zero */
  if (mid_pos < 0.01f) mid_pos = 0.01f;
  if (mid_pos > 0.99f) mid_pos = 0.99f;

  const Babl *format = babl_format ("RGBA float");
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const gint W = extent->width;
  const gint H = extent->height;

  gfloat *src = g_new (gfloat, (glong)W * H * 4);
  gegl_buffer_get (input, extent, 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  /* Convert gradient stops to sRGB once  */
  gfloat grad_shadow[4], grad_mid[4], grad_high[4];
  for (int i = 0; i < 3; i++) {
    grad_shadow[i] = lin_to_srgb (col_shadow[i]);
    grad_mid[i]    = lin_to_srgb (col_mid[i]);
    grad_high[i]   = lin_to_srgb (col_high[i]);
  }
  grad_shadow[3] = col_shadow[3];
  grad_mid[3]    = col_mid[3];
  grad_high[3]   = col_high[3];

  /* Dither intensity */
  const gfloat dither_amt = 1.0f / 255.0f;

  for (gint y = roi->y; y < roi->y + roi->height; y++) {
    for (gint x = roi->x; x < roi->x + roi->width; x++) {
      gint ix = x - extent->x;
      gint iy = y - extent->y;

      glong idx = (glong)(iy * W + ix) * 4;

      gfloat in_r = src[idx + 0];
      gfloat in_g = src[idx + 1];
      gfloat in_b = src[idx + 2];
      gfloat in_a = src[idx + 3];

      /* Convert input to sRGB */
      gfloat s_r = lin_to_srgb (in_r);
      gfloat s_g = lin_to_srgb (in_g);
      gfloat s_b = lin_to_srgb (in_b);

      /* Calculate Luma */
      gfloat luma = s_r * 0.299f + s_g * 0.587f + s_b * 0.114f;
      
      /* Apply Reverse */
      if (reverse) {
        luma = 1.0f - luma;
      }

      /* Apply Dither */
      if (dither) {
        gfloat noise = random_noise (ix, iy); 
        luma += (noise - 0.5f) * dither_amt;
      }

      luma = CLAMP (luma, 0.0f, 1.0f);

      gfloat out_px_srgb[4];

      if (two_pt) {
        /* 2-Point Mode: Simple lerp from Shadow to Highlight */
        mix_rgb (out_px_srgb, grad_shadow, grad_high, luma);
      } else {
        /* 3-Point Mode: Interpolate via Midpoint */
        if (luma < mid_pos) {
          gfloat t = luma / mid_pos;
          mix_rgb (out_px_srgb, grad_shadow, grad_mid, t);
        } else {
          gfloat t = (luma - mid_pos) / (1.0f - mid_pos);
          mix_rgb (out_px_srgb, grad_mid, grad_high, t);
        }
      }

      /* Convert result back to Linear for GEGL output */
      gfloat out_px[4];
      out_px[0] = srgb_to_lin (out_px_srgb[0]);
      out_px[1] = srgb_to_lin (out_px_srgb[1]);
      out_px[2] = srgb_to_lin (out_px_srgb[2]);

      /* Restore original alpha */
      out_px[3] = in_a;

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
    "name",        "akascape:ps-gradientmap",
    "title",       "PS Gradient Map",
    "categories",  "Color",
    "description", "Gradient Map with 2-Color/3-Color modes\nMade By Akascape",
    NULL);
}

#endif
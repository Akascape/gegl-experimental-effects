/*
Plugin Name: LCD
Author: Akascape
Description: Simulated LCD subpixel grid effect
License: GPLv3
Copyright (C) 2026 Akascape
www.akascape.com
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

property_double (scale, "Scale", 10.0)
  description  ("Size of the LCD subpixel")
  value_range  (1.0, 50.0)
  ui_range     (1.0, 10.0)

property_double (brightness, "Brightness", 2.5)
  description  ("Boost the brightness of the subpixels")
  value_range  (1.0, 10.0)
  ui_range     (1.0, 5.0)

#else /* GEGL_PROPERTIES */

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     lcd
#define GEGL_OP_C_FILE   "lcd.c"

#include <gegl-op.h>

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
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const gint W = extent->width;
  const gint H = extent->height;
  if (W <= 0 || H <= 0) return TRUE;

  gfloat *src = g_new (gfloat, (glong)W * H * 4);
  gegl_buffer_get (input, extent, 1.0, format, src, 
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  const gfloat scale      = (gfloat)o->scale;
  const gfloat brightness = (gfloat)o->brightness;

  for (gint y = roi->y; y < roi->y + roi->height; y++) {
    for (gint x = roi->x; x < roi->x + roi->width; x++) {
      gint ix = x - extent->x;
      gint iy = y - extent->y;

      /* Shader Logic: Find the center of the LCD cell */
      gfloat cell_x = floorf ((gfloat)ix / scale) * scale;
      gfloat cell_y = floorf ((gfloat)iy / scale) * scale;

      /* Sample the source at the cell center */
      gint sx = (gint)CLAMP (cell_x, 0, W - 1);
      gint sy = (gint)CLAMP (cell_y, 0, H - 1);
      glong s_idx = (glong)(sy * W + sx) * 4;

      gfloat in_r = src[s_idx + 0];
      gfloat in_g = src[s_idx + 1];
      gfloat in_b = src[s_idx + 2];
      gfloat in_a = src[s_idx + 3];

      /* Determine which subpixel (R, G, or B) we are inside */
      gfloat subpixel_pos = fmodf ((gfloat)ix, scale) / scale;

      gfloat mask_r = 0.0f;
      gfloat mask_g = 0.0f;
      gfloat mask_b = 0.0f;

      if (subpixel_pos < 0.333f) {
          mask_r = 1.0f;
      } else if (subpixel_pos < 0.666f) {
          mask_g = 1.0f;
      } else {
          mask_b = 1.0f;
      }

      /* Horizontal gap (black line between cells) */
      if (fmodf ((gfloat)ix, scale) < (scale * 0.1f)) {
          mask_r *= 0.2f; mask_g *= 0.2f; mask_b *= 0.2f;
      }
      /* Vertical gap (scanline effect) */
      if (fmodf ((gfloat)iy, scale) < (scale * 0.1f)) {
          mask_r *= 0.2f; mask_g *= 0.2f; mask_b *= 0.2f;
      }

      gfloat out_px[4] = {
        in_r * mask_r * brightness,
        in_g * mask_g * brightness,
        in_b * mask_b * brightness,
        in_a
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
    "name",        "akascape-lcd",
    "title",       "LCD Effect",
    "categories",  "Artistic",
    "description", "LCD subpixel grid effect\nMade By Akascape",
    NULL);
}

#endif /* GEGL_PROPERTIES */
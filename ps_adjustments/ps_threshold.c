/*
Plugin Name: PS Threshold
Author: Akascape
Description: Photoshop-style Threshold adjustment layer
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

property_double (level, "Threshold Level", 128.0)
  description  ("Threshold level (1-255). Pixels brighter than this become white.")
  value_range  (1.0, 255.0)
  ui_range     (1.0, 255.0)

#else /* GEGL_PROPERTIES */

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_threshold
#define GEGL_OP_C_FILE   "ps_threshold.c"

#include <gegl-op.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  const Babl *format = babl_format ("R'G'B'A float");
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const gint W = extent->width;
  const gint H = extent->height;

  if (W <= 0 || H <= 0) return TRUE;

  gfloat *src = g_new (gfloat, (glong)W * H * 4);
  gegl_buffer_get (input, extent, 1.0, format, src, 
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  /* Normalize the user input (1-255) to 0.0-1.0 float range */
  gfloat threshold_val = (gfloat)o->level / 255.0f;

  for (gint y = roi->y; y < roi->y + roi->height; y++) {
    for (gint x = roi->x; x < roi->x + roi->width; x++) {
      gint ix = x - extent->x;
      gint iy = y - extent->y;
      
      glong idx = (glong)(iy * W + ix) * 4;

      gfloat r = src[idx + 0];
      gfloat g = src[idx + 1];
      gfloat b = src[idx + 2];
      gfloat a = src[idx + 3];

      /* This converts the color pixel to a grayscale value */
      gfloat luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;

      /* Binary Threshold  */
      gfloat val = (luma >= threshold_val) ? 1.0f : 0.0f;

      gfloat out_px[4] = { val, val, val, a };

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
    "name",        "akascape:ps-threshold",
    "title",       "PS Threshold",
    "categories",  "Artistic",
    "description", "Photoshop-style Threshold adjustment\nMade By Akascape",
    NULL);
}

#endif /* GEGL_PROPERTIES */
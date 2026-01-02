/*
Plugin Name: PS Posterize
Author: Akascape
Description: Photoshop-style Posterize effect
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

property_int (levels, "Levels", 4)
  description  ("Number of tonal levels per channel (2-255)")
  value_range  (2, 255)
  ui_range     (2, 255)

#else /* GEGL_PROPERTIES */

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_posterize
#define GEGL_OP_C_FILE   "ps_posterize.c"

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
  
  gfloat n_levels = (gfloat)o->levels;
  gfloat max_idx  = n_levels - 1.0f;

  for (gint y = roi->y; y < roi->y + roi->height; y++) {
    for (gint x = roi->x; x < roi->x + roi->width; x++) {
      gint ix = x - extent->x;
      gint iy = y - extent->y;
      
      glong idx = (glong)(iy * W + ix) * 4;

      gfloat r = src[idx + 0];
      gfloat g = src[idx + 1];
      gfloat b = src[idx + 2];

      gfloat ir = floorf (r * n_levels);
      gfloat ig = floorf (g * n_levels);
      gfloat ib = floorf (b * n_levels);

      ir = CLAMP (ir, 0.0f, max_idx);
      ig = CLAMP (ig, 0.0f, max_idx);
      ib = CLAMP (ib, 0.0f, max_idx);

      gfloat out_px[4] = {
        ir / max_idx,
        ig / max_idx,
        ib / max_idx,
        src[idx + 3]
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
    "name",        "akascape:ps-posterize",
    "title",       "PS Posterize",
    "categories",  "Artistic",
    "description", "Photoshop-style Posterize effect\nMade By Akascape",
    NULL);
}

#endif /* GEGL_PROPERTIES */
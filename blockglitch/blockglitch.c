/*
Plugin Name: BlockGlitch
Author: Akascape
Description: Blocky slice glitch with channel split
License: GPLv3
Copyright (C) 2026 Akascape
www.akascape.com
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* ---- UI Properties ---- */

enum_start (split_color_enum)
  enum_value (SPLIT_COLOR_RED_CYAN,     "red-cyan",     N_("Red - Cyan"))
  enum_value (SPLIT_COLOR_GREEN_MAGENTA,"green-magenta",N_("Green - Magenta"))
  enum_value (SPLIT_COLOR_BLUE_YELLOW,  "blue-yellow",  N_("Blue - Yellow"))
enum_end (SplitColorEnum)

property_double (slice_offset, "Slice Offset", 0.90)
  description  ("Amount of horizontal slice glitch")
  value_range  (0.0, 1.0)
  ui_range     (0.0, 1.0)

property_double (seed, "Seed", 0.20)
  description  ("Random seed for glitch pattern")
  value_range  (0.0, 1000.0)
  ui_range     (0.0, 10.0)

property_double (block_size, "Block Size", 0.25)
  description  ("Vertical size of glitch blocks")
  value_range  (0.0, 1.0)
  ui_range     (0.0, 0.5)

property_enum (split_color, "Split Color",
               SplitColorEnum, split_color_enum,
               SPLIT_COLOR_RED_CYAN)
  description ("Color channel split pair")

#else /* GEGL_PROPERTIES */

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     blockglitch
#define GEGL_OP_C_FILE   "blockglitch.c"

#include <gegl-op.h>

typedef struct { gfloat x, y; } Vec2;

static inline gfloat fractf (gfloat x)
{
  return x - floorf (x);
}

static inline gfloat stepf (gfloat edge, gfloat x)
{
  return (x < edge) ? 0.0f : 1.0f;
}

/* random2d from shader */
static inline gfloat random2d (Vec2 n)
{
  gfloat d = n.x * 12.9898f + n.y * 4.1414f;
  return fractf (sinf (d) * 43758.5453f);
}

static inline gfloat randomRange (Vec2 seed, gfloat min, gfloat max)
{
  return min + random2d (seed) * (max - min);
}

static inline gfloat insideRange (gfloat v, gfloat bottom, gfloat top)
{
  return stepf (bottom, v) - stepf (top, v);
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

  const gfloat slice_offset = (gfloat)o->slice_offset;  /* AMT */
  const gfloat seed        = (gfloat)o->seed;           /* SPEED -> Seed */
  const gfloat block_size  = (gfloat)o->block_size;
  const gint   split_color = o->split_color;

  const Babl *format = babl_format ("RGBA float");
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const gint W = extent->width;
  const gint H = extent->height;

  if (W <= 0 || H <= 0)
    return TRUE;

  const gfloat fW = (gfloat)W;
  const gfloat fH = (gfloat)H;

  const glong n_pix = (glong)W * (glong)H;

  /* Grab full source as RGBA float */
  gfloat *src = g_new (gfloat, n_pix * 4);
  gegl_buffer_get (input, extent, 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  const gfloat time_val = floorf (seed * 60.0f);

  gint num_slices = (gint)floorf (10.0f * slice_offset);
  if (num_slices < 0) num_slices = 0;
  if (num_slices > 50) num_slices = 50; 

  const gfloat maxOffset    = slice_offset * 0.5f;
  const gfloat maxColOffset = slice_offset / 6.0f;

  /* MAIN ROI LOOP */
  for (gint y = roi->y; y < roi->y + roi->height; y++)
  for (gint x = roi->x; x < roi->x + roi->width; x++)
  {
    gint ix = x - extent->x;
    gint iy = y - extent->y;

    if (ix < 0 || iy < 0 || ix >= W || iy >= H)
      continue;

    glong idx0 = (glong)iy * W + ix;
    gfloat *in_px = &src[idx0 * 4];

    gfloat uvx = ((gfloat)ix + 0.5f) / fW;
    gfloat uvy = ((gfloat)iy + 0.5f) / fH;

    gfloat out_r = in_px[0];
    gfloat out_g = in_px[1];
    gfloat out_b = in_px[2];
    gfloat out_a = in_px[3];

    /* ---- Horizontal slice glitch (from shader loop) ---- */
    if (num_slices > 0 && slice_offset > 0.0f)
    {
      for (gint i = 0; i < num_slices; i++)
      {
        Vec2 s1 = { time_val, 2345.0f + (gfloat)i };
        Vec2 s2 = { time_val, 9035.0f + (gfloat)i };
        Vec2 s3 = { time_val, 9625.0f + (gfloat)i };

        gfloat sliceY = random2d (s1);
        gfloat sliceH = random2d (s2) * block_size;   
        gfloat hOffset = randomRange (s3, -maxOffset, maxOffset);

        gfloat uvOffx = uvx + hOffset;

        /* Clamp horizontally */
        if (uvOffx < 0.0f) uvOffx = 0.0f;
        if (uvOffx > 1.0f) uvOffx = 1.0f;

        gfloat in_range = insideRange (uvy, sliceY, fractf (sliceY + sliceH));

        if (in_range >= 1.0f)
        {
          /* Sample from offset x at same y (nearest-neighbour) */
          gint sx = (gint)(uvOffx * fW);
          if (sx < 0)     sx = 0;
          if (sx >= W)    sx = W - 1;

          glong sidx = (glong)iy * W + sx;
          gfloat *spx = &src[sidx * 4];

          out_r = spx[0];
          out_g = spx[1];
          out_b = spx[2];
        }
      }
    }

    if (slice_offset > 0.0f)
    {
      Vec2 seed_x = { time_val, 9545.0f };
      Vec2 seed_y = { time_val, 7205.0f };
      Vec2 rnd_s  = { time_val, 9545.0f };

      gfloat offx = randomRange (seed_x, -maxColOffset, maxColOffset);
      gfloat offy = randomRange (seed_y, -maxColOffset, maxColOffset);

      gfloat rnd  = random2d (rnd_s); 

      (void)rnd; 

      gfloat uv_col_x = uvx + offx;
      gfloat uv_col_y = uvy + offy;

      if (uv_col_x < 0.0f) uv_col_x = 0.0f;
      if (uv_col_x > 1.0f) uv_col_x = 1.0f;
      if (uv_col_y < 0.0f) uv_col_y = 0.0f;
      if (uv_col_y > 1.0f) uv_col_y = 1.0f;

      gint sx = (gint)(uv_col_x * fW);
      gint sy = (gint)(uv_col_y * fH);

      if (sx < 0)  sx = 0;
      if (sx >= W) sx = W - 1;
      if (sy < 0)  sy = 0;
      if (sy >= H) sy = H - 1;

      glong sidx = (glong)sy * W + sx;
      gfloat *cp = &src[sidx * 4];

      switch (split_color)
      {
        case SPLIT_COLOR_RED_CYAN:
          /* Move Red channel, keep Green+Blue as base (Cyan) */
          out_r = cp[0];
          break;

        case SPLIT_COLOR_GREEN_MAGENTA:
          /* Move Green channel, keep Red+Blue as base (Magenta) */
          out_g = cp[1];
          break;

        case SPLIT_COLOR_BLUE_YELLOW:
        default:
          /* Move Blue channel, keep Red+Green as base (Yellow) */
          out_b = cp[2];
          break;
      }
    }

    gfloat out_px[4] = { out_r, out_g, out_b, out_a };

    gegl_buffer_set (output,
                     GEGL_RECTANGLE (x, y, 1, 1),
                     0, format, out_px,
                     GEGL_AUTO_ROWSTRIDE);
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
    "name",        "akascape-blockglitch",
    "title",       "BlockGlitch",
    "categories",  "Artistic",
    "description", "Blocky slice glitch with channel split\nMade By Akascape",
    NULL);
}

#endif /* GEGL_PROPERTIES */

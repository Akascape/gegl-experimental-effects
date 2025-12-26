/*
Plugin Name: Modulation
Author: Akascape
Description: Directional FM Modulation Effect
License: GPLv3
Copyright (C) 2025 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/*---- UI Properties ----*/
enum_start (modulation_direction_enum)
  enum_value (MODULATION_DIRECTION_RIGHT, "right", N_("Right"))
  enum_value (MODULATION_DIRECTION_LEFT,  "left",  N_("Left"))
  enum_value (MODULATION_DIRECTION_UP,    "up",    N_("Up"))
  enum_value (MODULATION_DIRECTION_DOWN,  "down",  N_("Down"))
enum_end (ModulationDirectionEnum)

enum_start (modulation_channel_enum)
  enum_value (MODULATION_CHANNEL_RED,   "red",   N_("Red"))
  enum_value (MODULATION_CHANNEL_GREEN, "green", N_("Green"))
  enum_value (MODULATION_CHANNEL_BLUE,  "blue",  N_("Blue"))
enum_end (ModulationChannelEnum)

property_double (line_ratio, _("Frequency"), 0.10)
  value_range   (0.0, 0.5)
  ui_range      (0.0, 0.5)

property_double (hardness, _("Density"), 0.5)
  value_range   (0.0, 5.0)
  ui_range      (0.0, 2.0)

property_double (width, _("Line Width"), 2.5)
  value_range   (0.1, 10.0)
  ui_range      (0.1, 10.0)

property_enum (direction, _("Direction"),
               ModulationDirectionEnum, modulation_direction_enum,
               MODULATION_DIRECTION_RIGHT)

property_enum (combobox, _("Mod Channel"),
               ModulationChannelEnum, modulation_channel_enum,
               MODULATION_CHANNEL_RED)

#else /* GEGL_PROPERTIES */

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME    modulation
#define GEGL_OP_C_FILE  "modulation.c"

#include <gegl-op.h>

#define NB_SAMPLE 15

typedef struct { gfloat x, y; } Vec2;

static inline gfloat fractf (gfloat x) { return x - floorf(x); }

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

/* We define a helper to handle the math logic to keep the process loop clean */
static inline void
do_modulation_math (gint x, gint y, gint W, gint H, 
                    const GeglRectangle *extent, GeglProperties *o,
                    gfloat *prefixH, gfloat *prefixV, 
                    gfloat tex_in, gfloat alpha_in, gfloat *pixel_out)
{
  const gfloat line_ratio = (gfloat)o->line_ratio;
  const gfloat hardness   = (gfloat)o->hardness;
  const gfloat width_val  = (gfloat)o->width;
  const gint   direction  = o->direction;

  gint ix = x - extent->x;
  gint iy = y - extent->y;
  gfloat uvx = (gfloat)ix / (gfloat)W;
  gfloat uvy = (gfloat)iy / (gfloat)H;

  Vec2 p[NB_SAMPLE];
  const gint mid = NB_SAMPLE / 2 - 1;

  for (gint si = 0; si < NB_SAMPLE; si++) {
    gfloat fi = (gfloat)si;
    gfloat half = (gfloat)mid;
    gfloat sx = uvx;
    gfloat sy = uvy;

    if (direction < 2)
      sx += (direction == 0 ? (fi-half)/W : (half-fi)/W);
    else
      sy += (direction == 2 ? (fi-half)/H : (half-fi)/H);

    gfloat sxp = sx * W;
    gfloat syp = sy * H;
    gfloat sum = 0.0f;

    // Use boundary checks to prevent segfaults
    if (direction == 0) { // Right
      gint yy = (gint)(syp + 0.5f);
      if (yy >= 0 && yy < H) {
        gint mx = CLAMP((gint)sxp, 0, W);
        if (mx > 0) sum = prefixH[yy * W + (mx - 1)];
      }
    } else if (direction == 1) { // Left
      gint yy = (gint)(syp + 0.5f);
      if (yy >= 0 && yy < H) {
        gint st = CLAMP((gint)sxp, 0, W);
        gfloat tot = prefixH[yy * W + (W - 1)];
        sum = tot - (st > 0 ? prefixH[yy * W + (st - 1)] : 0);
      }
    } else if (direction == 2) { // Up
      gint xx = (gint)(sxp + 0.5f);
      if (xx >= 0 && xx < W) {
        gint my = CLAMP((gint)syp, 0, H);
        if (my > 0) sum = prefixV[(my - 1) * W + xx];
      }
    } else { // Down
      gint xx = (gint)(sxp + 0.5f);
      if (xx >= 0 && xx < W) {
        gint st = CLAMP((gint)syp, 0, H);
        gfloat tot = prefixV[(H - 1) * W + xx];
        sum = tot - (st > 0 ? prefixV[(st - 1) * W + xx] : 0);
      }
    }

    gfloat scaled = sum * line_ratio * (direction < 2 ? W : H);
    p[si].x = floorf(scaled);
    p[si].y = fractf(scaled);
  }

  gfloat c = 0.0f;
  for (gint si = 0; si < NB_SAMPLE - 1; si++) {
    if (p[si+1].x - p[si].x > 0.5f) {
      gfloat v = ((gfloat)si - (gfloat)mid) + (1.0f - p[si].y) / ((1.0f - p[si].y) + p[si+1].y);
      gfloat val = 1.0f - fabsf(v / width_val);
      if (val > 0.0f) c += val;
    }
  }

  gfloat out_luma = (c > 0.0f) ? tex_in * powf(c, hardness) : 0.0f;
  pixel_out[0] = out_luma;
  pixel_out[1] = out_luma;
  pixel_out[2] = out_luma;
  pixel_out[3] = alpha_in;
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  const Babl *format = gegl_operation_get_format (op, "input");
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const gint W = extent->width;
  const gint H = extent->height;

  if (W <= 0 || H <= 0) return TRUE;

  /* 1. Pre-calculate prefix sums for the whole buffer */
  gfloat *full_src = g_new (gfloat, W * H * 4);
  gfloat *prefixH  = g_new0 (gfloat, W * H);
  gfloat *prefixV  = g_new0 (gfloat, W * H);

  gegl_buffer_get (input, extent, 1.0, format, full_src, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  for (gint y = 0; y < H; y++) {
    gfloat acc = 0.0f;
    for (gint x = 0; x < W; x++) {
      acc += full_src[(y * W + x) * 4 + o->combobox] / (gfloat)W;
      prefixH[y * W + x] = acc;
    }
  }
  for (gint x = 0; x < W; x++) {
    gfloat acc = 0.0f;
    for (gint y = 0; y < H; y++) {
      acc += full_src[(y * W + x) * 4 + o->combobox] / (gfloat)H;
      prefixV[y * W + x] = acc;
    }
  }

  /* 2. Setup Iterator for the ROI */
  GeglBufferIterator *gi = gegl_buffer_iterator_new (input, roi, level, format,
                                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (gi, output, roi, level, format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi)) {
    gfloat *src_buf = (gfloat *)gi->items[0].data;
    gfloat *dst_buf = (gfloat *)gi->items[1].data;
    GeglRectangle *chunk = &gi->items[0].roi;

    for (gint y = chunk->y; y < chunk->y + chunk->height; y++) {
      for (gint x = chunk->x; x < chunk->x + chunk->width; x++) {
        
        // Calculate the local index in the iterator's current chunk
        gint it_idx = ((y - chunk->y) * chunk->width + (x - chunk->x)) * 4;
        
        // Get input pixel from current iterator stream
        gfloat tex_in   = src_buf[it_idx + o->combobox];
        gfloat alpha_in = src_buf[it_idx + 3];

        do_modulation_math (x, y, W, H, extent, o, prefixH, prefixV, 
                           tex_in, alpha_in, &dst_buf[it_idx]);
      }
    }
  }

  g_free (full_src);
  g_free (prefixH);
  g_free (prefixV);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "akascape:modulation",
    "title",       _("Modulation"),
    "categories",  "artistic",
    "description", _("Frequency Modulation Effect"),
    NULL);
}

#endif 
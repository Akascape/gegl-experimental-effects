/*
Plugin Name: PS Curves
Author: Akascape
Description: Photoshop-style Curves Adjustment 
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>
#include <stdlib.h>

#ifdef GEGL_PROPERTIES

enum_start (curves_channel_enum)
  enum_value (CURVES_CHANNEL_RGB,   "rgb",   N_("Master (RGB)"))
  enum_value (CURVES_CHANNEL_RED,   "red",   N_("Red"))
  enum_value (CURVES_CHANNEL_GREEN, "green", N_("Green"))
  enum_value (CURVES_CHANNEL_BLUE,  "blue",  N_("Blue"))
  enum_value (CURVES_CHANNEL_ALPHA, "alpha", N_("Alpha"))
enum_end (CurvesChannelEnum)

enum_start (curves_points_enum)
  enum_value (CURVES_POINTS_0, "0", N_("0 Points (Linear)"))
  enum_value (CURVES_POINTS_1, "1", N_("1 Point"))
  enum_value (CURVES_POINTS_2, "2", N_("2 Points"))
  enum_value (CURVES_POINTS_3, "3", N_("3 Points"))
enum_end (CurvesPointsEnum)

/* ---- UI Properties ---- */

property_enum (channel, "Channel",
               CurvesChannelEnum, curves_channel_enum,
               CURVES_CHANNEL_RGB)
    description ("Select the channel to adjust")

property_enum (point_count, "Active Points",
               CurvesPointsEnum, curves_points_enum,
               CURVES_POINTS_0)
    description ("Number of control points")

/* ---- Point 1 ---- */
property_double (p1_x, "Point 1 X", 64.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

property_double (p1_y, "Point 1 Y", 64.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

/* ---- Point 2 ---- */
property_double (p2_x, "Point 2 X", 128.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

property_double (p2_y, "Point 2 Y", 128.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

/* ---- Point 3 ---- */
property_double (p3_x, "Point 3 X", 192.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

property_double (p3_y, "Point 3 Y", 192.0)
  value_range (0.0, 255.0)
  ui_range    (0.0, 255.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_curves
#define GEGL_OP_C_FILE   "ps_curves.c"

#include <gegl-op.h>

#define LUT_SIZE   4096
#define MAX_POINTS 5

typedef struct {
  gfloat x, y;
} Point;

/* Photoshop-like perceptual domain */

static inline gfloat
to_gamma (gfloat v)
{
  return powf (CLAMP (v, 0.0f, 1.0f), 1.0f / 2.2f);
}

static inline gfloat
to_linear (gfloat v)
{
  return powf (CLAMP (v, 0.0f, 1.0f), 2.2f);
}

static int
compare_points (const void *a, const void *b)
{
  const Point *p1 = a;
  const Point *p2 = b;
  return (p1->x > p2->x) - (p1->x < p2->x);
}

/* Fritsch–Carlson + Photoshop-style clamping */

static void
calculate_lut (gfloat *lut, Point *p, int n)
{
  gfloat d[MAX_POINTS];
  gfloat m[MAX_POINTS];

  /* Slopes */
  for (int i = 0; i < n - 1; i++) {
    gfloat dx = p[i+1].x - p[i].x;
    gfloat dy = p[i+1].y - p[i].y;
    d[i] = (dx > 1e-6f) ? dy / dx : 0.0f;
  }

  /* Endpoint tangents */
  m[0]     = d[0];
  m[n - 1] = d[n - 2];

  /* Interior tangents */
  for (int i = 1; i < n - 1; i++) {
    if (d[i-1] * d[i] <= 0.0f)
      m[i] = 0.0f;
    else
      m[i] = 0.5f * (d[i-1] + d[i]);
  }

  /* Photoshop-style curvature limiting */
  for (int i = 0; i < n - 1; i++) {
    if (fabsf (d[i]) < 1e-6f) {
      m[i] = m[i+1] = 0.0f;
      continue;
    }

    gfloat a = m[i]   / d[i];
    gfloat b = m[i+1] / d[i];

    if (a*a + b*b > 9.0f) {
      gfloat t = 3.0f / sqrtf (a*a + b*b);
      m[i]   *= t;
      m[i+1] *= t;
    }
  }

  /* Build LUT */
  int seg = 0;
  for (int i = 0; i < LUT_SIZE; i++) {
    gfloat x = (gfloat)i / (LUT_SIZE - 1);

    while (seg < n - 2 && x > p[seg+1].x)
      seg++;

    gfloat x0 = p[seg].x;
    gfloat x1 = p[seg+1].x;
    gfloat y0 = p[seg].y;
    gfloat y1 = p[seg+1].y;
    gfloat h  = x1 - x0;

    if (h < 1e-6f) {
      lut[i] = y0;
      continue;
    }

    gfloat t  = (x - x0) / h;
    gfloat t2 = t * t;
    gfloat t3 = t2 * t;

    gfloat h00 =  2*t3 - 3*t2 + 1;
    gfloat h10 =      t3 - 2*t2 + t;
    gfloat h01 = -2*t3 + 3*t2;
    gfloat h11 =      t3 - t2;

    gfloat y =
      h00*y0 + h10*h*m[seg] +
      h01*y1 + h11*h*m[seg+1];

    lut[i] = CLAMP (y, 0.0f, 1.0f);
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
process (GeglOperation *op,
         GeglBuffer *input,
         GeglBuffer *output,
         const GeglRectangle *roi,
         gint level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);

  Point pts[MAX_POINTS];
  int count = 0;

  pts[count++] = (Point){0.0f, 0.0f};

  if (o->point_count >= 1)
    pts[count++] = (Point){o->p1_x / 255.0f, o->p1_y / 255.0f};
  if (o->point_count >= 2)
    pts[count++] = (Point){o->p2_x / 255.0f, o->p2_y / 255.0f};
  if (o->point_count >= 3)
    pts[count++] = (Point){o->p3_x / 255.0f, o->p3_y / 255.0f};

  pts[count++] = (Point){1.0f, 1.0f};

  qsort (pts, count, sizeof (Point), compare_points);

  gfloat *lut = g_new (gfloat, LUT_SIZE);
  calculate_lut (lut, pts, count);

  const Babl *fmt = babl_format ("RGBA float");

  GeglRectangle rect = *roi;
  gfloat *buf = g_new (gfloat, rect.width * rect.height * 4);

  gegl_buffer_get (input, &rect, 1.0, fmt, buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

#define SAMPLE(v) \
  ({ gfloat g = to_gamma (v); \
     gfloat p = g * (LUT_SIZE - 1); \
     int i0 = (int)p; \
     int i1 = (i0 < LUT_SIZE - 1) ? i0 + 1 : i0; \
     to_linear (lut[i0] + (lut[i1] - lut[i0]) * (p - i0)); })

  for (int i = 0; i < rect.width * rect.height; i++) {
    gfloat *px = buf + i*4;

    if (o->channel == CURVES_CHANNEL_RGB || o->channel == CURVES_CHANNEL_RED)
      px[0] = SAMPLE (px[0]);
    if (o->channel == CURVES_CHANNEL_RGB || o->channel == CURVES_CHANNEL_GREEN)
      px[1] = SAMPLE (px[1]);
    if (o->channel == CURVES_CHANNEL_RGB || o->channel == CURVES_CHANNEL_BLUE)
      px[2] = SAMPLE (px[2]);
    if (o->channel == CURVES_CHANNEL_ALPHA)
      px[3] = SAMPLE (px[3]);
  }

  gegl_buffer_set (output, &rect, 0, fmt, buf, GEGL_AUTO_ROWSTRIDE);

  g_free (buf);
  g_free (lut);
  return TRUE;
}

/* ===================== CLASS INIT ===================== */

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *op_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  op_class->prepare = prepare;
  filter_class->process = process;

  gegl_operation_class_set_keys (op_class,
    "name",        "akascape:ps-curves",
    "title",       "PS Curves",
    "categories",  "Color",
    "description", "Photoshop-style Curves \nBy Akascape",
    NULL);
}

#endif

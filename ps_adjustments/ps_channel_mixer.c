/*
Plugin Name: PS Channel Mixer
Author: Akascape
Description: Photoshop-style Channel Mixer adjustment 
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (ps_channel_mixer_target_enum)
  enum_value (TARGET_CHAN_RED,   "red",   N_("Red"))
  enum_value (TARGET_CHAN_GREEN, "green", N_("Green"))
  enum_value (TARGET_CHAN_BLUE,  "blue",  N_("Blue"))
enum_end (PsChannelMixerTargetEnum)

property_enum (target_channel, "Channel",
               PsChannelMixerTargetEnum, ps_channel_mixer_target_enum,
               TARGET_CHAN_RED)
    description ("Select the output channel to modify (Ignored in Monochromatic mode)")

property_boolean (monochromatic, "Monochromatic", FALSE)
    description ("Create a grayscale image using the slider weights")

property_double (red_adj, "Red", 100.0)
    description ("Red channel contribution (%)")
    value_range (-200.0, 200.0)
    ui_range    (-200.0, 200.0)
    ui_meta     ("unit", "%")

property_double (green_adj, "Green", 0.0)
    description ("Green channel contribution (%)")
    value_range (-200.0, 200.0)
    ui_range    (-200.0, 200.0)
    ui_meta     ("unit", "%")

property_double (blue_adj, "Blue", 0.0)
    description ("Blue channel contribution (%)")
    value_range (-200.0, 200.0)
    ui_range    (-200.0, 200.0)
    ui_meta     ("unit", "%")

property_double (constant, "Total", 0.0)
    description ("Constant offset added to the channel (often labeled Constant or Total)")
    value_range (-200.0, 200.0)
    ui_range    (-200.0, 200.0)
    ui_meta     ("unit", "%")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ps_channel_mixer
#define GEGL_OP_C_FILE   "ps_channel_mixer.c"

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

    const gboolean mono = o->monochromatic;
    const gint target = o->target_channel;

    /* Convert percentages to normalized factors */
    const gfloat w_r = (gfloat)o->red_adj / 100.0f;
    const gfloat w_g = (gfloat)o->green_adj / 100.0f;
    const gfloat w_b = (gfloat)o->blue_adj / 100.0f;
    const gfloat w_c = (gfloat)o->constant / 100.0f;

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

            gfloat in_r = src[idx + 0];
            gfloat in_g = src[idx + 1];
            gfloat in_b = src[idx + 2];
            gfloat in_a = src[idx + 3];

            gfloat out_r = in_r;
            gfloat out_g = in_g;
            gfloat out_b = in_b;

            /* Calculate: (Red*Rw) + (Green*Gw) + (Blue*Bw) + Constant */
            gfloat mixed_val = (in_r * w_r) + (in_g * w_g) + (in_b * w_b) + w_c;

            if (mono) {
                /* Monochromatic Mode: The mix becomes the grayscale value */
                out_r = out_g = out_b = mixed_val;
            } else {
                /* Color Mode: Apply the mix only to the selected channel */
                switch (target) {
                    case TARGET_CHAN_RED:
                        out_r = mixed_val;
                        break;
                    case TARGET_CHAN_GREEN:
                        out_g = mixed_val;
                        break;
                    case TARGET_CHAN_BLUE:
                        out_b = mixed_val;
                        break;
                }
            }

            /* Clamp results to valid range */
            gfloat out_px[4] = {
                CLAMP(out_r, 0.0f, 1.0f),
                CLAMP(out_g, 0.0f, 1.0f),
                CLAMP(out_b, 0.0f, 1.0f),
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
        "name",        "akascape-ps-channel-mixer",
        "title",       "PS Channel Mixer",
        "categories",  "Color",
        "description", "Photoshop-style Channel Mixer adjustment\nMade By Akascape",
        NULL);
}

#endif
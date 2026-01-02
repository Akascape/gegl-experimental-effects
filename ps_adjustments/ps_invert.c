/*
Plugin Name: PS Invert
Author: Akascape
Description: Photoshop-style invert adjustment
License: GPLv3
Copyright (C) 2026 Akascape
*/

#define GETTEXT_PACKAGE "gegl-0.4"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     ps_invert
#define GEGL_OP_C_FILE   "ps_invert.c"

#include <gegl-op.h>

/* ---------------- GEGL Operation ---------------- */

static void
prepare (GeglOperation *operation)
{
    const Babl *space = gegl_operation_get_source_space (operation, "input");
    const Babl *format = babl_format_with_space ("R'G'B'A float", space);

    gegl_operation_set_format (operation, "input", format);
    gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
    gfloat *in  = in_buf;
    gfloat *out = out_buf;

    for (glong i = 0; i < n_pixels; i++)
    {
        /* Simple Inversion: 1.0 - Value */
        out[0] = 1.0f - in[0]; 
        out[1] = 1.0f - in[1]; 
        out[2] = 1.0f - in[2]; 
        out[3] = in[3];        

        in  += 4;
        out += 4;
    }

    return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
    GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
    GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

    operation_class->prepare = prepare;
    point_filter_class->process = process;

    gegl_operation_class_set_keys (operation_class,
        "name",        "akascape:ps-invert",
        "title",       "PS Invert",
        "categories",  "Color",
        "description", "Inverts the colors of the image.\nMade By Akascape",
        NULL);
}

#endif
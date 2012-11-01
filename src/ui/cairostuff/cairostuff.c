/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <cairo.h>
#include "fcitx/fcitx.h"
#include "fcitx-utils/utf8.h"
#include "fcitx-config/fcitx-config.h"
#include "cairostuff.h"

#ifdef _ENABLE_PANGO
#include <pango/pangocairo.h>
#endif

void StringSizeStrict(const char* str, const char* font, int fontSize, int dpi, int* w, int* h)
{
    if (!str || str[0] == 0) {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
    }
    if (!fcitx_utf8_check_string(str)) {
        if (w) *w = 0;
        if (h) *h = 0;

        return;
    }
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);

    SetFontContext(c, font, fontSize, dpi);
#ifdef _ENABLE_PANGO
    PangoRectangle rect;
    PangoContext* pc = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(pc, dpi);
    PangoLayout *layout = pango_layout_new(pc);
    pango_layout_set_text(layout, str, -1);
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_get_pixel_extents(layout, &rect, NULL);
    if (w)
        *w = rect.width;
    if (h)
        *h = rect.height;
    g_object_unref(layout);
#else
    cairo_text_extents_t extents;
    cairo_text_extents(c, str, &extents);
    if (w)
        *w = extents.width;
    if (h)
        *h = extents.height;
#endif
    ResetFontContext();

    cairo_destroy(c);
    cairo_surface_destroy(surface);
}


int
StringWidth(const char *str, const char *font, int fontSize, int dpi)
{
    if (!str || str[0] == 0)
        return 0;
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);
    SetFontContext(c, font, fontSize, dpi);

    int             width = 0;
    StringSizeWithContext(c, dpi, str, &width, NULL);
    ResetFontContext();

    cairo_destroy(c);
    cairo_surface_destroy(surface);

    return width;
}

#ifdef _ENABLE_PANGO
void
StringSizeWithContextReal(cairo_t * c, PangoFontDescription* fontDesc, int dpi, const char *str, int* w, int* h)
{
    if (!str || str[0] == 0) {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
    }
    if (!fcitx_utf8_check_string(str)) {
        if (w) *w = 0;
        if (h) *h = 0;

        return;
    }

    PangoContext *pc = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(pc, dpi);
    PangoLayout *layout = pango_layout_new(pc);
    pango_layout_set_text(layout, str, -1);
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_get_pixel_size(layout, w, h);
    g_object_unref(layout);
    g_object_unref(pc);
}
#else

void
StringSizeWithContextReal(cairo_t * c, const char *str, int* w, int* h)
{
    if (!str || str[0] == 0) {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
    }
    if (!fcitx_utf8_check_string(str)) {
        if (w) *w = 0;
        if (h) *h = 0;

        return;
    }
    cairo_text_extents_t extents;
    cairo_font_extents_t fontextents;
    cairo_text_extents(c, str, &extents);
    cairo_font_extents(c, &fontextents);
    if (w)
        *w = extents.x_advance;
    if (h)
        *h = fontextents.height;
}
#endif

int
FontHeight(const char *font, int fontSize, int dpi)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);

    SetFontContext(c, font, fontSize, dpi);
    int             height = FontHeightWithContext(c, dpi);
    ResetFontContext();

    cairo_destroy(c);
    cairo_surface_destroy(surface);
    return height;
}

#ifdef _ENABLE_PANGO
int
FontHeightWithContextReal(cairo_t* c, PangoFontDescription* fontDesc, int dpi)
{
    int height;

    PangoContext* pc = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(pc, dpi);
    PangoLayout *layout = pango_layout_new(pc);
    pango_layout_set_text(layout, "Ayg中", -1);
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_get_pixel_size(layout, NULL, &height);
    g_object_unref(layout);
    g_object_unref(pc);

    return height;
}
#else

int
FontHeightWithContextReal(cairo_t * c)
{
    cairo_matrix_t matrix;
    cairo_get_font_matrix(c, &matrix);

    int             height = matrix.xx;
    return height;
}
#endif

/*
 * 以指定的颜色在窗口的指定位置输出字串
 */
void
OutputString(cairo_t * c, const char *str, const char *font, int fontSize, int dpi, int x,
             int y, FcitxConfigColor * color)
{
    if (!str || str[0] == 0)
        return;

    cairo_save(c);

    cairo_set_source_rgb(c, color->r, color->g, color->b);
    SetFontContext(c, font, fontSize, dpi);
    OutputStringWithContext(c, dpi, str, x, y);
    ResetFontContext();

    cairo_restore(c);
}

#ifdef _ENABLE_PANGO
void
OutputStringWithContextReal(cairo_t * c, PangoFontDescription* desc, int dpi, const char *str, int x, int y)
{
    if (!str || str[0] == 0)
        return;
    if (!fcitx_utf8_check_string(str))
        return;
    cairo_save(c);

    PangoContext *pc = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(pc, dpi);
    PangoLayout *layout = pango_layout_new(pc);
    pango_layout_set_font_description(layout, desc);
    int height1, height2;
    pango_layout_set_text(layout, "Ayg中", -1);
    pango_layout_get_pixel_size(layout, NULL, &height1);

    pango_layout_set_text(layout, str, -1);
    pango_layout_get_pixel_size(layout, NULL, &height2);

    cairo_move_to(c, x, y + (height1 - height2));
    pango_cairo_show_layout(c, layout);

    cairo_restore(c);
    g_object_unref(layout);
    g_object_unref(pc);
}
#else

void
OutputStringWithContextReal(cairo_t * c, const char *str, int x, int y)
{
    if (!str || str[0] == 0)
        return;
    if (!fcitx_utf8_check_string(str))
        return;
    cairo_save(c);
    int             height = FontHeightWithContextReal(c);
    cairo_move_to(c, x, y + height);
    cairo_show_text(c, str);
    cairo_restore(c);
}
#endif


#ifdef _ENABLE_PANGO
PangoFontDescription* GetPangoFontDescription(const char* font, int size, int dpi)
{
    PangoFontDescription* desc;
    desc = pango_font_description_from_string(font);
    if (dpi)
        pango_font_description_set_size(desc, size * PANGO_SCALE);
    else
        pango_font_description_set_absolute_size(desc, size * PANGO_SCALE);
    return desc;
}
#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;

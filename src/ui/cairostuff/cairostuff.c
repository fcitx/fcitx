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

struct _FcitxCairoTextContext {
    boolean ownSurface;
    cairo_surface_t* surface;
    cairo_t* cr;
#ifdef _ENABLE_PANGO
    PangoContext* pangoContext;
    PangoLayout* pangoLayout;
    PangoFontDescription* fontDesc;
#endif
};


#ifdef _ENABLE_PANGO
static PangoFontDescription* GetPangoFontDescription(const char* font, int size, int dpi)
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

FcitxCairoTextContext* FcitxCairoTextContextCreate(cairo_t* cr)
{
    FcitxCairoTextContext* ctc = fcitx_utils_new(FcitxCairoTextContext);

    if (cr) {
        ctc->cr = cr;
        ctc->ownSurface = false;
    } else {
        ctc->surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
        ctc->ownSurface = true;
        ctc->cr = cairo_create(ctc->surface);
    }
#ifdef _ENABLE_PANGO
    ctc->pangoContext = pango_cairo_create_context(ctc->cr);
    ctc->pangoLayout = pango_layout_new(ctc->pangoContext);
#endif

    return ctc;
}

void FcitxCairoTextContextFree(FcitxCairoTextContext* ctc)
{
#ifdef _ENABLE_PANGO
    g_object_unref(ctc->pangoLayout);
    g_object_unref(ctc->pangoContext);

    if (ctc->fontDesc) {
        pango_font_description_free(ctc->fontDesc);
    }
#endif

    if (ctc->ownSurface) {
        cairo_destroy(ctc->cr);
        cairo_surface_destroy(ctc->surface);
    }
    free(ctc);
}

void FcitxCairoTextContextSet(FcitxCairoTextContext* ctc, const char* font, int fontSize, int dpi)
{
#ifdef _ENABLE_PANGO
    PangoFontDescription* fontDesc = GetPangoFontDescription(font, fontSize, dpi);
    pango_cairo_context_set_resolution(ctc->pangoContext, dpi);
    pango_layout_set_font_description(ctc->pangoLayout, fontDesc);
    if (ctc->fontDesc) {
        pango_font_description_free(ctc->fontDesc);
        ctc->fontDesc = fontDesc;
    }
#else
    cairo_select_font_face(ctc->cr, font,
                            CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctc->cr, fontSize);
#endif
}

void FcitxCairoTextContextStringSize(FcitxCairoTextContext* ctc, const char* str, int* w, int* h)
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

#ifdef _ENABLE_PANGO
    pango_layout_set_text(ctc->pangoLayout, str, -1);
    pango_layout_get_pixel_size(ctc->pangoLayout, w, h);
#else
    cairo_text_extents_t extents;
    cairo_font_extents_t fontextents;
    cairo_text_extents(ctc->cr, str, &extents);
    cairo_font_extents(ctc->cr, &fontextents);
    if (w)
        *w = extents.x_advance;
    if (h)
        *h = fontextents.height;
#endif
}

void FcitxCairoTextContextStringSizeStrict(FcitxCairoTextContext* ctc, const char* str, int* w, int* h)
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

#ifdef _ENABLE_PANGO
    PangoRectangle rect;
    pango_layout_set_text(ctc->pangoLayout, str, -1);
    pango_layout_get_pixel_extents(ctc->pangoLayout, &rect, NULL);
    if (w)
        *w = rect.width;
    if (h)
        *h = rect.height;
#else
    cairo_text_extents_t extents;
    cairo_text_extents(ctc->cr, str, &extents);
    if (w)
        *w = extents.width;
    if (h)
        *h = extents.height;
#endif
}


int FcitxCairoTextContextStringWidth(FcitxCairoTextContext* ctc, const char *str)
{
    if (!str || str[0] == 0)
        return 0;
    int             width = 0;
    FcitxCairoTextContextStringSize(ctc, str, &width, NULL);
    return width;
}

int FcitxCairoTextContextFontHeight(FcitxCairoTextContext* ctc)
{
    int             height = 0;
    FcitxCairoTextContextStringSize(ctc, "Ayg中", NULL, &height);
    return height;
}

void
FcitxCairoTextContextOutputString(FcitxCairoTextContext* ctc, const char* str, int x, int y, FcitxConfigColor* color)
{
    if (!str || str[0] == 0)
        return;
    if (!fcitx_utf8_check_string(str))
        return;
    cairo_save(ctc->cr);
    if (color) {
        cairo_set_source_rgb(ctc->cr, color->r, color->g, color->b);
    }
#ifdef _ENABLE_PANGO
    pango_layout_set_text(ctc->pangoLayout, str, -1);

    cairo_move_to(ctc->cr, x, y);
    pango_cairo_show_layout(ctc->cr, ctc->pangoLayout);
#else
    int             height = FcitxCairoTextContextFontHeight(ctc);
    cairo_move_to(ctc->cr, x, y + height);
    cairo_show_text(ctc->cr, str);
#endif
    cairo_restore(ctc->cr);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;

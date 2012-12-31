/******************************************************************

         Copyright 1994, 1995 by Sun Microsystems, Inc.
         Copyright 1993, 1994 by Hewlett-Packard Company

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sun Microsystems, Inc.
and Hewlett-Packard not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.
Sun Microsystems, Inc. and Hewlett-Packard make no representations about
the suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

SUN MICROSYSTEMS INC. AND HEWLETT-PACKARD COMPANY DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
SUN MICROSYSTEMS, INC. AND HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Author: Hidetoshi Tajima(tajima@Eng.Sun.COM) Sun Microsystems, Inc.

******************************************************************/
#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "Xi18n.h"
#include "IC.h"
#include "fcitx-utils/log.h"
#include "xim.h"
#include "fcitx-utils/utils.h"
#include "ximhandler.h"
#include "fcitx/instance.h"

/**
 * @file IC.c
 *
 * Process XIM Input Context
 */

static int Is(char *attr, XICAttribute * attr_list);
static void StoreIC(FcitxXimIC * rec, IMChangeICStruct * call_data);

static int Is(char *attr, XICAttribute * attr_list)
{
    return !strcmp(attr, attr_list->name);
}

boolean  XimCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    FCITX_UNUSED(arg);
    CARD16* picid = (CARD16*) priv;
    FcitxXimIC* rec = (FcitxXimIC*) context->privateic;
    if (rec->id == *picid)
        return true;
    else
        return false;
}

static void StoreIC(FcitxXimIC * rec, IMChangeICStruct * call_data)
{
    XICAttribute   *ic_attr = call_data->ic_attr;
    XICAttribute   *pre_attr = call_data->preedit_attr;
    XICAttribute   *sts_attr = call_data->status_attr;
    register int    i;

    for (i = 0; i < (int) call_data->ic_attr_num; i++, ic_attr++) {
        if (Is(XNInputStyle, ic_attr))
            rec->input_style = *(INT32 *) ic_attr->value;

        else if (Is(XNClientWindow, ic_attr)) {
            CARD32 value = *(CARD32*) ic_attr->value;
            rec->client_win = (Window) value;
        } else if (Is(XNFocusWindow, ic_attr)) {
            CARD32 value = *(CARD32*) ic_attr->value;
            rec->focus_win = (Window) value;
        }
    }

    for (i = 0; i < (int) call_data->preedit_attr_num; i++, pre_attr++) {
        if (Is(XNArea, pre_attr)) {
            rec->pre_attr.area = *(XRectangle *) pre_attr->value;
        } else if (Is(XNAreaNeeded, pre_attr)) {
            rec->pre_attr.area_needed = *(XRectangle *) pre_attr->value;
        } else if (Is(XNSpotLocation, pre_attr)) {
            rec->pre_attr.spot_location = *(XPoint *) pre_attr->value;
        } else if (Is(XNColormap, pre_attr))
            rec->pre_attr.cmap = *(Colormap *) pre_attr->value;

        else if (Is(XNStdColormap, pre_attr))
            rec->pre_attr.cmap = *(Colormap *) pre_attr->value;

        else if (Is(XNForeground, pre_attr)) {
            rec->pre_attr.foreground = *(CARD32 *) pre_attr->value;
        } else if (Is(XNBackground, pre_attr)) {
            rec->pre_attr.background = *(CARD32 *) pre_attr->value;
        } else if (Is(XNBackgroundPixmap, pre_attr))
            rec->pre_attr.bg_pixmap = *(Pixmap *) pre_attr->value;

        else if (Is(XNFontSet, pre_attr)) {
            int size = strlen((char*)pre_attr->value);
            if (rec->pre_attr.base_font != NULL) {
                if (Is(rec->pre_attr.base_font, pre_attr)) {
                    continue;
                }
                rec->pre_attr.base_font = realloc(rec->pre_attr.base_font,
                                                  size);
            } else {
                rec->pre_attr.base_font = malloc(size);
            }
            memcpy(rec->pre_attr.base_font, pre_attr->value, size);
        } else if (Is(XNLineSpace, pre_attr))
            rec->pre_attr.line_space = *(CARD32 *) pre_attr->value;
        else if (Is(XNCursor, pre_attr))
            rec->pre_attr.cursor = *(Cursor *) pre_attr->value;
    }

    for (i = 0; i < (int) call_data->status_attr_num; i++, sts_attr++) {
        if (Is(XNArea, sts_attr)) {
            rec->sts_attr.area = *(XRectangle *) sts_attr->value;
        } else if (Is(XNAreaNeeded, sts_attr)) {
            rec->sts_attr.area_needed = *(XRectangle *) sts_attr->value;
        } else if (Is(XNColormap, sts_attr)) {
            rec->sts_attr.cmap = *(Colormap *) sts_attr->value;
        } else if (Is(XNStdColormap, sts_attr))
            rec->sts_attr.cmap = *(Colormap *) sts_attr->value;

        else if (Is(XNForeground, sts_attr)) {
            rec->sts_attr.foreground = *(CARD32 *) sts_attr->value;
        } else if (Is(XNBackground, sts_attr)) {
            rec->sts_attr.background = *(CARD32 *) sts_attr->value;
        }

        else if (Is(XNBackgroundPixmap, sts_attr))
            rec->sts_attr.bg_pixmap = *(Pixmap *) sts_attr->value;

        else if (Is(XNFontSet, sts_attr)) {
            int size = strlen((char*)sts_attr->value) + 1;
            if (rec->sts_attr.base_font != NULL) {
                if (Is(rec->sts_attr.base_font, sts_attr))
                    continue;
                rec->sts_attr.base_font = realloc(rec->sts_attr.base_font,
                                                  size);
            } else {
                rec->sts_attr.base_font = malloc(size);
            }
            memcpy(rec->sts_attr.base_font, sts_attr->value, size);
        } else if (Is(XNLineSpace, sts_attr)) {
            rec->sts_attr.line_space = *(CARD32 *) sts_attr->value;
        } else if (Is(XNCursor, sts_attr)) {
            rec->sts_attr.cursor = *(Cursor *) sts_attr->value;
        }
    }
}

/**
 * Interface for XIM Create Input Context
 *
 * @param  context Input Context
 * @param  priv private data passed by CreateIC
 * @return void
 **/
void XimCreateIC(void* arg, FcitxInputContext* context, void *priv)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    IMChangeICStruct * call_data = (IMChangeICStruct *)priv;
    context->privateic = fcitx_utils_malloc0(sizeof(FcitxXimIC));
    FcitxXimIC* privic = (FcitxXimIC*) context->privateic;
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(xim->owner);

    privic->connect_id = call_data->connect_id;
    privic->id = ++ xim->icid;
    privic->offset_x = -1;
    privic->offset_y = -1;
    StoreIC(privic, call_data);
    SetTrackPos(xim, context, call_data);
    call_data->icid = privic->id;

    if (config->shareState == ShareState_PerProgram)
        FcitxInstanceSetICStateFromSameApplication(xim->owner, xim->frontendid, context);

    if (privic->input_style & XIMPreeditCallbacks)
        context->contextCaps |= CAPACITY_PREEDIT;
    else
        context->contextCaps &= ~CAPACITY_PREEDIT;

    return;
}

/**
 * Destroy Input Context for XIM
 *
 * @param context Input Context to Destroy
 * @return void
 **/
void XimDestroyIC(void* arg, FcitxInputContext* context)
{
    FCITX_UNUSED(arg);
    //free resource
    FcitxXimIC* privic = (FcitxXimIC*) context->privateic;
    if (privic->resource_name)
        free(privic->resource_name);
    if (privic->resource_class)
        free(privic->resource_class);

    context->privateic = NULL;
    free(privic);
    return;
}

/**
 * Set Input Context Data
 *
 * @param call_data
 * @return void
 **/
void XimSetIC(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    FcitxInputContext   *ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);

    if (ic == NULL)
        return;
    FcitxXimIC* rec = (FcitxXimIC*) ic->privateic;
    StoreIC(rec, call_data);

    if (rec->input_style & XIMPreeditCallbacks)
        ic->contextCaps |= CAPACITY_PREEDIT;
    else
        ic->contextCaps &= ~CAPACITY_PREEDIT;

    return;
}

/**
 * Fetch Input Context Data
 *
 * @param call_data
 * @return void
 **/
void XimGetIC(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    XICAttribute   *ic_attr = call_data->ic_attr;
    XICAttribute   *pre_attr = call_data->preedit_attr;
    XICAttribute   *sts_attr = call_data->status_attr;
    register int    i;
    FcitxInputContext *ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
    if (ic == NULL)
        return;
    FcitxXimIC* rec = (FcitxXimIC*) ic->privateic;

    if (rec == NULL)
        return;
    for (i = 0; i < (int) call_data->ic_attr_num; i++, ic_attr++) {
        if (Is(XNFilterEvents, ic_attr)) {
            ic_attr->value = (void *) malloc(sizeof(CARD32));
            *(CARD32 *) ic_attr->value = KeyPressMask | KeyReleaseMask;
            ic_attr->value_length = sizeof(CARD32);
        }
    }

    /* preedit attributes */
    for (i = 0; i < (int) call_data->preedit_attr_num; i++, pre_attr++) {
        if (Is(XNArea, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(XRectangle));
            *(XRectangle *) pre_attr->value = rec->pre_attr.area;
            pre_attr->value_length = sizeof(XRectangle);

        } else if (Is(XNAreaNeeded, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(XRectangle));
            *(XRectangle *) pre_attr->value = rec->pre_attr.area_needed;
            pre_attr->value_length = sizeof(XRectangle);

        } else if (Is(XNSpotLocation, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(XPoint));
            *(XPoint *) pre_attr->value = rec->pre_attr.spot_location;
            pre_attr->value_length = sizeof(XPoint);

        } else if (Is(XNFontSet, pre_attr)) {
            CARD16          base_len = (CARD16) strlen(rec->pre_attr.base_font);
            int             total_len = sizeof(CARD16) + (CARD16) base_len;
            char           *p;

            pre_attr->value = (void *) malloc(total_len);
            p = (char *) pre_attr->value;
            memcpy(p, &base_len, sizeof(CARD16));
            p += sizeof(CARD16);
            strncpy(p, rec->pre_attr.base_font, base_len);
            pre_attr->value_length = total_len;

        } else if (Is(XNForeground, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(long));
            *(long *) pre_attr->value = rec->pre_attr.foreground;
            pre_attr->value_length = sizeof(long);

        } else if (Is(XNBackground, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(long));
            *(long *) pre_attr->value = rec->pre_attr.background;
            pre_attr->value_length = sizeof(long);

        } else if (Is(XNLineSpace, pre_attr)) {
            pre_attr->value = (void *) malloc(sizeof(long));
            *(long *) pre_attr->value = 18;
            pre_attr->value_length = sizeof(long);
        }
    }

    /* status attributes */
    for (i = 0; i < (int) call_data->status_attr_num; i++, sts_attr++) {
        if (Is(XNArea, sts_attr)) {
            sts_attr->value = (void *) malloc(sizeof(XRectangle));
            *(XRectangle *) sts_attr->value = rec->sts_attr.area;
            sts_attr->value_length = sizeof(XRectangle);

        } else if (Is(XNAreaNeeded, sts_attr)) {
            sts_attr->value = (void *) malloc(sizeof(XRectangle));
            *(XRectangle *) sts_attr->value = rec->sts_attr.area_needed;
            sts_attr->value_length = sizeof(XRectangle);

        } else if (Is(XNFontSet, sts_attr)) {
            CARD16          base_len = (CARD16) strlen(rec->sts_attr.base_font);
            int             total_len = sizeof(CARD16) + (CARD16) base_len;
            char           *p;

            sts_attr->value = (void *) malloc(total_len);
            p = (char *) sts_attr->value;
            memcpy(p, &base_len, sizeof(CARD16));
            p += sizeof(CARD16);
            strncpy(p, rec->sts_attr.base_font, base_len);
            sts_attr->value_length = total_len;

        } else if (Is(XNForeground, sts_attr)) {
            sts_attr->value = (void *) malloc(sizeof(long));
            *(long *) sts_attr->value = rec->sts_attr.foreground;
            sts_attr->value_length = sizeof(long);

        } else if (Is(XNBackground, sts_attr)) {
            sts_attr->value = (void *) malloc(sizeof(long));
            *(long *) sts_attr->value = rec->sts_attr.background;
            sts_attr->value_length = sizeof(long);

        } else if (Is(XNLineSpace, sts_attr)) {
            sts_attr->value = (void *) malloc(sizeof(long));
            *(long *) sts_attr->value = 18;
            sts_attr->value_length = sizeof(long);
        }
    }
}

boolean XimCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic)
{
    FCITX_UNUSED(arg);
    FcitxXimIC* ximictoCheck = GetXimIC(icToCheck);
    FcitxXimIC* ximic = GetXimIC(ic);

    return ximictoCheck->connect_id == ximic->connect_id;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;

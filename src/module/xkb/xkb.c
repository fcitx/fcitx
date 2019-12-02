/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include <X11/extensions/XKBstr.h>
#include "fcitx/context.h"
#include "fcitx/hook.h"
#include "fcitx/addon.h"
#include "fcitx/module.h"
#include "module/x11/fcitx-x11.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

#include "config.h"
#include "xkb.h"
#include "xkb-internal.h"
#include "rules.h"
#ifdef _ENABLE_DBUS
#include "module/xkbdbus/fcitx-xkbdbus.h"
#endif


#ifndef XKB_RULES_XML_FILE
#define XKB_RULES_XML_FILE "/usr/share/X11/xkb/rules/evdev.xml"
#endif

#define GROUP_CHANGE_MASK (XkbGroupStateMask | XkbGroupBaseMask |       \
                           XkbGroupLatchMask | XkbGroupLockMask)

DECLARE_ADDFUNCTIONS(Xkb)
static void FcitxXkbGetCurrentLayoutInternal(FcitxXkb *xkb,
                                             char **layout, char **variant);
static void* FcitxXkbCreate(struct _FcitxInstance* instance);
static void FcitxXkbDestroy(void*);
static void FcitxXkbReloadConfig(void*);
static void FcitxXkbInitDefaultLayout (FcitxXkb* xkb);
static Bool FcitxXkbSetRules(FcitxXkb* xkb, const char *rules_file,
                             const char *model, const char *all_layouts,
                             const char *all_variants, const char *all_options);
static Bool FcitxXkbUpdateProperties(FcitxXkb* xkb, const char *rules_file,
                                     const char *model, const char *all_layouts,
                                     const char *all_variants,
                                     const char *all_options);
static void FcitxXkbApplyCustomScript(FcitxXkb* xkb);


static inline boolean IMIsKeyboardIM(const char* imname) {
    return strncmp(imname, "fcitx-keyboard-",
                   strlen("fcitx-keyboard-")) == 0;
}

static void ExtractKeyboardIMLayout(const char* imname, char** layout, char** variant) {
    if (!IMIsKeyboardIM(imname)) {
        return;
    }

    imname += strlen("fcitx-keyboard-");
    char* v = strchr(imname, '-');
    if (v) {
        *layout = strndup(imname, v - imname);
        v++;
        *variant = strdup(v);
    } else {
        *layout = strdup(imname);
    }
}

#if 0
static char* FcitxXkbGetCurrentLayout(FcitxXkb* xkb);
static char* FcitxXkbGetCurrentModel(FcitxXkb* xkb);
static char* FcitxXkbGetCurrentOption(FcitxXkb* xkb);
#endif
static boolean
FcitxXkbSetLayoutByName(FcitxXkb *xkb,
                        const char *layout, const char *variant, boolean toDefault);
static void
FcitxXkbRetrieveCloseGroup(FcitxXkb *xkb);
static void FcitxXkbSetLayout  (FcitxXkb* xkb,
                                const char *layouts,
                                const char *variants,
                                const char *options);
static int FcitxXkbGetCurrentGroup (FcitxXkb* xkb);
static void FcitxXkbIMKeyboardLayoutChanged(void* arg, const void* value);
static int FcitxXkbFindLayoutIndex(FcitxXkb* xkb, const char* layout, const char* variant);
static void FcitxXkbCurrentStateChanged(void* arg);
static void FcitxXkbCurrentStateChangedTriggerOn(void* arg);
static char* FcitxXkbGetRulesName(FcitxXkb* xkb);
static char* FcitxXkbFindXkbRulesFile(FcitxXkb* xkb);
static boolean LoadXkbConfig(FcitxXkb* xkb);
static void SaveXkbConfig(FcitxXkb* fs);
static boolean FcitxXkbEventHandler(void* arg, XEvent* event);

CONFIG_DESC_DEFINE(GetXkbConfigDesc, "fcitx-xkb.desc")

typedef struct _LayoutOverride {
    char* im;
    char* layout;
    char* variant;
    UT_hash_handle hh;
} LayoutOverride;

FCITX_DEFINE_PLUGIN(fcitx_xkb, module, FcitxModule) = {
    FcitxXkbCreate,
    NULL,
    NULL,
    FcitxXkbDestroy,
    FcitxXkbReloadConfig
};

boolean FcitxXkbSupported(FcitxXkb* xkb, int* xkbOpcode)
{
    // Verify the Xlib has matching XKB extension.

    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;

    if (!XkbLibraryVersion(&major, &minor)) {
        FcitxLog(WARNING, "Xlib XKB extension %d.%d != %d %d",
                 major, minor, XkbMajorVersion, XkbMinorVersion);
        return false;
    }

    // Verify the X server has matching XKB extension.

    int opcode_rtrn;
    int error_rtrn;
    int xkb_opcode;
    if (!XkbQueryExtension(xkb->dpy, &opcode_rtrn, &xkb_opcode,
                           &error_rtrn, &major, &minor)) {
        FcitxLog(WARNING, "Xlib XKB extension %d.%d != %d %d",
                 major, minor, XkbMajorVersion, XkbMinorVersion);
        return false;
    }

    if (xkbOpcode != NULL) {
        *xkbOpcode = xkb_opcode;
    }

    return true;
}

static void FcitxXkbFixInconsistentLayoutVariant(FcitxXkb *xkb) {
    while (utarray_len(xkb->defaultVariants) <
           utarray_len(xkb->defaultLayouts)) {
        const char* dummy = "";
        utarray_push_back(xkb->defaultVariants, &dummy);
    }

    while (utarray_len(xkb->defaultVariants) >
           utarray_len(xkb->defaultLayouts)) {
        utarray_pop_back(xkb->defaultVariants);
    }
}

static inline void
FcitxXkbClearVarDefsRec(XkbRF_VarDefsRec *vdp)
{
    fcitx_utils_free(vdp->model);
    fcitx_utils_free(vdp->layout);
    fcitx_utils_free(vdp->variant);
    fcitx_utils_free(vdp->options);
}

static char* FcitxXkbGetRulesName(FcitxXkb* xkb)
{
    XkbRF_VarDefsRec vd;
    char *tmp = NULL;

    /* the four strings in vd are either %NULL or strdup'ed */
    if (XkbRF_GetNamesProp(xkb->dpy, &tmp, &vd)) {
        FcitxXkbClearVarDefsRec(&vd);
        return tmp;
    }
    return NULL;
}


static char* FcitxXkbFindXkbRulesFile(FcitxXkb* xkb)
{
    char *rulesFile = NULL;
    char *rulesName = FcitxXkbGetRulesName(xkb);

    if (rulesName) {
        if (rulesName[0] == '/') {
            fcitx_utils_alloc_cat_str(rulesFile, rulesName, ".xml");
        } else {
            fcitx_utils_alloc_cat_str(rulesFile, XKEYBOARDCONFIG_XKBBASE,
                                      "/rules/", rulesName, ".xml");
        }
        free(rulesName);
    } else {
        return strdup(XKB_RULES_XML_FILE);
    }
    return rulesFile;
}

UT_array*
splitAndKeepEmpty(UT_array *list,
                  const char* str, const char *delm)
{
    const char *lastPos, *pos;
    lastPos = str;
    // even lastPos points to '\0" it will return something meaningful.
    pos = lastPos + strcspn(lastPos, delm);

    while (*pos || *lastPos) {
        fcitx_utils_string_list_append_len(list, lastPos, pos - lastPos);
        if (*pos == '\0') {
            break;
        }
        lastPos = pos + 1;
        pos = lastPos + strcspn(lastPos, delm);
    }
    return list;
}

static void
FcitxXkbInitDefaultLayout(FcitxXkb* xkb)
{
    Display* dpy = xkb->dpy;
    XkbRF_VarDefsRec vd;

    utarray_clear(xkb->defaultLayouts);
    utarray_clear(xkb->defaultModels);
    utarray_clear(xkb->defaultOptions);
    utarray_clear(xkb->defaultVariants);

    /* XkbRF_GetNamesProp won't fill the second parameter if it is NULL */
    if (!XkbRF_GetNamesProp(dpy, NULL, &vd)) {
        FcitxLog(WARNING, "Couldn't interpret %s property",
                 _XKB_RF_NAMES_PROP_ATOM);
        return;
    }
    /* Print warning only, in order to free vd correctly. */
    if (!vd.model || !vd.layout)
        FcitxLog(WARNING, "Could not get group layout from X property");
    if (vd.layout) {
        splitAndKeepEmpty(xkb->defaultLayouts, vd.layout, ",");
    }
    if (vd.model) {
        splitAndKeepEmpty(xkb->defaultModels, vd.model, ",");
    }
    if (vd.options) {
        splitAndKeepEmpty(xkb->defaultOptions, vd.options, ",");
    }
    if (vd.variant) {
        splitAndKeepEmpty(xkb->defaultVariants, vd.variant, ",");
    }

    FcitxXkbFixInconsistentLayoutVariant(xkb);

    FcitxXkbClearVarDefsRec(&vd);
}

static Bool
FcitxXkbSetRules(FcitxXkb* xkb, const char *rules_file, const char *model,
                 const char *all_layouts, const char *all_variants,
                 const char *all_options)
{
    Display *dpy = xkb->dpy;
    char *prefix;
    XkbRF_RulesPtr rules = NULL;
    XkbRF_VarDefsRec rdefs;
    XkbComponentNamesRec rnames;
    XkbDescPtr xkbDesc;

    if (!rules_file)
        return False;

    if (rules_file[0] != '/') {
        prefix = "./rules/";
    } else {
        prefix = "";
    }
    char *rules_path1;
    fcitx_utils_alloc_cat_str(rules_path1, prefix, rules_file);
    rules = XkbRF_Load(rules_path1, "C", True, True);
    free(rules_path1);
    if (rules == NULL) {
        char *rulesPath = FcitxXkbFindXkbRulesFile(xkb);
        size_t rulesBaseLen = strlen(rulesPath) - strlen(".xml");
        if (strlen(rulesPath) > strlen(".xml") && strcmp(rulesPath + rulesBaseLen, ".xml") == 0) {
            rulesPath[rulesBaseLen] = '\0';
        }
        rules = XkbRF_Load(rulesPath, "C", True, True);
        free(rulesPath);
    }
    if (rules == NULL)
        return False;

    memset(&rdefs, 0, sizeof(XkbRF_VarDefsRec));
    memset(&rnames, 0, sizeof(XkbComponentNamesRec));
    rdefs.model = model ? strdup(model) : NULL;
    rdefs.layout = all_layouts ? strdup(all_layouts) : NULL;
    rdefs.variant = all_variants && all_variants[0] ? strdup(all_variants) : NULL;
    rdefs.options = all_options && all_options[0] ? strdup(all_options) : NULL;
    XkbRF_GetComponents(rules, &rdefs, &rnames);
    xkbDesc = XkbGetKeyboardByName(dpy, XkbUseCoreKbd, &rnames,
                                   XkbGBN_AllComponentsMask,
                                   XkbGBN_AllComponentsMask &
                                   (~XkbGBN_GeometryMask), True);

    XkbRF_Free(rules, True);
    free(rnames.keymap);
    free(rnames.keycodes);
    free(rnames.types);
    free(rnames.compat);
    free(rnames.symbols);
    free(rnames.geometry);

    Bool result = True;
    if (!xkbDesc) {
        FcitxLog (WARNING, "Cannot load new keyboard description.");
        result = False;
    }
    else {
        char* tempstr = strdup(rules_file);
        XkbRF_SetNamesProp(dpy, tempstr, &rdefs);
        free (tempstr);
        XkbFreeKeyboard(xkbDesc, XkbGBN_AllComponentsMask, True);
    }
    free(rdefs.model);
    free(rdefs.layout);
    free(rdefs.variant);
    free(rdefs.options);

    return result;
}

static Bool
FcitxXkbUpdateProperties(FcitxXkb* xkb, const char *rules_file,
                         const char *model, const char *all_layouts,
                         const char *all_variants, const char *all_options)
{
    Display *dpy = xkb->dpy;
    int len;
    char *pval;
    char *next;
    static Atom rules_atom = None;
    Window root_window;

    len = (rules_file ? strlen (rules_file) : 0);
    len += (model ? strlen (model) : 0);
    len += (all_layouts ? strlen (all_layouts) : 0);
    len += (all_variants ? strlen (all_variants) : 0);
    len += (all_options ? strlen (all_options) : 0);

    if (len < 1) {
        return True;
    }
    len += 5; /* trailing NULs */

    if (rules_atom == None)
        rules_atom = XInternAtom (dpy, _XKB_RF_NAMES_PROP_ATOM, False);
    root_window = XDefaultRootWindow (dpy);
    pval = next = (char*) fcitx_utils_malloc0 (sizeof(char) *(len + 1));
    if (!pval) {
        return True;
    }

    if (rules_file) {
        strcpy (next, rules_file);
        next += strlen (rules_file);
    }
    *next++ = '\0';
    if (model) {
        strcpy (next, model);
        next += strlen (model);
    }
    *next++ = '\0';
    if (all_layouts) {
        strcpy (next, all_layouts);
        next += strlen (all_layouts);
    }
    *next++ = '\0';
    if (all_variants) {
        strcpy (next, all_variants);
        next += strlen (all_variants);
    }
    *next++ = '\0';
    if (all_options) {
        strcpy (next, all_options);
        next += strlen (all_options);
    }
    *next++ = '\0';
    if ((next - pval) == len) {
        XChangeProperty (dpy, root_window,
                        rules_atom, XA_STRING, 8, PropModeReplace,
                        (unsigned char *) pval, len);
    }

    free(pval);
    return True;
}

static void
FcitxXkbGetCurrentLayoutInternal(FcitxXkb *xkb, char **layout, char **variant)
{
    int cur_group = FcitxXkbGetCurrentGroup(xkb);
    char* const *layoutName = fcitx_array_eltptr(xkb->defaultLayouts,
                                                 cur_group);
    char* const *pVariantName = fcitx_array_eltptr(xkb->defaultVariants,
                                                   cur_group);
    if (layoutName)
        *layout = strdup(*layoutName);
    else
        *layout = NULL;
    if (pVariantName && strlen(*pVariantName) != 0)
        *variant = strdup(*pVariantName);
    else
        *variant = NULL;
}

#if 0

static char*
FcitxXkbGetCurrentModel(FcitxXkb* xkb)
{
    if (xkb->defaultModels == NULL) {
        return NULL;
    }
    return fcitx_utils_join_string_list(xkb->defaultModels, ',');
}

static char*
FcitxXkbGetCurrentOption(FcitxXkb* xkb)
{
    if (xkb->defaultOptions == NULL) {
        return NULL;
    }
    return fcitx_utils_join_string_list(xkb->defaultOptions, ',');
}
#endif

/* This SHOULD be _SetLayouts_ .... */
static void
FcitxXkbSetLayout(FcitxXkb* xkb, const char *layouts,
                  const char *variants, const char *options)
{
    char *layouts_line;
    char *options_line;
    char *variants_line;
    char *model_line;

    if (utarray_len(xkb->defaultLayouts) == 0) {
        FcitxLog(WARNING, "Your system seems not to support XKB.");
        return;
    }

    if (layouts == NULL)
        layouts_line = fcitx_utils_join_string_list(xkb->defaultLayouts, ',');
    else
        layouts_line = strdup(layouts);

    if (variants == NULL)
        variants_line = fcitx_utils_join_string_list(xkb->defaultVariants, ',');
    else
        variants_line = strdup(variants);

    if (options == NULL)
        options_line = fcitx_utils_join_string_list(xkb->defaultOptions, ',');
    else
        options_line = strdup(options);

    model_line = fcitx_utils_join_string_list(xkb->defaultModels, ',');

    char* rulesName = FcitxXkbGetRulesName(xkb);
    if (rulesName) {
        if (FcitxXkbSetRules(xkb,
                             rulesName, model_line,
                             layouts_line, variants_line, options_line)) {
            FcitxXkbUpdateProperties(xkb,
                                     rulesName, model_line,
                                     layouts_line, variants_line, options_line);
            xkb->waitingForRefresh = true;
        }
        free(rulesName);
    }
    free(layouts_line);
    free(variants_line);
    free(options_line);
    free(model_line);
}

static int
FcitxXkbGetCurrentGroup(FcitxXkb* xkb)
{
    Display *dpy = xkb->dpy;
    XkbStateRec state;

    if (utarray_len(xkb->defaultLayouts) == 0) {
        FcitxLog(WARNING, "Your system seems not to support XKB.");
        return 0;
    }

    if (XkbGetState(dpy, XkbUseCoreKbd, &state) != Success) {
        FcitxLog(WARNING, "Could not get state");
        return 0;
    }

    return state.group;
}

static void FcitxXkbAddNewLayout(FcitxXkb* xkb, const char* layoutString,
                                 const char* variantString, boolean toDefault, int index)
{
    if (!layoutString)
        return;

    FcitxXkbFixInconsistentLayoutVariant(xkb);

    if (toDefault) {
        if (index == 0) {
            return;
        }
        if (index > 0) {
            utarray_remove_quick_full(xkb->defaultLayouts, index);
            utarray_remove_quick_full(xkb->defaultVariants, index);
        }
        utarray_push_front(xkb->defaultLayouts, &layoutString);
        if (variantString) {
            utarray_push_front(xkb->defaultVariants, &variantString);
        } else {
            const char *dummy = "";
            utarray_push_front(xkb->defaultVariants, &dummy);
        }
    } else {
        while (utarray_len(xkb->defaultVariants) >= 4) {
            utarray_pop_back(xkb->defaultVariants);
            utarray_pop_back(xkb->defaultLayouts);
        }
        utarray_push_back(xkb->defaultLayouts, &layoutString);
        if (variantString) {
            utarray_push_back(xkb->defaultVariants, &variantString);
        } else {
            const char *dummy = "";
            utarray_push_back(xkb->defaultVariants, &dummy);
        }
    }
    FcitxXkbSetLayout(xkb, NULL, NULL, NULL);
}

static int
FcitxXkbFindOrAddLayout(FcitxXkb *xkb, const char *layout, const char *variant, boolean toDefault)
{
    int index;
    if (layout == NULL)
        return -1;
    index = FcitxXkbFindLayoutIndex(xkb, layout, variant);
    if (!xkb->config.bOverrideSystemXKBSettings)
        return index;
    if (!(index < 0 || (index > 0 && toDefault)))
        return index;
    if (!xkb->blockOverride) {
        FcitxXkbAddNewLayout(xkb, layout, variant, toDefault, index);
    }
    FcitxXkbInitDefaultLayout(xkb);
    return FcitxXkbFindLayoutIndex(xkb, layout, variant);
}

static boolean
FcitxXkbSetLayoutByName(FcitxXkb *xkb, const char *layout, const char *variant, boolean toDefault)
{
    int index;
    index = FcitxXkbFindOrAddLayout(xkb, layout, variant, toDefault);
    if (index < 0) {
        return false;
    }
#ifdef _ENABLE_DBUS
    // xkbdbus should be gnone at this point, don't touch the code.
    if (FcitxInstanceGetIsDestroying(xkb->owner)) {
        XkbLockGroup(xkb->dpy, XkbUseCoreKbd, index);
        return false;
    }
    FcitxAddon *addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(xkb->owner), "fcitx-xkbdbus");
    if (!addon || !addon->addonInstance || !FcitxXkbDBusLockGroupByHelper(xkb->owner, index))
#endif
    {
        XkbLockGroup(xkb->dpy, XkbUseCoreKbd, index);
    }
    return true;
}

static void
FcitxXkbRetrieveCloseGroup(FcitxXkb *xkb)
{
    LayoutOverride* item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, "default", item);
    if (item) {
        FcitxXkbSetLayoutByName(xkb, item->layout, item->variant, true);
    } else {
        do {
            if (!xkb->config.useFirstKeyboardIMAsDefaultLayout) {
                break;
            }
            char *layout = NULL, *variant = NULL;

            UT_array* imes = FcitxInstanceGetIMEs(xkb->owner);
            if (utarray_len(imes) == 0) {
                break;
            }

            FcitxIM* im = (FcitxIM*) utarray_front(imes);
            ExtractKeyboardIMLayout(im->uniqueName, &layout, &variant);
            if (!layout) {
                break;
            }
            FcitxXkbSetLayoutByName(xkb, layout, variant, true);
            free(layout);
            free(variant);
            return;
        } while(0);
        FcitxXkbSetLayoutByName(xkb, xkb->closeLayout, xkb->closeVariant, true);
    }
}

static void FcitxXkbIMKeyboardLayoutChanged(void* arg, const void* value)
{
    /* fcitx-keyboard won't be able to set layout,
     * if fcitx-xkb doesn't change the layout for it. */
    FcitxXkb *xkb = (FcitxXkb*) arg;
    FcitxIM *currentIM = FcitxInstanceGetCurrentIM(xkb->owner);

    /* active means im will be take care */
    if (FcitxInstanceGetCurrentStatev2(xkb->owner) == IS_ACTIVE) {
        char *layoutToFree = NULL, *variantToFree = NULL;
        char* layoutString = NULL;
        char* variantString = NULL;
        LayoutOverride* item = NULL;
        UT_array* s = NULL;
        if (currentIM)
            HASH_FIND_STR(xkb->layoutOverride, currentIM->uniqueName, item);
        if (item) {
            layoutString = item->layout;
            variantString = item->variant;
        }
        else {

            do {
                if (!xkb->config.useFirstKeyboardIMAsDefaultLayout) {
                    break;
                }
                if (currentIM && IMIsKeyboardIM(currentIM->uniqueName)) {
                    break;
                }

                UT_array* imes = FcitxInstanceGetIMEs(xkb->owner);
                if (utarray_len(imes) == 0) {
                    break;
                }

                FcitxIM* im = (FcitxIM*) utarray_front(imes);
                ExtractKeyboardIMLayout(im->uniqueName, &layoutToFree, &variantToFree);
                if (!layoutToFree) {
                    break;
                }

                layoutString = layoutToFree;
                variantString = variantToFree;

            } while(0);

            if (!layoutString) {
                const char* layoutVariantString = (const char*) value;
                if (layoutVariantString) {
                    s = fcitx_utils_split_string(layoutVariantString, ',');
                    char  **pLayoutString = (char**)utarray_eltptr(s, 0);
                    char **pVariantString = (char**)utarray_eltptr(s, 1);
                    layoutString = (pLayoutString)? *pLayoutString: NULL;
                    variantString = (pVariantString)? *pVariantString: NULL;
                }
                else {
                    layoutString = NULL;
                    variantString = NULL;
                }
            }
        }
        if (layoutString) {
            if (!FcitxXkbSetLayoutByName(xkb, layoutString, variantString, false)) {
                FcitxXkbRetrieveCloseGroup(xkb);
            }
        }
        if (s) {
            fcitx_utils_free_string_list(s);
        }

        fcitx_utils_free(layoutToFree);
        fcitx_utils_free(variantToFree);
    } else {
        FcitxXkbRetrieveCloseGroup(xkb);
    }
}

static int FcitxXkbFindLayoutIndex(FcitxXkb* xkb, const char* layout, const char* variant)
{
    char** layoutName;
    char* variantName, **pVariantName;
    if (layout == NULL)
        return -1;

    unsigned int i = 0;
    for (i = 0;i < utarray_len(xkb->defaultLayouts);i++) {
        layoutName = (char**)utarray_eltptr(xkb->defaultLayouts, i);
        pVariantName = (char**)utarray_eltptr(xkb->defaultVariants, i);
        variantName = pVariantName ? *pVariantName : NULL;
        if (strcmp(*layoutName, layout) == 0 &&
            fcitx_utils_strcmp_empty(variantName, variant) == 0) {
            return i;
        }
    }
    return -1;
}

static void
FcitxXkbSaveCloseGroup(FcitxXkb *xkb)
{
    char *tmplayout;
    char *tmpvariant;
    FcitxXkbGetCurrentLayoutInternal(xkb, &tmplayout, &tmpvariant);
    if (!tmplayout) {
        fcitx_utils_free(tmpvariant);
        return;
    }
    fcitx_utils_free(xkb->closeLayout);
    fcitx_utils_free(xkb->closeVariant);
    xkb->closeLayout = tmplayout;
    xkb->closeVariant = tmpvariant;
    FcitxXkbRetrieveCloseGroup(xkb);
}

static void
FcitxXkbGetDefaultXmodmap(FcitxXkb *xkb)
{
    static char *home = NULL;
    if (fcitx_likely(xkb->defaultXmodmapPath))
        return;
    if (fcitx_likely(!home)) {
        home = getenv("HOME");
        if (fcitx_unlikely(!home)) {
            return;
        }
    }
    fcitx_utils_alloc_cat_str(xkb->defaultXmodmapPath, home, "/.Xmodmap");
}

static void* FcitxXkbCreate(FcitxInstance* instance)
{
    FcitxXkb *xkb = fcitx_utils_new(FcitxXkb);
    xkb->owner = instance;
    do {
        xkb->dpy = FcitxX11GetDisplay(xkb->owner);
        if (!xkb->dpy)
            break;

        if (!FcitxXkbSupported(xkb, &xkb->xkbOpcode))
            break;

        if (!LoadXkbConfig(xkb)) {
            break;
        }

        char* rulesPath = FcitxXkbFindXkbRulesFile(xkb);
        xkb->rules = FcitxXkbReadRules(rulesPath);
        free(rulesPath);

        xkb->defaultLayouts = fcitx_utils_new_string_list();
        xkb->defaultModels = fcitx_utils_new_string_list();
        xkb->defaultOptions = fcitx_utils_new_string_list();
        xkb->defaultVariants = fcitx_utils_new_string_list();

        FcitxXkbInitDefaultLayout(xkb);
        FcitxXkbSaveCloseGroup(xkb);

        int eventMask = XkbNewKeyboardNotifyMask | XkbStateNotifyMask;
        XkbSelectEvents(xkb->dpy, XkbUseCoreKbd, eventMask, eventMask);

        FcitxX11AddXEventHandler(xkb->owner, FcitxXkbEventHandler, xkb);

        FcitxInstanceWatchContext(instance, CONTEXT_IM_KEYBOARD_LAYOUT,
                                  FcitxXkbIMKeyboardLayoutChanged, xkb);

        FcitxIMEventHook hk;
        hk.arg = xkb;
        hk.func = FcitxXkbCurrentStateChanged;
        FcitxInstanceRegisterInputFocusHook(instance, hk);
        FcitxInstanceRegisterInputUnFocusHook(instance, hk);
        FcitxInstanceRegisterTriggerOffHook(instance, hk);

        hk.arg = xkb;
        hk.func = FcitxXkbCurrentStateChangedTriggerOn;
        FcitxInstanceRegisterTriggerOnHook(instance, hk);

        FcitxXkbAddFunctions(instance);
        if (xkb->config.bOverrideSystemXKBSettings)
            FcitxXkbSetLayout(xkb, NULL, NULL, NULL);
        return xkb;
    } while (0);

    free(xkb);
    return NULL;
}

static void FcitxXkbScheduleRefresh(void* arg) {
    FcitxXkb* xkb = (FcitxXkb*) arg;
    FcitxUIUpdateInputWindow(xkb->owner);
    FcitxXkbInitDefaultLayout(xkb);
    // we shall now ignore all outside world change, apply only if we do it on our own
    xkb->blockOverride = true;
    FcitxXkbCurrentStateChanged(xkb);
    if (xkb->waitingForRefresh) {
        xkb->waitingForRefresh = false;
        FcitxXkbApplyCustomScript(xkb);
    }
    xkb->blockOverride = false;
}

static boolean FcitxXkbEventHandler(void* arg, XEvent* event)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;

    if (event->type == xkb->xkbOpcode) {
        XkbEvent *xkbEvent = (XkbEvent*) event;

        if (xkbEvent->any.xkb_type == XkbStateNotify &&
            xkbEvent->state.changed & GROUP_CHANGE_MASK) {
            if (xkb->config.useFirstKeyboardIMAsDefaultLayout
                && FcitxInstanceGetCurrentStatev2(xkb->owner) != IS_ACTIVE) {
                FcitxXkbSaveCloseGroup(xkb);
            }
        }

        if (xkbEvent->any.xkb_type == XkbNewKeyboardNotify
            && xkbEvent->new_kbd.serial != xkb->lastSerial
        ) {
            xkb->lastSerial = xkbEvent->new_kbd.serial;
            FcitxInstanceRemoveTimeoutByFunc(xkb->owner, FcitxXkbScheduleRefresh);
            FcitxInstanceAddTimeout(xkb->owner, 10, FcitxXkbScheduleRefresh, xkb);
        }
        return true;
    }
    return false;
}

static void FcitxXkbCurrentStateChanged(void* arg)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    const char* layout = FcitxInstanceGetContextString(xkb->owner, CONTEXT_IM_KEYBOARD_LAYOUT);
    FcitxXkbIMKeyboardLayoutChanged(xkb, layout);
}

static void FcitxXkbCurrentStateChangedTriggerOn(void* arg)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    FcitxXkbSaveCloseGroup(xkb);
    FcitxXkbCurrentStateChanged(arg);
}

static void FcitxXkbDestroy(void* arg)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    if (xkb->config.bOverrideSystemXKBSettings)
        FcitxXkbSetLayout(xkb, NULL, NULL, NULL);
    FcitxXkbRetrieveCloseGroup(xkb);
    XSync(xkb->dpy, False);
    FcitxXkbRulesFree(xkb->rules);
    fcitx_utils_free_string_list(xkb->defaultVariants);
    fcitx_utils_free_string_list(xkb->defaultLayouts);
    fcitx_utils_free_string_list(xkb->defaultModels);
    fcitx_utils_free_string_list(xkb->defaultOptions);
    fcitx_utils_free(xkb->closeLayout);
    fcitx_utils_free(xkb->closeVariant);
    fcitx_utils_free(xkb->defaultXmodmapPath);

    FcitxConfigFree(&xkb->config.gconfig);
    free(xkb);
}

static void FcitxXkbReloadConfig(void* arg)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    LoadXkbConfig(xkb);
    FcitxXkbCurrentStateChanged(xkb);
    if (xkb->config.bOverrideSystemXKBSettings)
        FcitxXkbSetLayout(xkb, NULL, NULL, NULL);
}

static void FcitxXkbApplyCustomScript(FcitxXkb* xkb)
{
    FcitxXkbConfig *config = &xkb->config;
    if (!config->bOverrideSystemXKBSettings || !config->xmodmapCommand ||
        !config->xmodmapCommand[0])
        return;

    char *to_free = NULL;
    char *xmodmap_script = NULL;
    if (!config->customXModmapScript || !config->customXModmapScript[0]) {
        if (strcmp(config->xmodmapCommand, "xmodmap") == 0) {
            FcitxXkbGetDefaultXmodmap(xkb);
            if (!(xkb->defaultXmodmapPath &&
                  fcitx_utils_isreg(xkb->defaultXmodmapPath)))
                return;
            xmodmap_script = xkb->defaultXmodmapPath;
        }
    } else {
        FcitxXDGGetFileUserWithPrefix("data", config->customXModmapScript,
                                      NULL, &to_free);
        xmodmap_script = to_free;
    }

    char* args[] = {
        config->xmodmapCommand,
        xmodmap_script,
        NULL
    };
    fcitx_utils_start_process(args);
    fcitx_utils_free(to_free);
}

static inline void LayoutOverrideFree(LayoutOverride* item) {
    fcitx_utils_free(item->im);
    fcitx_utils_free(item->layout);
    fcitx_utils_free(item->variant);
    free(item);
}

void LoadLayoutOverride(FcitxXkb* xkb)
{
    char *buf = NULL;
    size_t len = 0;
    char *trimedbuf = NULL;

    /* clean all old configuration */
    while (xkb->layoutOverride) {
        LayoutOverride *cur = xkb->layoutOverride;
        HASH_DEL(xkb->layoutOverride, cur);
        LayoutOverrideFree(cur);
    }

    FILE *fp = FcitxXDGGetFileUserWithPrefix("data", "layout_override",
                                             "r", NULL);
    if (!fp)
        return;

    while (getline(&buf, &len, fp) != -1) {
        if (trimedbuf)
            free(trimedbuf);
        trimedbuf = fcitx_utils_trim(buf);

        UT_array* s = fcitx_utils_split_string(trimedbuf, ',');
        char** pIMString = (char**)utarray_eltptr(s, 0);
        char** pLayoutString = (char**)utarray_eltptr(s, 1);
        char** pVariantString = (char**)utarray_eltptr(s, 2);
        char* imString = (pIMString)? *pIMString: NULL;
        char* layoutString = (pLayoutString)? *pLayoutString: NULL;
        char* variantString = (pVariantString)? *pVariantString: NULL;

        if (!imString || !layoutString)
            continue;
        LayoutOverride* override = NULL;
        HASH_FIND_STR(xkb->layoutOverride, imString, override);
        /* if rule exists or the rule is for fcitx-keyboard, skip it */
        if (override || IMIsKeyboardIM(imString)) {
            continue;
        }

        override = fcitx_utils_new(LayoutOverride);
        override->im = strdup(imString);
        override->layout = strdup(layoutString);
        override->variant= variantString? strdup(variantString) : NULL;

        HASH_ADD_KEYPTR(hh, xkb->layoutOverride,
                        override->im, strlen(override->im), override);
    }

    if (buf)
        free(buf);
    if (trimedbuf)
        free(trimedbuf);

    fclose(fp);
}

boolean LoadXkbConfig(FcitxXkb* xkb)
{
    FcitxConfigFileDesc *configDesc = GetXkbConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-xkb.config",
                                             "r", NULL);

    if (!fp) {
        if (errno == ENOENT) {
            SaveXkbConfig(xkb);
        }
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
    FcitxXkbConfigConfigBind(&xkb->config, cfile, configDesc);
    FcitxConfigBindSync(&xkb->config.gconfig);

    if (fp)
        fclose(fp);

    LoadLayoutOverride(xkb);

    return true;
}

void SaveLayoutOverride(FcitxXkb* xkb)
{
    FILE *fp = FcitxXDGGetFileUserWithPrefix("data", "layout_override", "w", NULL);
    if (!fp)
        return;

    LayoutOverride* item = xkb->layoutOverride;
    while(item) {
        if (item->variant)
            fprintf(fp, "%s,%s,%s\n", item->im, item->layout, item->variant);
        else
            fprintf(fp, "%s,%s\n", item->im, item->layout);
        item = item->hh.next;
    }

    fclose(fp);
}

void SaveXkbConfig(FcitxXkb* xkb)
{
    FcitxConfigFileDesc *configDesc = GetXkbConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-xkb.config", "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &xkb->config.gconfig, configDesc);
    if (fp)
        fclose(fp);

    SaveLayoutOverride(xkb);
}

static void
FcitxXkbGetLayoutOverride(FcitxXkb *xkb, const char *imname, char **layout,
                          char **variant)
{
    LayoutOverride *item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, imname, item);
    if (item) {
        *layout = item->layout;
        *variant = item->variant;
    } else {
        *layout = NULL;
        *variant = NULL;
    }
}

static void
FcitxXkbSetLayoutOverride(FcitxXkb *xkb, const char *imname, const char *layout,
                          const char* variant)
{
    LayoutOverride *item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, imname, item);
    if (item) {
        HASH_DEL(xkb->layoutOverride, item);
        LayoutOverrideFree(item);
    }

    if (layout && layout[0] != '\0' &&
        !IMIsKeyboardIM(imname)) {
        item = fcitx_utils_new(LayoutOverride);
        item->im = strdup(imname);
        item->layout = strdup(layout);
        item->variant= (variant && variant[0] != '\0')? strdup(variant) : NULL;
        HASH_ADD_KEYPTR(hh, xkb->layoutOverride, item->im,
                        strlen(item->im), item);
    }
    SaveLayoutOverride(xkb);
    FcitxXkbCurrentStateChanged(xkb);
}

static void
FcitxXkbSetDefaultLayout(FcitxXkb *xkb, const char *layout, const char *variant)
{
    LayoutOverride *item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, "default", item);
    if (item) {
        HASH_DEL(xkb->layoutOverride, item);
        LayoutOverrideFree(item);
    }

    if (layout && layout[0] != '\0') {
        item = fcitx_utils_new(LayoutOverride);
        item->im = strdup("default");
        item->layout = strdup(layout);
        item->variant= (variant && variant[0] != '\0')? strdup(variant) : NULL;
        HASH_ADD_KEYPTR(hh, xkb->layoutOverride, item->im,
                        strlen(item->im), item);
    }
    SaveLayoutOverride(xkb);
    FcitxXkbCurrentStateChanged(xkb);
}

#include "fcitx-xkb-addfunctions.h"

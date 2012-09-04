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
#include "module/x11/x11stuff.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

#include "config.h"
#include "xkb.h"
#include "xkb-internal.h"
#include "rules.h"

#ifndef XKB_RULES_XML_FILE
#define XKB_RULES_XML_FILE "/usr/share/X11/xkb/rules/evdev.xml"
#endif

#define GROUP_CHANGE_MASK \
    ( XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask )

static void* FcitxXkbGetRules(void* arg, FcitxModuleFunctionArg args);
static void* FcitxXkbLayoutExists(void* arg, FcitxModuleFunctionArg args);
static void FcitxXkbGetCurrentLayoutInternal(FcitxXkb *xkb,
                                             char **layout, char **variant);
static void* FcitxXkbGetCurrentLayout(void* arg, FcitxModuleFunctionArg args);
static void* FcitxXkbGetLayoutOverride(void* arg, FcitxModuleFunctionArg args);
static void* FcitxXkbSetLayoutOverride(void* arg, FcitxModuleFunctionArg args);
static void* FcitxXkbSetDefaultLayout(void* arg, FcitxModuleFunctionArg args);
static void* FcitxXkbCreate(struct _FcitxInstance* instance);
static void FcitxXkbDestroy(void*);
static void FcitxXkbReloadConfig(void*);
static void FcitxXkbInitDefaultLayout (FcitxXkb* xkb);
static Bool FcitxXkbSetRules (FcitxXkb* xkb,
             const char *rules_file, const char *model,
             const char *all_layouts, const char *all_variants,
             const char *all_options);
static Bool FcitxXkbUpdateProperties (FcitxXkb* xkb,
                     const char *rules_file, const char *model,
                     const char *all_layouts, const char *all_variants,
                     const char *all_options);
static void FcitxXkbApplyCustomScript(FcitxXkb* xkb);

#if 0
static char* FcitxXkbGetCurrentLayout (FcitxXkb* xkb);
static char* FcitxXkbGetCurrentModel (FcitxXkb* xkb);
static char* FcitxXkbGetCurrentOption (FcitxXkb* xkb);
#endif
static boolean
FcitxXkbSetLayoutByName(FcitxXkb *xkb,
                        const char *layout, const char *variant);
static void
FcitxXkbRetrieveCloseGroup(FcitxXkb *xkb);
static boolean FcitxXkbSetLayout  (FcitxXkb* xkb,
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

    if (!XkbLibraryVersion(&major, &minor))
    {
        FcitxLog(WARNING, "Xlib XKB extension %d.%d != %d %d", major, minor, XkbMajorVersion, XkbMinorVersion);
        return false;
    }

    // Verify the X server has matching XKB extension.

    int opcode_rtrn;
    int error_rtrn;
    int xkb_opcode;
    if( ! XkbQueryExtension(xkb->dpy, &opcode_rtrn, &xkb_opcode, &error_rtrn, &major, &minor)) {
        FcitxLog(WARNING, "Xlib XKB extension %d.%d != %d %d", major, minor, XkbMajorVersion, XkbMinorVersion);
        return false;
    }

    if( xkbOpcode != NULL ) {
        *xkbOpcode = xkb_opcode;
    }

    return true;
}


static char* FcitxXkbGetRulesName(FcitxXkb* xkb)
{
    XkbRF_VarDefsRec vd;
    char *tmp = NULL;

    if (XkbRF_GetNamesProp(xkb->dpy, &tmp, &vd) && tmp != NULL ) {
        return strdup(tmp);
    }

    return NULL;
}


static char* FcitxXkbFindXkbRulesFile(FcitxXkb* xkb)
{
    char* rulesFile = NULL;
    char* rulesName = FcitxXkbGetRulesName(xkb);

    if ( rulesName != NULL ) {
        if (rulesName[0] == '/') {
            asprintf(&rulesFile, "%s.xml", rulesName);
        }
        else {
            char* xkbParentDir = NULL;

            const char* base = XLIBDIR;

            int count = 0, i = 0;
            while(base[i]) {
                if (base[i] == '/')
                    count++;
                i ++;
            }

            if( count >= 3 ) {
                // .../usr/lib/X11 -> /usr/share/X11/xkb vs .../usr/X11/lib -> /usr/X11/share/X11/xkb
                const char* delta = StringEndsWith(base, "X11") ? "/../../share/X11" : "/../share/X11";
                char* dirPath;
                asprintf(&dirPath, "%s%s", base, delta);

                DIR* dir = opendir(dirPath);
                if( dir ) {
                    closedir(dir);
                    xkbParentDir = realpath(dirPath, NULL);
                    free(dirPath);
                }
                else {
                    free(dirPath);
                    asprintf(&dirPath, "%s/X11", base);
                    DIR* dir = opendir(dirPath);
                    if( dir ) {
                        closedir(dir);
                        xkbParentDir = realpath(dirPath, NULL);
                    }
                    free(dirPath);
                }
            }

            if( xkbParentDir == NULL || strlen(xkbParentDir) == 0 ) {
                xkbParentDir = strdup("/usr/share/X11");
            }

            asprintf(&rulesFile, "%s/xkb/rules/%s.xml", xkbParentDir, rulesName);
            fcitx_utils_free(xkbParentDir);
        }
    }

    fcitx_utils_free(rulesName);

    if (rulesFile == NULL)
        rulesFile = strdup(XKB_RULES_XML_FILE);

    return rulesFile;
}

static void
FcitxXkbInitDefaultLayout (FcitxXkb* xkb)
{
    Display* dpy = xkb->dpy;
    XkbRF_VarDefsRec vd;
    char *tmp = NULL;

    if (xkb->defaultLayouts) fcitx_utils_free_string_list(xkb->defaultLayouts);
    if (xkb->defaultModels) fcitx_utils_free_string_list(xkb->defaultModels);
    if (xkb->defaultOptions) fcitx_utils_free_string_list(xkb->defaultOptions);
    if (xkb->defaultVariants) fcitx_utils_free_string_list(xkb->defaultVariants);

    if (!XkbRF_GetNamesProp(dpy, &tmp, &vd) || !tmp)
    {
        FcitxLog(WARNING, "Couldn't interpret %s property", _XKB_RF_NAMES_PROP_ATOM);
        return;
    }
    if (!vd.model || !vd.layout) {
        FcitxLog (WARNING, "Could not get group layout from X property");
        return;
    }
    if (vd.layout)
        xkb->defaultLayouts = fcitx_utils_split_string (vd.layout, ',');
    else
        xkb->defaultLayouts = fcitx_utils_new_string_list();
    if (vd.model)
        xkb->defaultModels = fcitx_utils_split_string (vd.model, ',');
    else
        xkb->defaultModels = fcitx_utils_new_string_list();
    if (vd.options)
        xkb->defaultOptions = fcitx_utils_split_string (vd.options, ',');
    else
        xkb->defaultOptions = fcitx_utils_new_string_list();
    if (vd.variant) {
        xkb->defaultVariants = fcitx_utils_split_string (vd.variant, ',');
    }
    else
        xkb->defaultVariants = fcitx_utils_new_string_list();
}

static Bool
FcitxXkbSetRules (FcitxXkb* xkb,
             const char *rules_file, const char *model,
             const char *all_layouts, const char *all_variants,
             const char *all_options)
{
    Display* dpy = xkb->dpy;
    char *rulesPath;
    XkbRF_RulesPtr rules = NULL;
    XkbRF_VarDefsRec rdefs;
    XkbComponentNamesRec rnames;
    XkbDescPtr xkbDesc;

    if (!rules_file)
        return False;

    if (rules_file[0] != '/')
        asprintf(&rulesPath, "./rules/%s", rules_file);
    else
        rulesPath = strdup(rules_file);
    rules = XkbRF_Load (rulesPath, "C", True, True);
    if (rules == NULL) {
        free (rulesPath);

        rulesPath = FcitxXkbFindXkbRulesFile(xkb);
        if (strcmp (rulesPath + strlen(rulesPath) - 4, ".xml") == 0) {
            char* old = rulesPath;
            rulesPath = strndup (rulesPath,
                                 strlen (rulesPath) - 4);
            free(old);
        }
        rules = XkbRF_Load (rulesPath, "C", True, True);
    }
    if (rules == NULL) {
        free(rulesPath);
        return False;
    }

    memset (&rdefs, 0, sizeof (XkbRF_VarDefsRec));
    memset (&rnames, 0, sizeof (XkbComponentNamesRec));
    rdefs.model = model ? strdup (model) : NULL;
    rdefs.layout = all_layouts ? strdup (all_layouts) : NULL;
    rdefs.variant = all_variants ? strdup (all_variants) : NULL;
    rdefs.options = all_options ? strdup (all_options) : NULL;
    XkbRF_GetComponents (rules, &rdefs, &rnames);
    xkbDesc = XkbGetKeyboardByName (dpy, XkbUseCoreKbd, &rnames,
                                XkbGBN_AllComponentsMask,
                                XkbGBN_AllComponentsMask &
                                (~XkbGBN_GeometryMask), True);
    if (!xkbDesc) {
        FcitxLog (WARNING, "Cannot load new keyboard description.");
        return False;
    }
    XkbRF_SetNamesProp (dpy, rulesPath, &rdefs);
    free (rulesPath);
    free (rdefs.model);
    free (rdefs.layout);
    free (rdefs.variant);
    free (rdefs.options);

    return True;
}

static Bool
FcitxXkbUpdateProperties (FcitxXkb* xkb,
                     const char *rules_file, const char *model,
                     const char *all_layouts, const char *all_variants,
                     const char *all_options)
{
    Display *dpy = xkb->dpy;
    int len;
    char *pval;
    char *next;
    Atom rules_atom;
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
    if ((next - pval) != len) {
        free (pval);
        return True;
    }

    XChangeProperty (dpy, root_window,
                    rules_atom, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) pval, len);
    XSync(dpy, False);

    return True;
}

static void
FcitxXkbGetCurrentLayoutInternal(FcitxXkb *xkb, char **layout, char **variant)
{
    char* const * layoutName = (char* const*) utarray_eltptr(xkb->defaultLayouts, FcitxXkbGetCurrentGroup(xkb));
    char* const * pVariantName = (char* const*) utarray_eltptr(xkb->defaultVariants, FcitxXkbGetCurrentGroup(xkb));
    if (layoutName)
        *layout = strdup(*layoutName);
    else
        *layout = NULL;
    if (pVariantName && strlen(*pVariantName) != 0)
        *variant = strdup(*pVariantName);
    else
        *variant = NULL;
}

static void*
FcitxXkbGetCurrentLayout (void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkbGetCurrentLayoutInternal((FcitxXkb*)arg,
                                     args.args[0], args.args[1]);
    return NULL;
}
#if 0

static char *
FcitxXkbGetCurrentModel (FcitxXkb* xkb)
{
    if (xkb->defaultModels == NULL) {
        return NULL;
    }
    return fcitx_utils_join_string_list(xkb->defaultModels, ',');
}

static char *
FcitxXkbGetCurrentOption (FcitxXkb* xkb)
{
    if (xkb->defaultOptions == NULL) {
        return NULL;
    }
    return fcitx_utils_join_string_list(xkb->defaultOptions, ',');
}
#endif

/* This SHOULD be _SetLayouts_ .... */
static boolean
FcitxXkbSetLayout  (FcitxXkb* xkb,
               const char *layouts,
               const char *variants,
               const char *options)
{
    boolean retval;
    char *layouts_line;
    char *options_line;
    char *variants_line;
    char *model_line;

    if (xkb->defaultLayouts == NULL) {
        FcitxLog(WARNING, "Your system seems not to support XKB.");
        return False;
    }

    if (layouts == NULL)
        layouts_line = fcitx_utils_join_string_list (xkb->defaultLayouts, ',');
    else
        layouts_line = strdup (layouts);

    if (variants == NULL)
        variants_line = fcitx_utils_join_string_list(xkb->defaultVariants, ',');
    else
        variants_line = strdup(variants);

    if (options == NULL)
        options_line = fcitx_utils_join_string_list(xkb->defaultOptions, ',');
    else
        options_line = strdup(options);

    model_line = fcitx_utils_join_string_list(xkb->defaultModels, ',');

    retval = FcitxXkbSetRules(xkb,
                              FcitxXkbGetRulesName(xkb), model_line,
                              layouts_line, variants_line, options_line);
    FcitxXkbUpdateProperties(xkb,
                             FcitxXkbGetRulesName(xkb), model_line,
                             layouts_line, variants_line, options_line);
    free (layouts_line);
    free (variants_line);
    free (options_line);
    free (model_line);

    return retval;
}

static int
FcitxXkbGetCurrentGroup (FcitxXkb* xkb)
{
    Display *dpy = xkb->dpy;
    XkbStateRec state;

    if (xkb->defaultLayouts == NULL) {
        FcitxLog(WARNING, "Your system seems not to support XKB.");
        return 0;
    }

    if (XkbGetState (dpy, XkbUseCoreKbd, &state) != Success) {
        FcitxLog(WARNING, "Could not get state");
        return 0;
    }

    return state.group;
}

static void FcitxXkbAddNewLayout(FcitxXkb* xkb, const char* layoutString, const char* variantString)
{
    if (!layoutString)
        return;

    while (utarray_len(xkb->defaultVariants) < utarray_len(xkb->defaultLayouts)) {
        const char* dummy = "";
        utarray_push_back(xkb->defaultVariants, &dummy);
    }
    while (utarray_len(xkb->defaultVariants) >= 4) {
        utarray_pop_back(xkb->defaultVariants);
        utarray_pop_back(xkb->defaultLayouts);
    }
    utarray_push_back(xkb->defaultLayouts, &layoutString);
    if (variantString)
        utarray_push_back(xkb->defaultVariants, &variantString);
    else {
        const char* dummy = "";
        utarray_push_back(xkb->defaultVariants, &dummy);
    }
    FcitxXkbSetLayout(xkb, NULL, NULL, NULL);
}

static int
FcitxXkbFindOrAddLayout(FcitxXkb *xkb, const char *layout, const char *variant)
{
    int index;
    if (layout == NULL)
        return -1;
    index = FcitxXkbFindLayoutIndex(xkb, layout, variant);
    if (index >= 0)
        return index;
    if (!xkb->config.bOverrideSystemXKBSettings)
        return -1;
    FcitxXkbAddNewLayout(xkb, layout, variant);
    FcitxXkbInitDefaultLayout(xkb);
    return FcitxXkbFindLayoutIndex(xkb, layout, variant);
}

static boolean
FcitxXkbSetLayoutByName(FcitxXkb *xkb, const char *layout, const char *variant)
{
    int index;
    index = FcitxXkbFindOrAddLayout(xkb, layout, variant);
    if (index < 0) {
        return false;
    }
    XkbLockGroup(xkb->dpy, XkbUseCoreKbd, index);
    return true;
}

static void
FcitxXkbRetrieveCloseGroup(FcitxXkb *xkb)
{
    LayoutOverride* item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, "default", item);
    if (item)
        FcitxXkbSetLayoutByName(xkb, item->layout, item->variant);
    else
        FcitxXkbSetLayoutByName(xkb, xkb->closeLayout, xkb->closeVariant);
}

static void FcitxXkbIMKeyboardLayoutChanged(void* arg, const void* value)
{
    /* fcitx-keyboard will be useless, if fcitx-xkb doesn't change the layout for them */
    FcitxXkb* xkb = (FcitxXkb*) arg;
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(xkb->owner);
    if (xkb->config.bIgnoreInputMethodLayoutRequest
        && (!currentIM || strncmp(currentIM->uniqueName, "fcitx-keyboard",
                                  strlen("fcitx-keyboard")) != 0))
    {
        FcitxXkbRetrieveCloseGroup(xkb);
        return;
    }

    /* active means im will be take care */
    if (FcitxInstanceGetCurrentStatev2(xkb->owner) == IS_ACTIVE) {
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
            const char* layout = (const char*) value;
            if (layout) {
                UT_array* s = fcitx_utils_split_string(layout, ',');
                char** pLayoutString = (char**) utarray_eltptr(s, 0);
                char** pVariantString = (char**) utarray_eltptr(s, 1);
                layoutString = (pLayoutString)? *pLayoutString: NULL;
                variantString = (pVariantString)? *pVariantString: NULL;
            }
            else {
                layoutString = NULL;
                variantString = NULL;
            }
        }
        if (!FcitxXkbSetLayoutByName(xkb, layoutString, variantString))
            FcitxXkbRetrieveCloseGroup(xkb);
        if (s)
            fcitx_utils_free_string_list(s);
    }
    else {
        FcitxXkbRetrieveCloseGroup(xkb);
    }
}

static boolean strcmp0(const char* a, const char* b)
{
    boolean isemptya = !(a && *a);
    boolean isemptyb = !(b && *b);
    if (isemptya && isemptyb)
        return true;
    if (isemptya ^ isemptyb)
        return false;
    return strcmp(a, b) == 0;
}

static int FcitxXkbFindLayoutIndex(FcitxXkb* xkb, const char* layout, const char* variant)
{
    char** layoutName;
    char* variantName, **pVariantName;
    if (layout == NULL)
        return -1;

    int i = 0;
    for (i = 0; i < utarray_len(xkb->defaultLayouts); i++ )
    {
        layoutName = (char**) utarray_eltptr(xkb->defaultLayouts, i);
        pVariantName = (char**) utarray_eltptr(xkb->defaultVariants, i);
        variantName = pVariantName ? *pVariantName : NULL;
        if (strcmp(*layoutName, layout) == 0 && strcmp0(variantName, variant)) {
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
        if (tmpvariant)
            free(tmpvariant);
        return;
    }
    if (xkb->closeLayout)
        free(xkb->closeLayout);
    xkb->closeLayout = tmplayout;
    if (xkb->closeVariant)
        free(xkb->closeVariant);
    xkb->closeVariant = tmpvariant;
}

static void* FcitxXkbCreate(FcitxInstance* instance)
{
    FcitxAddon* addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), "fcitx-xkb");
    FcitxXkb* xkb = (FcitxXkb*) fcitx_utils_malloc0(sizeof(FcitxXkb));
    xkb->owner = instance;
    do {

        FcitxModuleFunctionArg args;
        xkb->dpy = InvokeFunction(xkb->owner, FCITX_X11, GETDISPLAY, args);
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

        FcitxXkbInitDefaultLayout(xkb);
        FcitxXkbSaveCloseGroup(xkb);

        int eventMask = XkbNewKeyboardNotifyMask | XkbStateNotifyMask;
        XkbSelectEvents(xkb->dpy, XkbUseCoreKbd, eventMask, eventMask);

        FcitxModuleFunctionArg arg2;
        arg2.args[0] = FcitxXkbEventHandler;
        arg2.args[1] = xkb;
        InvokeFunction(xkb->owner, FCITX_X11, ADDXEVENTHANDLER, arg2);

        FcitxInstanceWatchContext(instance, CONTEXT_IM_KEYBOARD_LAYOUT, FcitxXkbIMKeyboardLayoutChanged, xkb);

        FcitxIMEventHook hk;
        hk.arg = xkb;
        hk.func = FcitxXkbCurrentStateChanged;
        FcitxInstanceRegisterInputFocusHook(instance, hk);
        FcitxInstanceRegisterInputUnFocusHook(instance, hk);
        FcitxInstanceRegisterTriggerOffHook(instance, hk);

        hk.arg = xkb;
        hk.func = FcitxXkbCurrentStateChangedTriggerOn;
        FcitxInstanceRegisterTriggerOnHook(instance, hk);

        AddFunction(addon, FcitxXkbGetRules);
        AddFunction(addon, FcitxXkbGetCurrentLayout);
        AddFunction(addon, FcitxXkbLayoutExists);
        AddFunction(addon, FcitxXkbGetLayoutOverride);
        AddFunction(addon, FcitxXkbSetLayoutOverride);
        AddFunction(addon, FcitxXkbSetDefaultLayout);

        if (xkb->config.bOverrideSystemXKBSettings)
            FcitxXkbSetLayout(xkb, NULL, NULL, NULL);

        return xkb;
    } while (0);

    free(xkb);

    return NULL;
}

static boolean FcitxXkbEventHandler(void* arg, XEvent* event)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;

    if (event->type == xkb->xkbOpcode) {
        XkbEvent *xkbEvent = (XkbEvent*) event;

        if (xkbEvent->any.xkb_type == XkbStateNotify && xkbEvent->state.changed & GROUP_CHANGE_MASK) {
            FcitxIM* currentIM = FcitxInstanceGetCurrentIM(xkb->owner);
            if (FcitxInstanceGetCurrentStatev2(xkb->owner) != IS_ACTIVE
                || (xkb->config.bIgnoreInputMethodLayoutRequest
                    && (!currentIM ||
                        strncmp(currentIM->uniqueName, "fcitx-keyboard",
                                strlen("fcitx-keyboard")) != 0)))
                FcitxXkbSaveCloseGroup(xkb);
        }

        if (xkbEvent->any.xkb_type == XkbNewKeyboardNotify) {
            XSync(xkb->dpy, False);
            FcitxUIUpdateInputWindow(xkb->owner);
            FcitxXkbInitDefaultLayout(xkb);
            FcitxXkbApplyCustomScript(xkb);
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
    fcitx_utils_free_string_list (xkb->defaultVariants);
    xkb->defaultVariants = NULL;
    fcitx_utils_free_string_list (xkb->defaultLayouts);
    xkb->defaultLayouts = NULL;
    fcitx_utils_free_string_list (xkb->defaultModels);
    xkb->defaultModels = NULL;
    fcitx_utils_free_string_list (xkb->defaultOptions);
    xkb->defaultOptions = NULL;
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


static void* FcitxXkbGetRules(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    return xkb->rules;
}

static void* FcitxXkbLayoutExists(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkb* xkb = (FcitxXkb*) arg;
    int idx = FcitxXkbFindLayoutIndex(xkb, args.args[0], args.args[1]);
    boolean* result = args.args[2];
    *result = (idx >= 0);
    return NULL;
}

static void FcitxXkbApplyCustomScript(FcitxXkb* xkb)
{
    if (!xkb->config.bOverrideSystemXKBSettings)
        return;

    if (!(xkb->config.xmodmapCommand && xkb->config.xmodmapCommand[0]
        && xkb->config.customXModmapScript && xkb->config.customXModmapScript[0]))
        return;

    char* customXModmapScript = NULL;
    FcitxXDGGetFileUserWithPrefix("data", xkb->config.customXModmapScript, NULL, &customXModmapScript);

    char* args[] = {
        xkb->config.xmodmapCommand,
        customXModmapScript,
        0
    };
    fcitx_utils_start_process(args);
    free(customXModmapScript);
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
        LayoutOverride* cur = xkb->layoutOverride;
        HASH_DEL(xkb->layoutOverride, cur);
        LayoutOverrideFree(cur);
    }

    FILE *fp = FcitxXDGGetFileUserWithPrefix("data", "layout_override", "r", NULL);
    if (!fp)
        return;

    while (getline(&buf, &len, fp) != -1) {
        if (trimedbuf)
            free(trimedbuf);
        trimedbuf = fcitx_utils_trim(buf);

        UT_array* s = fcitx_utils_split_string(trimedbuf, ',');
        char** pIMString = (char**) utarray_eltptr(s, 0);
        char** pLayoutString = (char**) utarray_eltptr(s, 1);
        char** pVariantString = (char**) utarray_eltptr(s, 2);
        char* imString = (pIMString)? *pIMString: NULL;
        char* layoutString = (pLayoutString)? *pLayoutString: NULL;
        char* variantString = (pVariantString)? *pVariantString: NULL;

        if (!imString || !layoutString)
            continue;
        LayoutOverride* override = NULL;
        HASH_FIND_STR(xkb->layoutOverride, imString, override);
        /* if rule exists or the rule is for fcitx-keyboard, skip it */
        if (override || strncmp(imString, "fcitx-keyboard",
                                strlen("fcitx-keyboard")) == 0) {
            continue;
        }

        override = fcitx_utils_new(LayoutOverride);
        override->im = strdup(imString);
        override->layout = strdup(layoutString);
        override->variant= variantString? strdup(variantString) : NULL;

        HASH_ADD_KEYPTR(hh, xkb->layoutOverride, override->im, strlen(override->im), override);
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

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-xkb.config", "r", NULL);

    if (!fp)
    {
        if (errno == ENOENT)
            SaveXkbConfig(xkb);
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


void* FcitxXkbGetLayoutOverride(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkb* xkb = arg;
    const char* imname = args.args[0];
    char** layout = args.args[1];
    char** variant = args.args[2];
    LayoutOverride* item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, imname, item);
    if (item) {
        *layout = item->layout;
        *variant = item->variant;
    }
    else {
        *layout = NULL;
        *variant = NULL;
    }
    return NULL;
}


void* FcitxXkbSetLayoutOverride(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkb* xkb = arg;
    const char* imname = args.args[0];
    const char* layout = args.args[1];
    const char* variant = args.args[2];
    LayoutOverride* item = NULL;
    HASH_FIND_STR(xkb->layoutOverride, imname, item);
    if (item) {
        HASH_DEL(xkb->layoutOverride, item);
        LayoutOverrideFree(item);
    }

    if (layout && layout[0] != '\0' &&
        strncmp(imname, "fcitx-keyboard", strlen("fcitx-keyboard")) != 0) {
        item = fcitx_utils_new(LayoutOverride);
        item->im = strdup(imname);
        item->layout = strdup(layout);
        item->variant= (variant && variant[0] != '\0')? strdup(variant) : NULL;

        HASH_ADD_KEYPTR(hh, xkb->layoutOverride, item->im, strlen(item->im), item);
    }

    SaveLayoutOverride(xkb);
    FcitxXkbCurrentStateChanged(xkb);

    return NULL;
}

void* FcitxXkbSetDefaultLayout(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXkb* xkb = arg;
    const char* layout = args.args[0];
    const char* variant = args.args[1];
    LayoutOverride* item = NULL;
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

        HASH_ADD_KEYPTR(hh, xkb->layoutOverride, item->im, strlen(item->im), item);
    }

    SaveLayoutOverride(xkb);
    FcitxXkbCurrentStateChanged(xkb);

    return NULL;
}

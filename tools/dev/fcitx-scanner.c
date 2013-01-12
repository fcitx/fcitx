/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcitx-utils/desktop-parse.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utarray.h>
#include <fcitx/fcitx.h>

/**
 * strings
 **/

static const char *fxscanner_header_str =
    "/************************************************************************\n"
    " * This program is free software; you can redistribute it and/or modify *\n"
    " * it under the terms of the GNU General Public License as published by *\n"
    " * the Free Software Foundation; either version 2 of the License, or    *\n"
    " * (at your option) any later version.                                  *\n"
    " *                                                                      *\n"
    " * This program is distributed in the hope that it will be useful,      *\n"
    " * but WITHOUT ANY WARRANTY; without even the implied warranty of       *\n"
    " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *\n"
    " * GNU General Public License for more details.                         *\n"
    " *                                                                      *\n"
    " * You should have received a copy of the GNU General Public License    *\n"
    " * along with this program; if not, write to the                        *\n"
    " * Free Software Foundation, Inc.,                                      *\n"
    " * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *\n"
    " ************************************************************************/\n";

#define FXSCANNER_BLANK " \b\f\v\r\t"

/**
 * typedefs
 **/

typedef struct {
    FcitxDesktopFile dfile;
    FcitxDesktopGroup *addon_grp;
    const char *name;
    size_t name_len;
    const char *prefix;
    const char *self_type;
    UT_array includes;
    UT_array functions;
    UT_array macros;
} FcitxAddonDesc;

typedef struct {
    const char *name;
    FcitxDesktopGroup *grp;
    boolean define;
    const char *value;
} FcitxAddonMacroDesc;

static const UT_icd fxaddon_macro_icd = {
    .sz = sizeof(FcitxAddonMacroDesc)
};

typedef struct {
    const char *name;
    FcitxDesktopGroup *grp;
    const char *name_id;
    const char *type;
    const char *err_ret;
    boolean cache;
    boolean enable_wrapper;
    UT_array args;
    const char *inline_code;
    const char *res_deref;
    const char *res_dereftype;
    const char *res_exp;
    const char *res_wrapfunc;
    const char *is_static;
} FcitxAddonFuncDesc;

static void
_fxaddon_func_desc_copy(void *_dst, const void *_src)
{
    FcitxAddonFuncDesc *dst = _dst;
    const FcitxAddonFuncDesc *src = _src;
    *dst = *src;
    dst->args = *utarray_clone(&src->args);
}

static void
_fxaddon_func_desc_free(void *_elt)
{
    FcitxAddonFuncDesc *elt = _elt;
    utarray_done(&elt->args);
}

static const UT_icd fxaddon_func_icd = {
    .sz = sizeof(FcitxAddonFuncDesc),
    .copy = _fxaddon_func_desc_copy,
    .dtor = _fxaddon_func_desc_free
};

typedef struct {
    const char *type;
    const char *deref;
    const char *deref_type;
} FcitxAddonArgDesc;

static const UT_icd fxaddon_arg_icd = {
    .sz = sizeof(FcitxAddonArgDesc)
};

typedef struct {
    char *buff;
    size_t len;
} FcitxAddonBuff;

typedef boolean (*FxScannerListLoader)(
    UT_array *array, const char *value, FcitxAddonBuff *buff,
    size_t prefix_l, void *data);

/**
 * utility functions
 **/

static inline void
fxaddon_buff_realloc(FcitxAddonBuff *buff, size_t len)
{
    if (fcitx_unlikely(buff->len < len)) {
        buff->len = len;
        buff->buff = realloc(buff->buff, len);
    }
}

static inline size_t
_write_len(FILE *fp, const char *str, size_t len)
{
    if (!(str && len))
        return 0;
    return fwrite(str, len, 1, fp);
}

static inline size_t
_write_str(FILE *fp, const char *str)
{
    return _write_len(fp, str, strlen(str));
}

static inline int
fxscanner_strcmp_len(const char *str1, const char *str2, boolean ignore_case)
{
    int len = strlen(str2);
    if (ignore_case ?
        !strncasecmp(str1, str2, len) : !strncmp(str1, str2, len))
        return len;
    return 0;
}

static int
fxscanner_strs_strip_cmp(const char *str, boolean ignore_case, ...)
{
    va_list ap;
    const char *cmp;
    int len;
    if (!str)
        return -1;
    str += strspn(str, FXSCANNER_BLANK);
    if (!*str)
        return -1;
    va_start(ap, ignore_case);
    while ((cmp = va_arg(ap, const char*))) {
        len = fxscanner_strcmp_len(str, cmp, ignore_case);
        if (len)
            break;
    }
    va_end(ap);
    if (!len)
        return 0;
    str += len;
    str += strspn(str, FXSCANNER_BLANK);
    return *str == '\0';
}

static inline const char*
fxscanner_group_get_value(FcitxDesktopGroup *grp, const char *name)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, name);
    return ety ? ety->value : NULL;
}

static inline boolean
fxscanner_value_get_boolean(const char *value, boolean default_val)
{
    if (default_val) {
        return !(fxscanner_strs_strip_cmp(value, true, "off", "false",
                                          "no", "0", NULL) > 0);
    } else {
        return fxscanner_strs_strip_cmp(value, true, "on", "true",
                                        "yes", "1", NULL) > 0;
    }
}

static inline boolean
fxscanner_group_get_boolean(FcitxDesktopGroup *grp, const char *name,
                            boolean default_val)
{
    const char *value;
    value = fxscanner_group_get_value(grp, name);
    return fxscanner_value_get_boolean(value, default_val);
}

static const char*
fxscanner_ignore_blank(const char *str)
{
    if (!str)
        return NULL;
    str += strspn(str, FXSCANNER_BLANK);
    if (*str)
        return str;
    return NULL;
}

static const char*
fxscanner_std_type(const char *type)
{
    if (!type)
        return NULL;
    type += strspn(type, FXSCANNER_BLANK);
    if (fxscanner_strs_strip_cmp(type, false, "void", NULL) != 0)
        return NULL;
    return type;
}

static boolean
fxscanner_load_entry_list(UT_array *ary, FcitxDesktopGroup *grp,
                          const char *prefix, boolean stop_at_empty,
                          FxScannerListLoader loader, void *data)
{
    size_t prefix_len = strlen(prefix);
    FcitxAddonBuff buff = {0};
    int i;
    FcitxDesktopEntry *tmp_ety;
    boolean res = true;
    for (i = 0;;i++) {
        fxaddon_buff_realloc(&buff, prefix_len + FCITX_INT_LEN + 1);
        memcpy(buff.buff, prefix, prefix_len);
        sprintf(buff.buff + prefix_len, "%d", i);
        tmp_ety = fcitx_desktop_group_find_entry(grp, buff.buff);
        if (!tmp_ety)
            break;
        if (!(tmp_ety->value && *tmp_ety->value)) {
            if (stop_at_empty) {
                break;
            } else {
                continue;
            }
        }
        if (loader) {
            if (!loader(ary, tmp_ety->value, &buff, prefix_len, data)) {
                res = false;
                break;
            }
        } else {
            utarray_push_back(ary, &tmp_ety->value);
        }
    }
    free(buff.buff);
    return res;
}

#define fxscanner_load_entry_list(ary, grp, prefix, stop, args...) \
    _fxscanner_load_entry_list(ary, grp, prefix, stop, ##args, NULL, NULL)
#define _fxscanner_load_entry_list(ary, grp, prefix, stop, loader, data, ...) \
    (fxscanner_load_entry_list)(ary, grp, prefix, stop, loader, data)

static void
fxscanner_name_to_macro(char *name)
{
    for (;*name;name++) {
        switch (*name) {
        case 'a' ... 'z':
            *name += 'A' - 'a';
            break;
        case '-':
            *name = '_';
            break;
        }
    }
}

/**
 * load .fxaddon
 **/

static const char*
fxscanner_function_get_return(FcitxDesktopGroup *grp)
{
    const char *type = fxscanner_group_get_value(grp, "Return");
    return fxscanner_std_type(type);
}

static boolean
fxscanner_macro_get_define(FcitxDesktopGroup *grp)
{
    const char *value;
    value = fxscanner_group_get_value(grp, "Define");
    if (value)
        return fxscanner_value_get_boolean(value, false);
    value = fxscanner_group_get_value(grp, "Undefine");
    if (value)
        return !fxscanner_value_get_boolean(value, false);
    return true;
}

#define fxaddon_load_string(tgt, grp, name)     \
    tgt = fxscanner_group_get_value(grp, name);

static boolean
fxscanner_macro_loader(UT_array *array, const char *value,
                       FcitxAddonBuff *buff, size_t prefix_l, void *data)
{
    FCITX_UNUSED(buff);
    FCITX_UNUSED(prefix_l);
    FcitxDesktopGroup *grp;
    FcitxAddonDesc *desc = data;
    grp = fcitx_desktop_file_find_group(&desc->dfile, value);
    /* ignore */
    if (!grp)
        return true;
    FcitxAddonMacroDesc macro_desc = {
        .name = value,
        .grp = grp,
        .value = fxscanner_group_get_value(grp, "Value"),
        .define = fxscanner_macro_get_define(grp)
    };
    utarray_push_back(array, &macro_desc);
    return true;
}

static boolean
fxscanner_arg_loader(UT_array *array, const char *value, FcitxAddonBuff *buff,
                     size_t prefix_l, void *data)
{
    FcitxAddonArgDesc arg_desc = {
        .type = value
    };
    FcitxDesktopGroup *grp = data;
    fxaddon_buff_realloc(buff, prefix_l + sizeof(".DerefType"));
    memcpy(buff->buff + prefix_l, ".Deref", sizeof(".Deref"));
    const char *tmp_str;
    tmp_str = fxscanner_group_get_value(grp, buff->buff);
    arg_desc.deref = fxscanner_ignore_blank(tmp_str);
    memcpy(buff->buff + prefix_l, ".DerefType", sizeof(".DerefType"));
    tmp_str = fxscanner_group_get_value(grp, buff->buff);
    arg_desc.deref_type = fxscanner_ignore_blank(tmp_str);
    if ((!!arg_desc.deref) != (!!arg_desc.deref_type)) {
        FcitxLog(ERROR, "DerefType and Deref must be both set (or not set).");
        return false;
    }
    utarray_push_back(array, &arg_desc);
    return true;
}

static boolean
fxscanner_func_loader(UT_array *array, const char *value, FcitxAddonBuff *buff,
                      size_t prefix_l, void *data)
{
    FCITX_UNUSED(buff);
    FCITX_UNUSED(prefix_l);
    FcitxAddonDesc *desc = data;
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(&desc->dfile, value);
    if (!grp)
        return false;
    FcitxAddonFuncDesc func_desc = {
        .name = value,
        .grp = grp
    };
    fxaddon_load_string(func_desc.name_id, grp, "Name");
    if (!func_desc.name_id)
        return false;
    func_desc.type = fxscanner_function_get_return(grp);
    fxaddon_load_string(func_desc.err_ret, grp, "ErrorReturn");
    func_desc.cache = fxscanner_group_get_boolean(grp, "CacheResult", false);
    func_desc.enable_wrapper = fxscanner_group_get_boolean(
        grp, "EnableWrapper", true);
    fxaddon_load_string(func_desc.inline_code, grp, "Inline.Code");
    fxaddon_load_string(func_desc.res_deref, grp, "Res.Deref");
    fxaddon_load_string(func_desc.res_dereftype, grp, "Res.DerefType");
    if ((!!func_desc.res_deref) != (!!func_desc.res_dereftype)) {
        FcitxLog(ERROR, "DerefType and Deref must be both set (or not set).");
        return false;
    }
    fxaddon_load_string(func_desc.res_exp, grp, "Res.Exp");
    fxaddon_load_string(func_desc.res_wrapfunc, grp, "Res.WrapFunc");
    fxaddon_load_string(func_desc.is_static, grp, "Static");
    utarray_init(&func_desc.args, &fxaddon_arg_icd);
    if (!fxscanner_load_entry_list(&func_desc.args, grp, "Arg", true,
                                   fxscanner_arg_loader, grp)) {
        utarray_done(&func_desc.args);
        return false;
    }
    utarray_push_back(array, &func_desc);
    return true;
}

/**
 * write .h
 **/

static void
fxscanner_write_header(FILE *ofp)
{
    _write_str(ofp, fxscanner_header_str);
}

static void
fxscanner_write_includes(FILE *ofp, UT_array *includes)
{
    _write_str(ofp,
               "#include <stdint.h>\n"
               "#include <fcitx-utils/utils.h>\n"
               "#include <fcitx/instance.h>\n"
               "#include <fcitx/addon.h>\n"
               "#include <fcitx/module.h>");
    char **p;
    for (p = (char**)utarray_front(includes);p;
         p = (char**)utarray_next(includes, p)) {
        _write_str(ofp, "\n#include ");
        _write_str(ofp, *p);
    }
    _write_str(ofp, "\n\n");
}

static void
fxscanner_write_macro(FILE *ofp, FcitxDesktopFile *dfile,
                      const char *macro_name)
{
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(dfile, macro_name);
    if (!grp)
        return;
    const char *value = fxscanner_group_get_value(grp, "Value");
    boolean define = fxscanner_macro_get_define(grp);
    _write_str(ofp, "#ifdef ");
    _write_str(ofp, macro_name);
    _write_str(ofp,
               "\n"
               "#  undef ");
    _write_str(ofp, macro_name);
    _write_str(ofp, "\n"
                    "#endif\n");
    if (define) {
        _write_str(ofp, "#define ");
        _write_str(ofp, macro_name);
        if (value && *value) {
            _write_str(ofp, " ");
            _write_str(ofp, value);
        }
        _write_str(ofp, "\n");
    }
}

static void
fxscanner_write_function(FILE *ofp, FcitxDesktopFile *dfile, const char *prefix,
                         const char *func_name, int id)
{
    FcitxDesktopGroup *grp;
    unsigned int i;
    grp = fcitx_desktop_file_find_group(dfile, func_name);
    if (!grp)
        return;
    /* require the Name entry although not used now. */
    if (!fcitx_desktop_group_find_entry(grp, "Name"))
        return;
    UT_array args;
    utarray_init(&args, fcitx_ptr_icd);
    fxscanner_load_entry_list(&args, grp, "Arg", true);
    const char *type = fxscanner_function_get_return(grp);
    const char *err_ret = fxscanner_group_get_value(grp, "ErrorReturn");
    boolean cache = fxscanner_group_get_boolean(grp, "CacheResult", false);
    boolean enable_wrapper = fxscanner_group_get_boolean(grp, "EnableWrapper",
                                                         true);
    if (cache && !type) {
        FcitxLog(WARNING, "Cannot cache result of type void.");
        cache = false;
    }
    if (!err_ret) {
        _write_str(ofp, "DEFINE_GET_AND_INVOKE_FUNC(");
        _write_str(ofp, prefix);
        _write_str(ofp, ", ");
        _write_str(ofp, func_name);
        _write_str(ofp, ", ");
        fprintf(ofp, "%d", id);
        _write_str(ofp, ")\n");
    } else {
        _write_str(ofp, "DEFINE_GET_AND_INVOKE_FUNC_WITH_ERROR(");
        _write_str(ofp, prefix);
        _write_str(ofp, ", ");
        _write_str(ofp, func_name);
        _write_str(ofp, ", ");
        fprintf(ofp, "%d", id);
        _write_str(ofp, ", ");
        _write_str(ofp, err_ret);
        _write_str(ofp, ")\n");
    }
    if (!enable_wrapper)
        _write_str(ofp, "#if 0\n");
    _write_str(ofp, "static inline ");
    _write_str(ofp, type ? type : "void");
    _write_str(ofp, "\nFcitx");
    _write_str(ofp, prefix);
    _write_str(ofp, func_name);
    _write_str(ofp, "(FcitxInstance *instance");
    char **p;
    for (i = 0;i < utarray_len(&args);i++) {
        p = (char**)_utarray_eltptr(&args, i);
        _write_str(ofp, ", ");
        _write_str(ofp, *p);
        _write_str(ofp, " _arg");
        fprintf(ofp, "%d", i);
    }
    _write_str(ofp,
               ")\n"
               "{\n");
    if (cache) {
        _write_str(ofp,
                   "    static boolean _init = false;\n"
                   "    static void *result = NULL;\n"
                   "    if (fcitx_likely(_init))\n"
                   "        FCITX_RETURN_FROM_PTR(");
        _write_str(ofp, type);
        _write_str(ofp,
                   ", result);\n"
                   "    _init = true;\n");
    } else if (type) {
        _write_str(ofp, "    void *result;\n");
    }
    for (i = 0;i < utarray_len(&args);i++) {
        p = (char**)_utarray_eltptr(&args, i);
        _write_str(ofp, "    FCITX_DEF_CAST_TO_PTR(arg");
        fprintf(ofp, "%d", i);
        _write_str(ofp, ", ");
        _write_str(ofp, *p);
        _write_str(ofp, ", _arg");
        fprintf(ofp, "%d", i);
        _write_str(ofp, ");\n");
    }
    _write_str(ofp, "    FCITX_DEF_MODULE_ARGS(args");
    for (i = 0;i < utarray_len(&args);i++) {
        _write_str(ofp, ", arg");
        fprintf(ofp, "%d", i);
    }
    _write_str(ofp,
               ");\n"
               "    ");
    if (type) {
        _write_str(ofp, "result = ");
    }
    _write_str(ofp, "Fcitx");
    _write_str(ofp, prefix);
    _write_str(ofp, "Invoke");
    _write_str(ofp, func_name);
    _write_str(ofp, "(instance, args);\n");
    if (type) {
        _write_str(ofp, "    FCITX_RETURN_FROM_PTR(");
        _write_str(ofp, type);
        _write_str(ofp, ", result);\n");
    }
    if (enable_wrapper) {
        _write_str(ofp, "}\n\n");
    } else {
        _write_str(ofp,
                   "}\n"
                   "#endif\n\n");
    }
    utarray_done(&args);
}

static boolean
fxscanner_load_addon(FcitxAddonDesc *addon_desc, FILE *ifp)
{
    FcitxDesktopFile *dfile = &addon_desc->dfile;
    if (!fcitx_desktop_file_init(dfile, NULL, NULL))
        return false;
    if (!fcitx_desktop_file_load_fp(dfile, ifp))
        goto free_desktop;
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(dfile, "FcitxAddon");
    if (!grp)
        goto free_desktop;
    addon_desc->addon_grp = grp;
    fxaddon_load_string(addon_desc->name, grp, "Name");
    if (!addon_desc->name)
        goto free_desktop;
    fxaddon_load_string(addon_desc->prefix, grp, "Prefix")
    if (!addon_desc->prefix)
        goto free_desktop;
    fxaddon_load_string(addon_desc->self_type, grp, "Self.Type");
    utarray_init(&addon_desc->includes, fcitx_ptr_icd);
    fxscanner_load_entry_list(&addon_desc->includes, grp, "Include", false);
    utarray_init(&addon_desc->functions, &fxaddon_func_icd);
    if (!fxscanner_load_entry_list(&addon_desc->functions, grp, "Function",
                                   true, fxscanner_func_loader, addon_desc))
        goto free_functions;
    utarray_init(&addon_desc->macros, &fxaddon_macro_icd);
    if (!fxscanner_load_entry_list(&addon_desc->macros, grp, "Macro", true,
                                   fxscanner_macro_loader, addon_desc))
        goto free_macros;
    return true;
free_macros:
    utarray_done(&addon_desc->macros);
free_functions:
    utarray_done(&addon_desc->functions);
    utarray_done(&addon_desc->includes);
free_desktop:
    fcitx_desktop_file_done(dfile);
    return false;
}

static int
fxscanner_scan_addon(FILE *ifp, FILE *ofp)
{
    FcitxAddonDesc addon_desc;
    FcitxDesktopFile *dfile = &addon_desc.dfile;
    char *buff = NULL;
    unsigned int i;
    char **p;
    if (!fcitx_desktop_file_init(dfile, NULL, NULL))
        return 1;
    if (!fcitx_desktop_file_load_fp(dfile, ifp))
        return 1;
    fclose(ifp);
    FcitxDesktopEntry *tmp_ety;
    addon_desc.addon_grp = fcitx_desktop_file_find_group(dfile, "FcitxAddon");
    if (!addon_desc.addon_grp)
        return 1;
    tmp_ety = fcitx_desktop_group_find_entry(addon_desc.addon_grp, "Name");
    if (!tmp_ety)
        return 1;
    addon_desc.name = tmp_ety->value;
    tmp_ety = fcitx_desktop_group_find_entry(addon_desc.addon_grp, "Prefix");
    if (!tmp_ety)
        return 1;
    addon_desc.prefix = tmp_ety->value;
    UT_array macros;
    utarray_init(&macros, fcitx_ptr_icd);
    fxscanner_load_entry_list(&macros, addon_desc.addon_grp, "Macro", false);
    UT_array includes;
    utarray_init(&includes, fcitx_ptr_icd);
    fxscanner_load_entry_list(&includes, addon_desc.addon_grp,
                              "Include", false);
    UT_array functions;
    utarray_init(&functions, fcitx_ptr_icd);
    fxscanner_load_entry_list(&functions, addon_desc.addon_grp,
                              "Function", true);
    fxscanner_write_header(ofp);
    addon_desc.name_len = strlen(addon_desc.name);
    buff = fcitx_utils_set_str_with_len(buff, addon_desc.name,
                                        addon_desc.name_len);
    fxscanner_name_to_macro(buff);
    _write_str(ofp, "\n#ifndef __FCITX_MODULE_");
    _write_len(ofp, buff, addon_desc.name_len);
    _write_str(ofp, "_H\n");
    _write_str(ofp, "#define __FCITX_MODULE_");
    _write_len(ofp, buff, addon_desc.name_len);
    _write_str(ofp, "_H\n"
                    "\n"
                    "#ifdef __cplusplus\n"
                    "extern \"C\" {\n"
                    "#endif\n"
                    "\n");
    for (i = 0;i < utarray_len(&macros);i++) {
        p = (char**)_utarray_eltptr(&macros, i);
        fxscanner_write_macro(ofp, dfile, *p);
    }
    fxscanner_write_includes(ofp, &includes);
    utarray_done(&includes);
    _write_str(ofp, "DEFINE_GET_ADDON(\"");
    _write_len(ofp, addon_desc.name, addon_desc.name_len);
    _write_str(ofp, "\", ");
    _write_str(ofp, addon_desc.prefix);
    _write_str(ofp, ")\n\n");
    for (i = 0;i < utarray_len(&functions);i++) {
        p = (char**)_utarray_eltptr(&functions, i);
        fxscanner_write_function(ofp, dfile, addon_desc.prefix, *p, i);
    }
    _write_str(ofp, "\n"
                    "#ifdef __cplusplus\n"
                    "}\n"
                    "#endif\n"
                    "\n"
                    "#endif\n");
    fclose(ofp);
    fcitx_utils_free(buff);
    fcitx_desktop_file_done(dfile);
    utarray_done(&functions);
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc != 3)
        exit(1);
    FILE *ifp = fopen(argv[1], "r");
    if (!ifp)
        exit(1);
    FILE *ofp = fopen(argv[2], "w");
    if (!ofp)
        exit(1);
    return fxscanner_scan_addon(ifp, ofp);
}

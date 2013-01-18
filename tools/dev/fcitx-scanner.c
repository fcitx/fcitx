/***************************************************************************
 *   Copyright (C) 2012~2013 by Yichao Yu                                  *
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
    const char *self_deref;
    const char *self_dereftype;
    boolean is_static;
} FcitxAddonFuncDesc;

static void
_fxaddon_func_desc_free(void *_elt)
{
    FcitxAddonFuncDesc *elt = _elt;
    utarray_done(&elt->args);
}

static const UT_icd fxaddon_func_icd = {
    .sz = sizeof(FcitxAddonFuncDesc),
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

#define FXSCANNER_XOR(a, b) ((!!(a)) != (!!(b)))

static boolean
fxscanner_parse_int(const char *str, int *res)
{
    if (!str)
        return false;
    str += strspn(str, FXSCANNER_BLANK);
    if (!*str)
        return false;
    char *end;
    *res = strtol(str, &end, 10);
    end += strspn(end, FXSCANNER_BLANK);
    if (!*end)
        return true;
    return false;
}

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

#define _write_strings(fp, strs...) do {                                \
        const char *__strs[] = { strs };                                \
        const int __n_strs = sizeof(__strs) / sizeof(const char*);      \
        int __i;                                                        \
        for (__i = 0;__i < __n_strs;__i++) {                            \
            _write_str(fp, __strs[__i]);                                \
        }                                                               \
    } while (0)

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

static inline const char*
fxscanner_group_get_value(FcitxDesktopGroup *grp, const char *name)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, name);
    return fxscanner_ignore_blank(ety ? ety->value : NULL);
}

#define fxaddon_load_string(tgt, grp, name)     \
    tgt = fxscanner_group_get_value(grp, name)

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
fxscanner_std_type(const char *type)
{
    if (fxscanner_strs_strip_cmp(fxscanner_ignore_blank(type),
                                 false, "void", NULL) != 0)
        return NULL;
    return type;
}

static const char*
fxscanner_group_get_type(FcitxDesktopGroup *grp, const char *name)
{
    const char *type = fxscanner_group_get_value(grp, name);
    return fxscanner_std_type(type);
}

static boolean
fxscanner_load_entry_list(UT_array *ary, FcitxDesktopGroup *grp,
                          const char *prefix, boolean stop_at_empty,
                          FxScannerListLoader loader, void *data)
{
    size_t prefix_len = strlen(prefix);
    FcitxAddonBuff buff = {0, 0};
    int i;
    FcitxDesktopEntry *tmp_ety;
    boolean res = true;
    for (i = 0;;i++) {
        fxaddon_buff_realloc(&buff, prefix_len + FCITX_INT_LEN + 1);
        memcpy(buff.buff, prefix, prefix_len);
        size_t p_len = prefix_len + sprintf(buff.buff + prefix_len, "%d", i);
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
            if (!loader(ary, tmp_ety->value, &buff, p_len, data)) {
                res = false;
                break;
            }
        } else {
            utarray_push_back(ary, &tmp_ety->value);
        }
    }
    fcitx_utils_free(buff.buff);
    return res;
}

#define fxscanner_load_entry_list(ary, grp, prefix, stop, args...)      \
    _fxscanner_load_entry_list(ary, grp, prefix, stop, ##args, NULL, NULL)
#define _fxscanner_load_entry_list(ary, grp, prefix, stop, loader, data, ...) \
    (fxscanner_load_entry_list)(ary, grp, prefix, stop, loader, data)

typedef int (*FcitxScannerTranslator)(FILE *ofp, const char *str, void *data);

static boolean
fxscanner_write_translate(FILE *ofp, const char *str,
                          FcitxScannerTranslator translator, void *data,
                          const char *delim)
{
    if (!delim)
        delim = "$";
    int len;
    while (true) {
        len = strcspn(str, delim);
        if (len)
            _write_len(ofp, str, len);
        str += len;
        if (!*str)
            return true;
        if (str[0] == str[1]) {
            _write_len(ofp, str, 1);
            str += 2;
            continue;
        }
        str++;
        len = translator(ofp, str, data);
        if (len < 0)
            return false;
        str += len;
    }
}

#define fxscanner_write_translate(ofp, str, translator, args...)        \
    _fxscanner_write_translate(ofp, str, translator, ##args, NULL, NULL)
#define _fxscanner_write_translate(ofp, str, translator, data, delim, ...) \
    (fxscanner_write_translate)(ofp, str, translator, data, delim)

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
    fxaddon_load_string(arg_desc.deref, grp, buff->buff);
    memcpy(buff->buff + prefix_l, ".DerefType", sizeof(".DerefType"));
    arg_desc.deref_type = fxscanner_group_get_type(grp, buff->buff);
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
    if (!grp) {
        FcitxLog(ERROR, "Group %s not found.", value);
        return false;
    }
    FcitxAddonFuncDesc func_desc = {
        .name = value,
        .grp = grp
    };
    fxaddon_load_string(func_desc.name_id, grp, "Name");
    if (!func_desc.name_id) {
        FcitxLog(ERROR, "Entry \"Name\" not found in group [%s].", value);
        return false;
    }
    func_desc.type = fxscanner_group_get_type(grp, "Return");
    fxaddon_load_string(func_desc.err_ret, grp, "ErrorReturn");
    func_desc.cache = fxscanner_group_get_boolean(grp, "CacheResult", false);
    func_desc.enable_wrapper = fxscanner_group_get_boolean(
        grp, "EnableWrapper", true);
    fxaddon_load_string(func_desc.inline_code, grp, "Inline.Code");
    fxaddon_load_string(func_desc.res_deref, grp, "Res.Deref");
    func_desc.res_dereftype = fxscanner_group_get_type(grp, "Res.DerefType");
    fxaddon_load_string(func_desc.self_deref, grp, "Self.Deref");
    func_desc.self_dereftype = fxscanner_group_get_type(grp, "Self.DerefType");
    fxaddon_load_string(func_desc.res_exp, grp, "Res.Exp");
    fxaddon_load_string(func_desc.res_wrapfunc, grp, "Res.WrapFunc");
    func_desc.is_static = fxscanner_group_get_boolean(grp, "Static", false);
    utarray_init(&func_desc.args, &fxaddon_arg_icd);
    if (!fxscanner_load_entry_list(&func_desc.args, grp, "Arg", true,
                                   fxscanner_arg_loader, grp) ||
        utarray_len(&func_desc.args) > 10) {
        utarray_done(&func_desc.args);
        return false;
    }
    utarray_push_back(array, &func_desc);
    return true;
}

static void
fxscanner_addon_init(FcitxAddonDesc *addon_desc)
{
    fcitx_desktop_file_init(&addon_desc->dfile, NULL, NULL);
    utarray_init(&addon_desc->includes, fcitx_ptr_icd);
    utarray_init(&addon_desc->functions, &fxaddon_func_icd);
    utarray_init(&addon_desc->macros, &fxaddon_macro_icd);
}

static void
fxscanner_addon_done(FcitxAddonDesc *addon_desc)
{
    fcitx_desktop_file_done(&addon_desc->dfile);
    utarray_done(&addon_desc->includes);
    utarray_done(&addon_desc->functions);
    utarray_done(&addon_desc->macros);
}

static boolean
fxscanner_addon_load(FcitxAddonDesc *addon_desc, FILE *ifp)
{
    FcitxDesktopFile *dfile = &addon_desc->dfile;
    if (!fcitx_desktop_file_load_fp(dfile, ifp)) {
        FcitxLog(ERROR, "Failed to load desktop file.");
        return false;
    }
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(dfile, "FcitxAddon");
    if (!grp) {
        FcitxLog(ERROR, "group [FcitxAddon] not found.");
        return false;
    }
    addon_desc->addon_grp = grp;
    fxaddon_load_string(addon_desc->name, grp, "Name");
    if (!addon_desc->name) {
        FcitxLog(ERROR, "Entry \"Name\" not found in [FcitxAddon] group.");
        return false;
    }
    addon_desc->name_len = strlen(addon_desc->name);
    fxaddon_load_string(addon_desc->prefix, grp, "Prefix");
    if (!addon_desc->prefix) {
        FcitxLog(ERROR, "Entry \"Prefix\" not found in [FcitxAddon] group.");
        return false;
    }
    addon_desc->self_type = fxscanner_group_get_type(grp, "Self.Type");
    fxscanner_load_entry_list(&addon_desc->includes, grp, "Include", false);
    if (!fxscanner_load_entry_list(&addon_desc->functions, grp, "Function",
                                   true, fxscanner_func_loader, addon_desc))
        return false;
    if (!fxscanner_load_entry_list(&addon_desc->macros, grp, "Macro", true,
                                   fxscanner_macro_loader, addon_desc))
        return false;
    return true;
}

/**
 * write public
 **/

static void
fxscanner_write_header_public(FILE *ofp)
{
    _write_str(ofp, fxscanner_header_str);
}

static void
fxscanner_macro_write_public(FcitxAddonMacroDesc *macro_desc, FILE *ofp)
{
    _write_strings(ofp,
                   "#ifdef ", macro_desc->name, "\n"
                   "#  undef ", macro_desc->name, "\n"
                   "#endif\n");
    if (macro_desc->define) {
        _write_strings(ofp, "#define ", macro_desc->name);
        if (macro_desc->value && *macro_desc->value) {
            _write_strings(ofp, " ", macro_desc->value);
        }
        _write_str(ofp, "\n");
    }
}

static void
fxscanner_includes_write_public(UT_array *includes, FILE *ofp)
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
        _write_strings(ofp, "\n#include ", *p);
    }
    _write_str(ofp, "\n\n");
}

static void
fxscanner_function_write_public(FcitxAddonFuncDesc *func_desc,
                                FcitxAddonDesc *addon_desc, int id, FILE *ofp)
{
    if (func_desc->cache && !func_desc->type) {
        FcitxLog(WARNING, "Cannot cache result of type void.");
        func_desc->cache = false;
    }
    if (func_desc->err_ret && !func_desc->type) {
        FcitxLog(WARNING, "Cannot set error return of type void.");
        func_desc->err_ret = false;
    }
    if (!func_desc->err_ret) {
        _write_strings(ofp, "DEFINE_GET_AND_INVOKE_FUNC(",
                       addon_desc->prefix, ", ", func_desc->name, ", ");
        fprintf(ofp, "%d", id);
        _write_str(ofp, ")\n");
    } else {
        _write_strings(ofp, "DEFINE_GET_AND_INVOKE_FUNC_WITH_TYPE_AND_ERROR(",
                       addon_desc->prefix, ", ", func_desc->name, ", ");
        fprintf(ofp, "%d", id);
        _write_strings(ofp, ", ", func_desc->type,
                       ", ", func_desc->err_ret, ")\n");
    }
    if (!func_desc->enable_wrapper)
        _write_str(ofp, "#if 0\n");
    _write_strings(ofp,
                   "static inline ",
                   func_desc->type ? func_desc->type : "void", "\n"
                   "Fcitx", addon_desc->prefix, func_desc->name,
                   "(FcitxInstance *instance");
    unsigned int i;
    FcitxAddonArgDesc *arg_desc;
    for (i = 0;i < utarray_len(&func_desc->args);i++) {
        arg_desc = (FcitxAddonArgDesc*)_utarray_eltptr(&func_desc->args, i);
        _write_strings(ofp, ", ", arg_desc->type, " _arg");
        fprintf(ofp, "%d", i);
    }
    _write_str(ofp,
               ")\n"
               "{\n");
    if (func_desc->cache) {
        _write_strings(ofp,
                       "    static FcitxInstance *_instance = NULL;\n"
                       "    static void *result = NULL;\n"
                       "    if (fcitx_likely(_instance == instance))\n"
                       "        FCITX_RETURN_FROM_PTR(", func_desc->type,
                       ", result);\n"
                       "    _instance = instance;\n");
    } else if (func_desc->type) {
        _write_str(ofp, "    void *result;\n");
    }
    for (i = 0;i < utarray_len(&func_desc->args);i++) {
        arg_desc = (FcitxAddonArgDesc*)_utarray_eltptr(&func_desc->args, i);
        _write_str(ofp, "    FCITX_DEF_CAST_TO_PTR(arg");
        fprintf(ofp, "%d", i);
        _write_strings(ofp, ", ", arg_desc->type, ", _arg");
        fprintf(ofp, "%d", i);
        _write_str(ofp, ");\n");
    }
    _write_str(ofp, "    FCITX_DEF_MODULE_ARGS(args");
    for (i = 0;i < utarray_len(&func_desc->args);i++) {
        _write_str(ofp, ", arg");
        fprintf(ofp, "%d", i);
    }
    _write_str(ofp,
               ");\n"
               "    ");
    if (func_desc->type) {
        _write_str(ofp, "result = ");
    }
    _write_strings(ofp, "Fcitx", addon_desc->prefix, "Invoke", func_desc->name,
                   "(instance, args);\n");
    if (func_desc->type) {
        _write_strings(ofp, "    FCITX_RETURN_FROM_PTR(",
                       func_desc->type, ", result);\n");
    }
    if (func_desc->enable_wrapper) {
        _write_str(ofp, "}\n\n");
    } else {
        _write_str(ofp,
                   "}\n"
                   "#endif\n\n");
    }
}

static boolean
fxscanner_addon_write_public(FcitxAddonDesc *addon_desc, FILE *ofp)
{
    fxscanner_write_header_public(ofp);
    char *buff = fcitx_utils_set_str_with_len(NULL, addon_desc->name,
                                              addon_desc->name_len);
    fxscanner_name_to_macro(buff);
    _write_strings(ofp, "\n#ifndef __FCITX_MODULE_", buff, "_API_H\n",
                   "#define __FCITX_MODULE_", buff, "_API_H\n\n");
    unsigned int i;
    FcitxAddonMacroDesc *macro_desc;
    for (i = 0;i < utarray_len(&addon_desc->macros);i++) {
        macro_desc = (FcitxAddonMacroDesc*)_utarray_eltptr(
            &addon_desc->macros, i);
        fxscanner_macro_write_public(macro_desc, ofp);
    }
    fxscanner_includes_write_public(&addon_desc->includes, ofp);
    _write_strings(ofp,
                   "#ifdef __cplusplus\n"
                   "extern \"C\" {\n"
                   "#endif\n\n"
                   "DEFINE_GET_ADDON(\"", addon_desc->name, "\", ",
                   addon_desc->prefix, ")\n\n");
    FcitxAddonFuncDesc *func_desc;
    for (i = 0;i < utarray_len(&addon_desc->functions);i++) {
        func_desc = (FcitxAddonFuncDesc*)_utarray_eltptr(
            &addon_desc->functions, i);
        fxscanner_function_write_public(func_desc, addon_desc, i, ofp);
    }
    _write_str(ofp,
               "\n"
               "#ifdef __cplusplus\n"
               "}\n"
               "#endif\n"
               "\n"
               "#endif\n");
    free(buff);
    return true;
}

/**
 * write private
 **/

static void
fxscanner_includes_write_private(FILE *ofp)
{
    _write_str(ofp,
               "#include <stdint.h>\n"
               "#include <fcitx/fcitx.h>\n"
               "#include <fcitx-utils/utils.h>\n"
               "#include <fcitx/instance.h>\n"
               "#include <fcitx/addon.h>\n"
               "#include <fcitx/module.h>\n\n");
}

static boolean
fxscanner_function_check_private(FcitxAddonFuncDesc *func_desc)
{
    if (!func_desc->res_deref && func_desc->res_dereftype) {
        FcitxLog(ERROR, "Res.Deref must be set if Res.DerefType is set.");
        return false;
    }
    if (FXSCANNER_XOR(func_desc->self_deref, func_desc->self_dereftype)) {
        FcitxLog(ERROR, "Self.Deref and Self.DerefType must be both"
                 "set or not set.");
        return false;
    }
    if (func_desc->is_static && !func_desc->res_wrapfunc) {
        FcitxLog(WARNING, "Static doesn't make sense without Res.WrapFunc.");
    }
    if (!FXSCANNER_XOR(func_desc->res_exp, func_desc->res_wrapfunc)) {
        FcitxLog(ERROR, "One and only one of Res.Exp and Res.WrapFunc "
                 "must be set.");
        return false;
    }
    return true;
}

static boolean
fxscanner_arg_check_private(FcitxAddonArgDesc *arg_desc)
{
    if (!arg_desc->deref && arg_desc->deref_type) {
        FcitxLog(ERROR, "Arg*.Deref must be set if Arg*.DerefType is set.");
        return false;
    }
    return true;
}

typedef struct {
    const char *key;
    const char *value;
} FxScannerTranslateMap;
typedef boolean (*FxScannerTranslatorNumFilter)(FILE *ofp, int num, void *data);
typedef struct {
    int max;
    FxScannerTranslatorNumFilter filter;
    void *data;
    const FxScannerTranslateMap *map;
} FxScannerNumTranslate;

static int
fxscanner_num_translator(FILE *ofp, const char *str, void *data)
{
    const FxScannerNumTranslate *tran = data;
    const FxScannerTranslateMap *map = tran->map;
    for (;map->key;map++) {
        int len = strlen(map->key);
        if (strncmp(str, map->key, len) == 0) {
            if (map->value)
                _write_str(ofp, map->value);
            return len;
        }
    }
    char *end;
    int id = strtol(str, &end, 10);
    if (end <= str) {
        FcitxLog(ERROR, "Cannot parse deref expression at %s", str);
        return -1;
    }
    if (tran->max >= 0 && id >= tran->max) {
        FcitxLog(ERROR, "%d in the deref expression is larger than "
                 "the number of arguments.", id);
        return -1;
    }
    if (!tran->filter || tran->filter(ofp, id, tran->data))
        return end - str;
    return -1;
}

static boolean
fxscanner_write_translate_num(FILE *ofp, const char *str,
                              const FxScannerNumTranslate *tran,
                              const char *delim)
{
    return fxscanner_write_translate(ofp, str, fxscanner_num_translator,
                                     (void*)(intptr_t)tran, delim);
}

typedef struct {
    const FcitxAddonFuncDesc *func_desc;
    const char *arg_prefix;
    const char *deref_prefix;
} FxScannerExpData;

static boolean
fxscanner_trans_exp_filter(FILE *ofp, int id, void *_data)
{
    const FxScannerExpData *data = _data;
    FcitxAddonArgDesc *arg_desc;
    arg_desc = (FcitxAddonArgDesc*)_utarray_eltptr(&data->func_desc->args, id);
    if (data->deref_prefix) {
        if (!arg_desc->deref) {
            _write_strings(ofp, "(", data->arg_prefix);
        } else {
            _write_strings(ofp, "(", data->deref_prefix);
        }
    } else {
        if (arg_desc->deref && !arg_desc->deref_type) {
            FcitxLog(ERROR, "Refer to void argument $%d in the expression", id);
            return false;
        }
        _write_strings(ofp, "(", data->arg_prefix);
    }
    fprintf(ofp, "%d", id);
    _write_str(ofp, ")");
    return true;
}

static boolean
fxscanner_write_translate_exp(FILE *ofp, const char *str,
                              const FcitxAddonFuncDesc *func_desc,
                              const char *self_str, const char *res_str,
                              const char *arg_prefix, const char *deref_prefix)
{
    FxScannerExpData data = {
        .func_desc = func_desc,
        .arg_prefix = arg_prefix,
        .deref_prefix = deref_prefix
    };
    FxScannerTranslateMap map[] = {{
            .key = "<",
            .value = self_str
        }, {
            .key = res_str ? "@" : NULL,
            .value = res_str
        }
    };
    FxScannerNumTranslate tran = {
        .max = utarray_len(&func_desc->args),
        .filter = fxscanner_trans_exp_filter,
        .data = &data,
        .map = map
    };
    return fxscanner_write_translate_num(ofp, str, &tran, "$");
}

static boolean
fxscanner_write_deref(FILE *ofp, const char *prefix, const char *deref,
                      const char *deref_type,
                      const FcitxAddonFuncDesc *func_desc,
                      const char *self_str, const char *res_str,
                      const char *arg_prefix, const char *deref_prefix,
                      const char *res_fmt, const char *orig_fmt, ...)
{
    va_list ap;
    _write_strings(ofp, prefix);
    if (deref_type) {
        _write_strings(ofp, deref_type, " ");
        va_start(ap, orig_fmt);
        vfprintf(ofp, res_fmt, ap);
        va_end(ap);
        _write_strings(ofp, " = (");
    }
    int level;
    if (fxscanner_parse_int(deref, &level)) {
        if (!deref_type) {
            FcitxLog(ERROR, "Trying to Deref %d level without a DerefType.",
                     level);
            return false;
        }
        if (level < -1) {
            FcitxLog(ERROR, "Invalid Deref expression \"%d\"", level);
            return false;
        } else if (level == -1) {
            _write_strings(ofp, "&");
        } else {
            int j;
            for (j = 0;j < level;j++) {
                _write_str(ofp, "*");
            }
        }
        _write_strings(ofp, "(");
        va_start(ap, orig_fmt);
        vfprintf(ofp, orig_fmt, ap);
        va_end(ap);
        _write_strings(ofp, ")");
    } else if (!fxscanner_write_translate_exp(ofp, deref, func_desc, self_str,
                                              res_str, arg_prefix,
                                              deref_prefix)) {
        return false;
    }
    if (deref_type)
        _write_strings(ofp, ")");
    _write_strings(ofp, ";\n");
    return true;
}

static boolean
fxscanner_function_write_private(FcitxAddonFuncDesc *func_desc,
                                 FcitxAddonDesc *addon_desc,
                                 FcitxAddonBuff *buff, FILE *ofp)
{
    if (!fxscanner_function_check_private(func_desc))
        return false;
    FCITX_UNUSED(buff);

    /**
     * function definition
     * casting self to correct type
     **/
    _write_strings(ofp,
                   "static void*\n"
                   "__fcitx_", addon_desc->prefix, "_function_",
                   func_desc->name,
                   "(void *_self, FcitxModuleFunctionArg _args)\n"
                   "{\n"
                   "    FCITX_DEF_CAST_FROM_PTR(", addon_desc->self_type,
                   ", __self, _self);\n");

    /**
     * casting arguments to correct types
     **/
    if (!utarray_len(&func_desc->args))
        _write_strings(ofp, "    FCITX_UNUSED(_args);\n");
    unsigned int i;
    FcitxAddonArgDesc *arg_desc;
    for (i = 0;i < utarray_len(&func_desc->args);i++) {
        arg_desc = (FcitxAddonArgDesc*)_utarray_eltptr(&func_desc->args, i);
        if (!fxscanner_arg_check_private(arg_desc))
            return false;
        _write_strings(ofp, "    FCITX_DEF_CAST_FROM_PTR(", arg_desc->type,
                       arg_desc->deref ? ", _arg" : ", arg");
        fprintf(ofp, "%d", i);
        _write_strings(ofp, ", _args.args[");
        fprintf(ofp, "%d", i);
        _write_strings(ofp, "]);\n");
    }

    /**
     * deref self
     **/
    if (!func_desc->self_deref){
        _write_strings(ofp, "    ", addon_desc->self_type, " self = __self;\n");
    } else if (!fxscanner_write_deref(ofp, "    ", func_desc->self_deref,
                                      func_desc->self_dereftype, func_desc,
                                      "(__self)", NULL, "arg", "_arg", "self",
                                      "__self")) {
        return false;
    }
    _write_strings(ofp, "    FCITX_UNUSED(self);\n");
    /**
     * deref arguments
     **/
    for (i = 0;i < utarray_len(&func_desc->args);i++) {
        arg_desc = (FcitxAddonArgDesc*)_utarray_eltptr(&func_desc->args, i);
        if (arg_desc->deref) {
            if (!arg_desc->deref_type && func_desc->res_wrapfunc) {
                FcitxLog(ERROR, "Using arg%d of DerefType void with "
                         "Res.WrapFunc.", i);
                return false;
            }
            if (!fxscanner_write_deref(ofp, "    ", arg_desc->deref,
                                      arg_desc->deref_type, func_desc,
                                      "(__self)", NULL, "arg", "_arg", "arg%d",
                                       "_arg%d", i)) {
                return false;
            }
        }
    }

    /**
     * inline code
     **/
    if (func_desc->inline_code) {
        _write_strings(ofp, "    ");
        if (!fxscanner_write_translate_exp(ofp, func_desc->inline_code,
                                           func_desc, "(self)", NULL, "arg",
                                           NULL))
            return false;
        _write_strings(ofp, ";\n");
    }

    /**
     * result expresion
     **/
    const char *res_type = (func_desc->res_deref ? func_desc->res_dereftype :
                            func_desc->type);
    _write_strings(ofp, "    ");
    if (res_type)
        _write_strings(ofp, res_type, " res = (");
    if (func_desc->res_wrapfunc) {
        _write_strings(ofp, func_desc->res_wrapfunc, "(");
        if (!func_desc->is_static) {
            if (utarray_len(&func_desc->args)) {
                _write_strings(ofp, "self, ");
            } else {
                _write_strings(ofp, "self");
            }
        }
        for (i = 0;i < utarray_len(&func_desc->args);i++) {
            if (i != 0)
                _write_strings(ofp, ", ");
            _write_strings(ofp, "arg");
            fprintf(ofp, "%d", i);
        }
        _write_strings(ofp, ")");
    } else if (!fxscanner_write_translate_exp(ofp, func_desc->res_exp,
                                              func_desc, "(self)", NULL, "arg",
                                              NULL)) {
        return false;
    }
    if (res_type)
        _write_strings(ofp, ")");
    _write_strings(ofp, ";\n");

    if (func_desc->res_deref &&
        !fxscanner_write_deref(ofp, "    ", func_desc->res_deref,
                               func_desc->type, func_desc,"(self)", res_type ?
                               "(res)" : NULL, "arg", NULL, "_res", "res")) {
            return false;
    }
    if (func_desc->type) {
        _write_strings(ofp, "    FCITX_RETURN_AS_PTR(", func_desc->type,
                       ", ", func_desc->res_deref ? "_res" : "res", ");\n");
    } else {
        _write_strings(ofp, "    return NULL;\n");
    }
    _write_strings(ofp, "}\n\n");
    return true;
}

static boolean
fxscanner_addon_check_private(FcitxAddonDesc *addon_desc)
{
    if (!addon_desc->self_type) {
        FcitxLog(ERROR, "Entry Self.Type must be provided in group "
                 "[FcitxAddon]");
        return false;
    }
    return true;
}

static boolean
fxscanner_addon_write_private(FcitxAddonDesc *addon_desc, FILE *ofp)
{
    if (!fxscanner_addon_check_private(addon_desc))
        return false;
    FcitxAddonBuff buff = {0, 0};
    fxaddon_buff_realloc(&buff, addon_desc->name_len + 1);
    memcpy(buff.buff, addon_desc->name, addon_desc->name_len + 1);
    fxscanner_name_to_macro(buff.buff);
    fxscanner_includes_write_private(ofp);
    _write_strings(ofp, "\n#ifndef __FCITX_MODULE_", buff.buff,
                   "_ADD_FUNCTION_H\n",
                   "#define __FCITX_MODULE_", buff.buff, "_ADD_FUNCTION_H\n\n"
                   "#ifdef __cplusplus\n"
                   "extern \"C\" {\n"
                   "#endif\n\n");
    /**
     * Change the prefix here to avoid any possible name conflict
     **/
    _write_strings(ofp, "DEFINE_GET_ADDON(\"", addon_desc->name, "\", _",
                   addon_desc->prefix, "_)\n\n");

    FcitxAddonFuncDesc *func_desc;
    unsigned int i;
    for (i = 0;i < utarray_len(&addon_desc->functions);i++) {
        func_desc = (FcitxAddonFuncDesc*)_utarray_eltptr(
            &addon_desc->functions, i);
        if (!fxscanner_function_write_private(func_desc, addon_desc,
                                              &buff, ofp)) {
            free(buff.buff);
            return false;
        }
    }
    _write_strings(ofp,
                   "static void\n"
                   "Fcitx", addon_desc->prefix,
                   "AddFunctions(FcitxInstance *instance)\n"
                   "{\n"
                   "    int i;\n"
                   "    FcitxAddon *addon = Fcitx_", addon_desc->prefix,
                   "_GetAddon(instance);\n");
    _write_strings(ofp,
                   "    static const FcitxModuleFunction ____fcitx_",
                   addon_desc->prefix, "_addon_functions_table[] = {\n");
    for (i = 0;i < utarray_len(&addon_desc->functions);i++) {
        func_desc = (FcitxAddonFuncDesc*)_utarray_eltptr(
            &addon_desc->functions, i);
        _write_strings(ofp, "        __fcitx_", addon_desc->prefix,
                       "_function_", func_desc->name, ",\n");
    }
    _write_strings(ofp,
                   "    };\n"
                   "    for (i = 0;i < ");
    fprintf(ofp, "%u", (unsigned)utarray_len(&addon_desc->functions));
    _write_strings(ofp,
                   ";i++) {\n"
                   "        FcitxModuleAddFunction(addon, ____fcitx_",
                   addon_desc->prefix, "_addon_functions_table[i]);\n"
                   "    }\n"
                   "}\n\n"
                   "#ifdef __cplusplus\n"
                   "}\n"
                   "#endif\n"
                   "\n"
                   "#endif\n");
    free(buff.buff);
    return true;
}

static int
fxscanner_scan_addon(const char *action, FILE *ifp, FILE *ofp)
{
    FcitxAddonDesc addon_desc;
    int res = 0;
    fxscanner_addon_init(&addon_desc);
    if (!fxscanner_addon_load(&addon_desc, ifp)) {
        res = 1;
        goto out;
    }
    if (strcmp(action, "--api") == 0) {
        if (!fxscanner_addon_write_public(&addon_desc, ofp)) {
            res = 1;
            goto out;
        }
    } else if (strcmp(action, "--private") == 0) {
        if (!fxscanner_addon_write_private(&addon_desc, ofp)) {
            res = 1;
            goto out;
        }
    } else {
        res = 1;
    }
out:
    fxscanner_addon_done(&addon_desc);
    fclose(ifp);
    fclose(ofp);
    return res;
}

int
main(int argc, char *argv[])
{
    if (argc != 4) {
        FcitxLog(ERROR, "Wrong number of arguments.");
        exit(1);
    }
    FILE *ifp = fopen(argv[2], "r");
    if (!ifp) {
        FcitxLog(ERROR, "Cannot open file %s for reading.", argv[2]);
        exit(1);
    }
    FILE *ofp = fopen(argv[3], "w");
    if (!ofp) {
        FcitxLog(ERROR, "Cannot open file %s for writing.", argv[3]);
        exit(1);
    }
    int res = fxscanner_scan_addon(argv[1], ifp, ofp);
    if (res != 0)
        unlink(argv[3]);
    return res;
}

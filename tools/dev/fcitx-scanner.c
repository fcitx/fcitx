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

static const char *fxaddon_copyright_str =
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

static const char*
fxaddon_function_get_return(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "Return");
    if (!ety)
        return NULL;
    const char *type = ety->value;
    if (!type)
        return NULL;
    type += strspn(type, " \b\f\v\r\t");
    if (fxscanner_strs_strip_cmp(ety->value, false, "void", NULL) != 0)
        return NULL;
    return type;
}

static const char*
fxaddon_function_get_error_return(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "ErrorReturn");
    if (!ety)
        return NULL;
    return ety->value;
}

static boolean
fxaddon_function_get_cache_result(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "CacheResult");
    if (!ety)
        return false;
    const char *cache = ety->value;
    return fxscanner_strs_strip_cmp(cache, true, "on", "true",
                                    "yes", "1", NULL) > 0;
}

static boolean
fxaddon_function_get_enable_wrapper(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "EnableWrapper");
    if (!ety)
        return true;
    const char *enable = ety->value;
    return !(fxscanner_strs_strip_cmp(enable, true, "off", "false",
                                      "no", "0", NULL) > 0);
}

static void
fxaddon_load_numbered_entries(UT_array *ret, FcitxDesktopGroup *grp,
                              const char *prefix, boolean stop_at_empty)
{
    utarray_init(ret, fcitx_ptr_icd);
    size_t prefix_len = strlen(prefix);
    char *buff = malloc(prefix_len + FCITX_INT_LEN + 1);
    memcpy(buff, prefix, prefix_len);
    char *num_start = buff + prefix_len;
    int i;
    FcitxDesktopEntry *tmp_ety;
    for (i = 0;;i++) {
        sprintf(num_start, "%d", i);
        tmp_ety = fcitx_desktop_group_find_entry(grp, buff);
        if (!tmp_ety)
            break;
        if (!(tmp_ety->value && *tmp_ety->value)) {
            if (stop_at_empty) {
                break;
            } else {
                continue;
            }
        }
        utarray_push_back(ret, &tmp_ety->value);
    }
    free(buff);
}

static void
fxaddon_write_copyright(FILE *ofp)
{
    _write_str(ofp, fxaddon_copyright_str);
}

static void
fxaddon_name_to_macro(char *name)
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

static void
fxaddon_write_includes(FILE *ofp, UT_array *includes)
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

static boolean
fxaddon_macro_get_define(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "Define");
    if (ety) {
        return fxscanner_strs_strip_cmp(ety->value, true, "on",
                                        "true", "yes", "1", NULL) > 0;
    }
    ety = fcitx_desktop_group_find_entry(grp, "Undefine");
    if (ety) {
        return !(fxscanner_strs_strip_cmp(ety->value, true, "on",
                                          "true", "yes", "1", NULL) > 0);
    }
    return true;
}

static const char*
fxaddon_macro_get_value(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "Value");
    if (!ety)
        return NULL;
    return ety->value;
}

static void
fxaddon_write_macro(FILE *ofp, FcitxDesktopFile *dfile, const char *macro_name)
{
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(dfile, macro_name);
    if (!grp)
        return;
    const char *value = fxaddon_macro_get_value(grp);
    boolean define = fxaddon_macro_get_define(grp);
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
fxaddon_write_function(FILE *ofp, FcitxDesktopFile *dfile, const char *prefix,
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
    fxaddon_load_numbered_entries(&args, grp, "Arg", true);
    const char *type = fxaddon_function_get_return(grp);
    const char *err_ret = fxaddon_function_get_error_return(grp);
    boolean cache = fxaddon_function_get_cache_result(grp);
    boolean enable_wrapper = fxaddon_function_get_enable_wrapper(grp);
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
        _write_str(ofp, " arg");
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
                   "        return (");
        _write_str(ofp, type);
        _write_str(ofp,
                   ")(intptr_t)result;\n"
                   "    _init = true;\n");
    } else if (type) {
        _write_str(ofp, "    void *result;\n");
    }
    _write_str(ofp, "    FCITX_DEF_MODULE_ARGS(args");
    for (i = 0;i < utarray_len(&args);i++) {
        _write_str(ofp, ", (void*)(intptr_t)arg");
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
        _write_str(ofp, "    return (");
        _write_str(ofp, type);
        _write_str(ofp, ")(intptr_t)result;\n");
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

static int
fxaddon_scan_addon(FILE *ifp, FILE *ofp)
{
    FcitxDesktopFile dfile;
    char *buff = NULL;
    unsigned int i;
    char **p;
    if (!fcitx_desktop_file_init(&dfile, NULL, NULL))
        return 1;
    if (!fcitx_desktop_file_load_fp(&dfile, ifp))
        return 1;
    fclose(ifp);
    FcitxDesktopGroup *addon_grp;
    FcitxDesktopEntry *tmp_ety;
    addon_grp = fcitx_desktop_file_find_group(&dfile, "FcitxAddon");
    if (!addon_grp)
        return 1;
    tmp_ety = fcitx_desktop_group_find_entry(addon_grp, "Name");
    if (!tmp_ety)
        return 1;
    const char *name = tmp_ety->value;
    tmp_ety = fcitx_desktop_group_find_entry(addon_grp, "Prefix");
    if (!tmp_ety)
        return 1;
    const char *prefix = tmp_ety->value;
    UT_array macros;
    fxaddon_load_numbered_entries(&macros, addon_grp, "Macro", false);
    UT_array includes;
    fxaddon_load_numbered_entries(&includes, addon_grp, "Include", false);
    UT_array functions;
    fxaddon_load_numbered_entries(&functions, addon_grp, "Function", true);
    fxaddon_write_copyright(ofp);
    size_t name_len = strlen(name);
    buff = fcitx_utils_set_str_with_len(buff, name, name_len);
    fxaddon_name_to_macro(buff);
    _write_str(ofp, "\n#ifndef __FCITX_MODULE_");
    _write_len(ofp, buff, name_len);
    _write_str(ofp, "_H\n");
    _write_str(ofp, "#define __FCITX_MODULE_");
    _write_len(ofp, buff, name_len);
    _write_str(ofp, "_H\n"
                    "\n"
                    "#ifdef __cplusplus\n"
                    "extern \"C\" {\n"
                    "#endif\n"
                    "\n");
    for (i = 0;i < utarray_len(&macros);i++) {
        p = (char**)_utarray_eltptr(&macros, i);
        fxaddon_write_macro(ofp, &dfile, *p);
    }
    fxaddon_write_includes(ofp, &includes);
    utarray_done(&includes);
    _write_str(ofp, "DEFINE_GET_ADDON(\"");
    _write_len(ofp, name, name_len);
    _write_str(ofp, "\", ");
    _write_str(ofp, prefix);
    _write_str(ofp, ")\n\n");
    for (i = 0;i < utarray_len(&functions);i++) {
        p = (char**)_utarray_eltptr(&functions, i);
        fxaddon_write_function(ofp, &dfile, prefix, *p, i);
    }
    _write_str(ofp, "\n"
                    "#ifdef __cplusplus\n"
                    "}\n"
                    "#endif\n"
                    "\n"
                    "#endif\n");
    fclose(ofp);
    fcitx_utils_free(buff);
    fcitx_desktop_file_done(&dfile);
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
    return fxaddon_scan_addon(ifp, ofp);
}

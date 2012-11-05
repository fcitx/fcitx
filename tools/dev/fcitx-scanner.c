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
#include <unistd.h>
#include <string.h>
#include <fcitx-utils/desktop-parse.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utarray.h>

static const char *copyright_str =
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

static const UT_icd const_str_icd = {
    sizeof(const char*), NULL, NULL, NULL
};

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

static const char*
get_return_type(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "Return");
    if (!ety)
        return NULL;
    const char *type = ety->value;
    if (!type)
        return NULL;
    type += strspn(type, " \b\f\v\r\t");
    if (!*type)
        return NULL;
    if (strncmp(type, "void", strlen("void")) &&
        !type[strlen("void") + strspn(type + strlen("void"), " \b\f\v\r\t")])
        return NULL;
    return type;
}

static const char*
get_error_return(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "ErrorReturn");
    if (!ety)
        return NULL;
    return ety->value;
}

static inline int
str_case_cmp_len(const char *str1, const char *str2)
{
    int len = strlen(str2);
    if (!strncasecmp(str1, str2, len))
        return len;
    return 0;
}

static boolean
get_cache_result(FcitxDesktopGroup *grp)
{
    FcitxDesktopEntry *ety;
    ety = fcitx_desktop_group_find_entry(grp, "CacheResult");
    if (!ety)
        return false;
    const char *cache = ety->value;
    if (!cache)
        return false;
    cache += strspn(cache, " \b\f\v\r\t");
    if (!*cache)
        return false;
    int len;
    if ((len = str_case_cmp_len(cache, "on")) ||
        (len = str_case_cmp_len(cache, "true")) ||
        (len = str_case_cmp_len(cache, "yes")) ||
        (len = str_case_cmp_len(cache, "1"))) {
        cache += len;
        cache += strspn(cache, " \b\f\v\r\t");
        if (!*cache) {
            return true;
        }
    }
    return false;
}

static void
load_includes(UT_array *ret, FcitxDesktopGroup *addon_grp)
{
    utarray_init(ret, &const_str_icd);
    char include_buff[strlen("Include") + FCITX_INT_LEN + 1];
    strcpy(include_buff, "Include");
    int i;
    FcitxDesktopEntry *tmp_ety;
    for (i = 0;;i++) {
        sprintf(include_buff + strlen("Include"), "%d", i);
        tmp_ety = fcitx_desktop_group_find_entry(addon_grp, include_buff);
        if (!tmp_ety)
            break;
        utarray_push_back(ret, &tmp_ety->value);
    }
}

static void
load_functions(UT_array *ret, FcitxDesktopGroup *addon_grp)
{
    utarray_init(ret, &const_str_icd);
    char function_buff[strlen("Function") + FCITX_INT_LEN + 1];
    strcpy(function_buff, "Function");
    int i;
    FcitxDesktopEntry *tmp_ety;
    for (i = 0;;i++) {
        sprintf(function_buff + strlen("Function"), "%d", i);
        tmp_ety = fcitx_desktop_group_find_entry(addon_grp, function_buff);
        if (!(tmp_ety && tmp_ety->value && *tmp_ety->value))
            break;
        utarray_push_back(ret, &tmp_ety->value);
    }
}

static void
write_copyright(FILE *ofp)
{
    _write_str(ofp, copyright_str);
}

static void
name_to_macro(char *name)
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
write_includes(FILE *ofp, UT_array *includes)
{
    _write_str(ofp, "#include <stdint.h>\n");
    _write_str(ofp, "#include <fcitx/instance.h>\n");
    _write_str(ofp, "#include <fcitx/addon.h>\n");
    _write_str(ofp, "#include <fcitx/module.h>");
    char **p;
    for (p = (char**)utarray_front(includes);p;
         p = (char**)utarray_next(includes, p)) {
        _write_str(ofp, "\n#include ");
        _write_str(ofp, *p);
    }
    _write_str(ofp, "\n\n");
}

static void
write_function(FILE *ofp, FcitxDesktopFile *dfile, const char *prefix,
               const char *func_name, int id)
{
    FcitxDesktopGroup *grp;
    grp = fcitx_desktop_file_find_group(dfile, func_name);
    if (!grp)
        return;
    /* require the Name entry although not used now. */
    if (!fcitx_desktop_group_find_entry(grp, "Name"))
        return;
    UT_array args;
    utarray_init(&args, &const_str_icd);
    char arg_buff[strlen("Arg") + FCITX_INT_LEN + 1];
    strcpy(arg_buff, "Arg");
    int i;
    FcitxDesktopEntry *tmp_ety;
    for (i = 0;;i++) {
        sprintf(arg_buff + strlen("Arg"), "%d", i);
        tmp_ety = fcitx_desktop_group_find_entry(grp, arg_buff);
        if (!(tmp_ety && tmp_ety->value && *tmp_ety->value))
            break;
        utarray_push_back(&args, &tmp_ety->value);
    }
    const char *type = get_return_type(grp);
    const char *err_ret = get_error_return(grp);
    boolean cache = get_cache_result(grp);
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
    _write_str(ofp, "}\n\n");
    utarray_done(&args);
}


static int
scan_fxaddon(FILE *ifp, FILE *ofp)
{
    FcitxDesktopFile dfile;
    char *buff = NULL;
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
    UT_array includes;
    load_includes(&includes, addon_grp);
    UT_array functions;
    load_functions(&functions, addon_grp);
    write_copyright(ofp);
    size_t name_len = strlen(name);
    buff = fcitx_utils_set_str_with_len(buff, name, name_len);
    name_to_macro(buff);
    _write_str(ofp, "\n#ifndef __FCITX_MODULE_");
    _write_len(ofp, buff, name_len);
    _write_str(ofp, "_H\n");
    _write_str(ofp, "#define __FCITX_MODULE_");
    _write_len(ofp, buff, name_len);
    _write_str(ofp, "_H\n");
    write_includes(ofp, &includes);
    utarray_done(&includes);
    _write_str(ofp, "DEFINE_GET_ADDON(\"");
    _write_len(ofp, name, name_len);
    _write_str(ofp, "\", ");
    _write_str(ofp, prefix);
    _write_str(ofp, ")\n\n");
    int i;
    char **p;
    for (i = 0;i < utarray_len(&functions);i++) {
        p = (char**)_utarray_eltptr(&functions, i);
        write_function(ofp, &dfile, prefix, *p, i);
    }
    _write_str(ofp, "#endif\n");
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
    return scan_fxaddon(ifp, ofp);
}

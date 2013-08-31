/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
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

#ifndef _FCITX_CONFIGURE_H_
#define _FCITX_CONFIGURE_H_

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcitx-utils/utils.h>
#include <fcitx-config/config-desc.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _FcitxConfig FcitxConfig;
    typedef struct _FcitxConfigGroup FcitxConfigGroup;
    typedef struct {
        FcitxConfigValue value;
        const FcitxConfigDescOption *desc;
    } FcitxConfigOption;

    typedef struct {
        boolean (*load)(FcitxConfig *config, const char *type,
                        const char *name);

        // bool?..
        boolean (*save)(FcitxConfig *config, const char *type,
                        const char *name);

        void (*free_group_data)(FcitxConfigGroup*, void *data);
        void (*free_option_data)(FcitxConfigOption*, void *data);
    } FcitxConfigBackend;

    /* Private struct defined in .c */
    typedef struct {
        FcitxConfigOption option;
        // ...
        FcitxConfigBackend *backend;
        void *data;
    } FcitxConfigOptionPrivate;
    /* end of private struct */

    FcitxConfig *fcitx_config_new(FcitxConfigDesc *desc);
    FcitxConfigGroup *fcitx_config_get_group(FcitxConfig *config,
                                             const char *grp_name);
    FcitxConfigGroup *fcitx_config_group_get_option(FcitxConfigGroup *grp,
                                                    const char *opt_name);

    // ref unref functions

    // cache result
    void *fcitx_config_group_get_data(FcitxConfigGroup *grp,
                                      FcitxConfigBackend *backend);
    void *fcitx_config_option_get_data(FcitxConfigOption *option,
                                       FcitxConfigBackend *backend);
    void fcitx_config_group_set_data(FcitxConfigGroup *grp,
                                     FcitxConfigBackend *backend,
                                     void *data);
    void fcitx_config_option_set_data(FcitxConfigOption *option,
                                      FcitxConfigBackend *backend,
                                      void *data);

    void fcitx_config_value_copy(const FcitxConfigValue *from,
                                 FcitxConfigValue *to);
#define DEF_SET_CONFIG_FUNC(name, type_id, val_type, field)             \
    static inline void fcitx_config_set_##name(FcitxConfigValue *config, \
                                               val_type val)            \
    {                                                                   \
        FcitxConfigValue from;                                          \
        from.type = type_id;                                            \
        from.field = val;                                               \
        fcitx_config_value_copy(&from, config);                         \
    }

    DEF_SET_CONFIG_FUNC(int, T_Integer, int, integer);
    DEF_SET_CONFIG_FUNC(bool, T_Bool, bool, boolvalue);
    DEF_SET_CONFIG_FUNC(hotkey, T_Hotkey, FcitxHotkey*, hotkey);
    // ...

    // priority (or in struct)
    int _fcitx_config_reg_backend(FcitxConfigManager *mgr,
                                  const FcitxConfigBackend *backend,
                                  size_t struct_size, void *data);
#define __fcitx_config_reg_backend(mgr, backend, data, ...)             \
    _fcitx_config_reg_backend(mgr, backend,                             \
                              (sizeof(FcitxConfigBackend)), data)
#define fcitx_config_reg_backend(mgr, backend, data...)         \
    __fcitx_config_reg_backend(mgr, backend, ##data, NULL)

    // With default backend(s)
    FcitxConfigManager *fcitx_config_manager_new();

#ifdef __cplusplus
}
#endif

#endif

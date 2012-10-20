/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef CONNECTION_IM_H
#define CONNECTION_IM_H


#include <gio/gio.h>

/*
 * Type macros
 */

/* define GOBJECT macros */
#define FCITX_TYPE_CONNECTION (fcitx_connection_get_type())
#define FCITX_CONNECTION(o) \
        (G_TYPE_CHECK_INSTANCE_CAST ((o), FCITX_TYPE_CONNECTION, FcitxConnection))
#define FCITX_IS_CONNECTION(object) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((object), FCITX_TYPE_CONNECTION))
#define FCITX_CONNECTION_CLASS(k) \
        (G_TYPE_CHECK_CLASS_CAST((k), FCITX_TYPE_CONNECTION, FcitxConnectionClass))
#define FCITX_CONNECTION_GET_CLASS(o) \
        (G_TYPE_INSTANCE_GET_CLASS ((o), FCITX_TYPE_CONNECTION, FcitxConnectionClass))

G_BEGIN_DECLS

typedef struct _FcitxConnection        FcitxConnection;
typedef struct _FcitxConnectionClass   FcitxConnectionClass;
typedef struct _FcitxConnectionPrivate FcitxConnectionPrivate;

struct _FcitxConnection {
    GObject parent_instance;
    /* instance member */
    FcitxConnectionPrivate* priv;
};

struct _FcitxConnectionClass {
    GObjectClass parent_class;
    /* signals */

    /*< private >*/
    /* padding */
};

GType        fcitx_connection_get_type(void) G_GNUC_CONST;
FcitxConnection* fcitx_connection_new();
gboolean fcitx_connection_is_valid(FcitxConnection* connection);
GDBusConnection* fcitx_connection_get_g_dbus_connection(FcitxConnection* connection);

G_END_DECLS

#endif //CONNECTION_IM_H

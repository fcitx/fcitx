/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include "fcitx/fcitx.h"
#include "fcitx/ime.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

#include "client.h"
#include "marshall.h"

#define LOG_LEVEL DEBUG
#define IC_NAME_MAX 64

struct FcitxIMClient {
    DBusGConnection* conn;
    DBusGProxy* proxy;
    DBusGProxy* icproxy;
    char icname[IC_NAME_MAX];
    int id;
    DBusGProxy* dbusproxy;
    FcitxIMClientConnectCallback connectcb;
    FcitxIMClientDestroyCallback destroycb;
    void *data;
};

static void FcitxIMClientCreateIC(FcitxIMClient* client);

static void _destroy_cb(DBusGProxy *proxy, gpointer user_data);
static void _changed_cb(DBusGProxy* proxy, char* service, char* old_owner, char* new_owner, gpointer user_data);

boolean IsFcitxIMClientValid(FcitxIMClient* client)
{
    if (client == NULL)
        return false;
    if (client->proxy == NULL || client->icproxy == NULL)
        return false;
    
    return true;
}

FcitxIMClient* FcitxIMClientOpen(FcitxIMClientConnectCallback connectcb, FcitxIMClientDestroyCallback destroycb, GObject* data)
{
    FcitxIMClient* client = fcitx_malloc0(sizeof(FcitxIMClient));
    GError *error = NULL;
    client->connectcb = connectcb;
    client->destroycb = destroycb;
    client->data = data;
    client->conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    client->id = -1;
    
    /* You must have dbus to make it works */
    if (client->conn == NULL)
    {
        g_warning(error->message);
        free(client);
        return NULL;
    }
    
    client->dbusproxy = dbus_g_proxy_new_for_name(client->conn,
                                           DBUS_SERVICE_DBUS,
                                           DBUS_PATH_DBUS,
                                           DBUS_INTERFACE_DBUS);
    
    if (!client->dbusproxy)
    {
        g_object_unref(client->conn);
        free(client);
        return NULL;
    }
    dbus_g_object_register_marshaller(fcitx_marshall_VOID__STRING_STRING_STRING, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(client->dbusproxy, "NameOwnerChanged", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(client->dbusproxy, "NameOwnerChanged",
                                G_CALLBACK(_changed_cb), client, NULL);
    
    FcitxIMClientCreateIC(client);
    return client;
}

static void _changed_cb(DBusGProxy* proxy, char* service, char* old_owner, char* new_owner, gpointer user_data)
{
    FcitxLog(LOG_LEVEL, "_changed_cb");
    FcitxIMClient* client = (FcitxIMClient*) user_data;
    if (g_str_equal(service, "org.fcitx.fcitx"))
    {
        gboolean new_owner_good = new_owner && (new_owner[0] != '\0');
        if (new_owner_good)
        {
            if (client->proxy) 
            {
                g_object_unref(client->proxy);
                client->proxy = NULL;
            }

            if (client->icproxy) 
            {
                g_object_unref(client->icproxy);
                client->icproxy = NULL;
            }
            
            FcitxIMClientCreateIC(client);
        }
    }
}

static void _destroy_cb(DBusGProxy *proxy, gpointer user_data)
{
    FcitxLog(LOG_LEVEL, "_destroy_cb");
    FcitxIMClient* client = (FcitxIMClient*) user_data;
    if (client->proxy == proxy)
    {
        g_object_unref(client->proxy);
        g_object_unref(client->icproxy);
        client->proxy = NULL;
        client->icproxy = NULL;
    }
    client->destroycb(client, client->data);
}

void FcitxIMClientCreateIC(FcitxIMClient* client)
{
    GError* error = NULL;
    int id = -1;
    
    client->proxy = dbus_g_proxy_new_for_name_owner(client->conn,
                                                "org.fcitx.fcitx",
                                                "/inputmethod",
                                                "org.fcitx.fcitx",
                                                &error);
    if (!client->proxy)
        return;
    
    g_signal_connect(client->proxy, "destroy", G_CALLBACK( _destroy_cb), client);

    
    dbus_g_proxy_call(client->proxy, "CreateIC", &error, G_TYPE_INVALID, G_TYPE_INT, &id, G_TYPE_INVALID);
    if (id >= 0)
        client->id = id;
    else
        return;
    
    sprintf(client->icname, "/inputcontext_%d", client->id);    
    
    client->icproxy = dbus_g_proxy_new_for_name_owner(client->conn,
                                                      "org.fcitx.fcitx",
                                                      client->icname,
                                                      "org.fcitx.fcitx",
                                                      &error
                                                     );
    if (!client->icproxy)
        return;
    
    dbus_g_proxy_add_signal(client->icproxy, "EnableIM", G_TYPE_INVALID);
    dbus_g_proxy_add_signal(client->icproxy, "CloseIM", G_TYPE_INVALID);
    dbus_g_proxy_add_signal(client->icproxy, "CommitString", G_TYPE_STRING, G_TYPE_INVALID);

    dbus_g_object_register_marshaller(fcitx_marshall_VOID__UINT_UINT_INT, G_TYPE_NONE, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_INVALID);

    dbus_g_proxy_add_signal(client->icproxy, "ForwardKey", G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_INVALID);
    client->connectcb(client, client->data);
}

void FcitxIMClientClose(FcitxIMClient* client)
{
    if (client->icproxy)
    {
        dbus_g_proxy_call_no_reply(client->icproxy, "DestroyIC", G_TYPE_INVALID);
    }
    DBusGProxy* icproxy = client->icproxy;
    DBusGProxy* proxy = client->proxy;
    client->icproxy = NULL;
    client->proxy = NULL;
    if (client->dbusproxy)
        g_object_unref(client->dbusproxy);
    if (proxy)
        g_signal_handlers_disconnect_by_func(proxy, G_CALLBACK( _destroy_cb), client);
    if (icproxy)
        g_object_unref(icproxy);
    if (proxy)
        g_object_unref(proxy);
    free(client);
}

void FcitxIMClientFocusIn(FcitxIMClient* client)
{
    if (client->icproxy)
    {
        dbus_g_proxy_call_no_reply(client->icproxy, "FocusIn", G_TYPE_INVALID);
    }
}

void FcitxIMClientFocusOut(FcitxIMClient* client)
{
    if (client->icproxy)
    {
        dbus_g_proxy_call_no_reply(client->icproxy, "FocusIn", G_TYPE_INVALID);
    }
}

void FcitxIMClientReset(FcitxIMClient* client)
{
    if (client->icproxy)
    {
        dbus_g_proxy_call_no_reply(client->icproxy, "Reset", G_TYPE_INVALID);
    }
}

void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y)
{
    if (client->icproxy)
    {
        dbus_g_proxy_call_no_reply(client->icproxy, "SetCursorLocation", G_TYPE_INT, x, G_TYPE_INT, y, G_TYPE_INVALID);
    }
}

int FcitxIMClientProcessKey(FcitxIMClient* client, uint32_t keyval, uint32_t keycode, uint32_t state, FcitxKeyEventType type, uint32_t t)
{
    int ret;
    int itype = type;
    GError* error = NULL;
    if (!dbus_g_proxy_call(client->icproxy, "ProcessKeyEvent",
                      &error,
                      G_TYPE_UINT, keyval,
                      G_TYPE_UINT, keycode,
                      G_TYPE_UINT, state,
                      G_TYPE_INT, itype,
                      G_TYPE_UINT, t,
                      G_TYPE_INVALID,
                      G_TYPE_INT, &ret,
                      G_TYPE_INVALID
                     ))
    {
        return -1;
    }
    
    
    return ret;
}

void FcitxIMClientConnectSignal(FcitxIMClient* imclient,
    GCallback enableIM,
    GCallback closeIM,
    GCallback commitString,
    GCallback forwardKey,
    void* user_data,
    GClosureNotify freefunc
)
{
    dbus_g_proxy_connect_signal(imclient->icproxy,
        "EnableIM",
        enableIM,
        user_data,
        freefunc
    );
        
    dbus_g_proxy_connect_signal(imclient->icproxy,
        "CloseIM",
        closeIM,
        user_data,
        freefunc
    );
    
    dbus_g_proxy_connect_signal(imclient->icproxy,
        "CommitString",
        commitString,
        user_data,
        freefunc
    );
    
    dbus_g_proxy_connect_signal(imclient->icproxy,
        "ForwardKey",
        forwardKey,
        user_data,
        freefunc
    );
}
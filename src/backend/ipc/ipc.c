#include <dbus/dbus.h>

#include "fcitx/fcitx.h"
#include "fcitx/backend.h"
#include "fcitx-utils/utils.h"
#include "module/dbus/dbusstuff.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"

static void* IPCCreate(FcitxInstance* instance, int backendid);
static boolean IPCDestroy(void* arg);
void IPCCreateIC (void* arg, FcitxInputContext* context, void *priv);
boolean IPCCheckIC (void* arg, FcitxInputContext* context, void* priv);
void IPCDestroyIC (void* arg, FcitxInputContext* context);
static void IPCEnableIM(void* arg, FcitxInputContext* ic);
static void IPCCloseIM(void* arg, FcitxInputContext* ic);
static void IPCCommitString(void* arg, FcitxInputContext* ic, char* str);
static void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y);
static void IPCGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y);
static boolean IPCDBusEventHandler(void* arg, DBusMessage* msg);

const char * introspection_xml =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node name=\"" FCITX_DBUS_PATH "\">\n"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"" FCITX_DBUS_SERVICE "\">\n"
"    <method name=\"CreateIC\">\n"
"      <arg name=\"icid\" direction=\"out\" type=\"i\"/>\n"
"    </method>\n"
"  </interface>\n"
"</node>\n";

typedef struct FcitxIPCBackend {
    int backendid;
    int maxid;
    DBusConnection* conn;
    FcitxInstance* owner;
} FcitxIPCBackend;

FCITX_EXPORT_API
FcitxBackend backend =
{
    IPCCreate,
    IPCDestroy,
    IPCCreateIC,
    IPCCheckIC,
    IPCDestroyIC,
    IPCEnableIM,
    IPCCloseIM,
    IPCCommitString,
    IPCForwardKey,
    IPCSetWindowOffset,
    IPCGetWindowPosition
};

void* IPCCreate(FcitxInstance* instance, int backendid)
{
    FcitxIPCBackend* ipc = fcitx_malloc0(sizeof(FcitxIPCBackend));
    ipc->backendid = backendid;
    ipc->owner = instance;    
    
    FcitxModuleFunctionArg arg;
       
    ipc->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
    
    arg.args[0] = IPCDBusEventHandler;
    arg.args[1] = ipc;
    InvokeFunction(instance, FCITX_DBUS, ADDEVENTHANDLER, arg);
    
    return ipc;
}

boolean IPCDestroy(void* arg)
{
    return true;
}

void IPCCreateIC(void* arg, FcitxInputContext* context, void* priv)
{

}

boolean IPCCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    return false;
}

void IPCDestroyIC(void* arg, FcitxInputContext* context)
{

}

void IPCEnableIM(void* arg, FcitxInputContext* ic)
{

}

void IPCCloseIM(void* arg, FcitxInputContext* ic)
{

}

void IPCCommitString(void* arg, FcitxInputContext* ic, char* str)
{

}

void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{

}

void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y)
{

}

void IPCGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y)
{

}

boolean IPCDBusEventHandler(void* arg, DBusMessage* msg)
{
    FcitxIPCBackend* ipc = (FcitxIPCBackend*) arg;
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
        dbus_connection_send (ipc->conn, reply, NULL);
        dbus_message_unref (reply);
        return true;
    }
    else if (dbus_message_is_method_call(msg, FCITX_DBUS_SERVICE, "CreateIC"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        
        int32_t newid = ipc->maxid;
        CreateIC(ipc->owner, ipc->backendid, &newid);
        ipc->maxid ++;
        dbus_message_append_args(reply, DBUS_TYPE_INT32, &newid, DBUS_TYPE_INVALID);
        dbus_connection_send (ipc->conn, reply, NULL);
        dbus_message_unref (reply);
    }
    return false;
}

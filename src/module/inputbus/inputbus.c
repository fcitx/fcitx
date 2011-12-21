#include <dbus/dbus.h>
#include <libgen.h>
#include <libintl.h>

#include "fcitx/addon.h"
#include "fcitx/ime.h"
#include "fcitx/candidate.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "module/dbus/dbusstuff.h"
#include "fcitx-config/hotkey.h"
#include "fcitx-utils/log.h"

#include "inputbus.h"

const char* inputbus_im_introspection_xml = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"" FCITX_DBUSIM_INTERFACE "\">\n"
    "    <method name=\"NewInputMethod\">\n"
    "      <arg name=\"unique_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"icon_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"priority\" type=\"u\" direction=\"in\" />\n"
    "      <arg name=\"lang_code\" type=\"s\" direction=\"in\" /> \n"
    "    </method> \n"
    "    <method name=\"UpdateStatus\"> \n"
    "      <arg name=\"status\" type=\"a{sv}\" direction=\"in\"/> \n"
    "    </method> \n"
    "    <method name=\"SetKeyMasks\">\n"
    "      <arg name=\"key_mask\" type=\"a(uuu)\" direction=\"in\" />\n"
    "    </method>\n"
    "    <method name=\"KeyProcessHint\">\n"
    "      <arg name=\"hints\" type=\"u\" direction=\"in\" />\n"
    "      <arg name=\"serial\" type=\"u\" />\n"
    "    </method>\n"
    "    <method name=\"CommitString\">\n"
    "      <arg name=\"string\" type=\"s\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <signal name=\"Input\">\n"
    "      <arg name=\"key\" type=\"u\" />\n"
    "      <arg name=\"state\" type=\"u\" />\n"
    "      <arg name=\"serial\" type=\"u\" />\n"
    "      <arg name=\"hintinfo\" type=\"u\" />\n"
    "    </signal>\n"
    "    <signal name=\"Reset\" />\n"
    "    <signal name=\"Save\" />\n"
    "    <signal name=\"Init\" />\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "</node>";

const char* inputbus_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"" FCITX_INPUTBUS_INTERFACE "\">\n"
    "    <method name=\"NewInputMethod\">\n"
    "      <arg name=\"unique_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"icon_name\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"priority\" type=\"u\" direction=\"in\" />\n"
    "      <arg name=\"lang_code\" type=\"s\" direction=\"in\" /> \n"
    "    </method> \n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "     <method name=\"Introspect\">\n"
    "        <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "     </method>\n"
    "  </interface>\n"
    "</node>";

typedef struct _FcitxInputBus {
    DBusConnection *conn;
    FcitxInstance *owner;
    UT_array inputMethodTable;
} FcitxInputBus;

typedef struct _FcitxInputBusKey {
    FcitxKeySym sym;
    unsigned int state;
    int ret;
} FcitxInputBusKey;

typedef struct _FcitxDBusInputMethod {
    char* name;
    char* nameOwner;
    char* objectPath;
    UT_array keyMasks;
    FcitxInputBus *owner;
} FcitxDBusInputMethod;

static void *FcitxInputBusCreate(FcitxInstance *);
static DBusHandlerResult FcitxInputBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *arg);
static DBusHandlerResult FcitxInputBusIMEventHandler(DBusConnection *connection, DBusMessage *msg, void *arg);
static DBusHandlerResult FcitxInputBusNewMethod(DBusConnection *, DBusMessage *, void *);
static FcitxDBusInputMethod* FcitxInputBusFindDBusInputMethodByName(
    FcitxInputBus* inputbus,
    const char* name
);
static void* FcitxInputBusRegisterIM(void* arg, FcitxModuleFunctionArg args);

static const UT_icd dbusim_icd = { sizeof(FcitxDBusInputMethod), NULL, NULL, NULL };
static const UT_icd inputBusKey_icd = { sizeof(FcitxInputBusKey), NULL, NULL, NULL };

FCITX_EXPORT_API
FcitxModule module = {
    FcitxInputBusCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void *FcitxInputBusCreate(FcitxInstance *instance)
{
    FcitxInputBus    *inputbus = (FcitxInputBus *)fcitx_utils_malloc0(sizeof(FcitxInputBus));
    FcitxAddon* inputbusaddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_INPUTBUS_NAME);
    if (!inputbus)
        return NULL;

    FcitxModuleFunctionArg arg;

    // So the dbus module has been successfully initialized, right? Thus no error handling...
    inputbus->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
    if (inputbus->conn == NULL) {
        free(inputbus);
        return NULL;
    }

    inputbus->owner = instance;
    utarray_init(&inputbus->inputMethodTable, &dbusim_icd);

    DBusObjectPathVTable vtable = {NULL, &FcitxInputBusEventHandler, NULL, NULL, NULL, NULL };
    dbus_connection_register_object_path(inputbus->conn, FCITX_INPUTBUS_PATH, &vtable, inputbus);
    AddFunction(inputbusaddon, FcitxInputBusRegisterIM);

    return inputbus;
}

boolean FcitxInputBusMethodInit(void *user_data)
{
    FcitxDBusInputMethod *input_method = user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Init");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
    return true;
}

void FcitxInputBusMethodReset(void *user_data)
{
    FcitxDBusInputMethod *input_method = user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Reset");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
}

INPUT_RETURN_VALUE FcitxInputBusMethodDoInput(void *user_data, FcitxKeySym sym, unsigned int state)
{
    FcitxDBusInputMethod *input_method = user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Input");
    dbus_message_append_args(signal, DBUS_TYPE_UINT32, &sym, DBUS_TYPE_UINT32, &state, DBUS_TYPE_INVALID);
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
    FcitxInputBusKey *keym = (FcitxInputBusKey *)utarray_front(&input_method->keyMasks);
    while (keym) {
        if (keym->sym == sym && keym->state == state)
            return keym->ret;
        keym = (FcitxInputBusKey *)utarray_next(&input_method->keyMasks, keym);
    }
    return IRV_TO_PROCESS;
}

void FcitxInputBusMethodSave(void *user_data)
{
    FcitxDBusInputMethod *input_method = user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Save");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
}

void FcitxInputBusMethodReload(void *user_data)
{
    FcitxDBusInputMethod *input_method = user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Reload");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
}
DBusHandlerResult FcitxInputBusCommitString(DBusConnection *conn,
        DBusMessage *msg,
        void *user_data)
{
    DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_NOT_SUPPORTED, "Not implemented yet");
    dbus_connection_send(conn, retmsg, NULL);
    return DBUS_HANDLER_RESULT_HANDLED;
}
INPUT_RETURN_VALUE FcitxInputBusMethodCommitCallback(void *user_data, FcitxCandidateWord *cand)
{
    return IRV_TO_PROCESS;
}

void* FcitxInputBusRegisterIM(void* arg, FcitxModuleFunctionArg args)
{
    FcitxInputBus* inputbus = arg;
    FcitxIM* im = args.args[0];
    
    char* cmd = NULL;
    asprintf(&cmd, "%s &", im->owner->library);
    FILE* p = popen(cmd, "r");
    free(cmd);
    if (!p) {
        FcitxLog(ERROR, _("Unable to create process"));
        return NULL;
    }
    
    //Build an instance for this input method, use unique_name to identify it
    FcitxDBusInputMethod d, *dbusim;
    memset(&d, 0, sizeof(FcitxDBusInputMethod));
    utarray_push_back(&inputbus->inputMethodTable, &d);
    dbusim = (FcitxDBusInputMethod*)utarray_back(&inputbus->inputMethodTable);
    dbusim->owner = inputbus;
    dbusim->name = strdup(im->uniqueName);
    dbusim->nameOwner = NULL;
    asprintf(&dbusim->objectPath, "%s/%s", FCITX_INPUTBUS_IM_PATH_PREFIX, im->uniqueName);
    //Register this input method to Fcitx
    FcitxInstanceRegisterIM(inputbus->owner,
                      dbusim,
                      im->uniqueName,
                      im->strName,
                      im->strIconName,
                      FcitxInputBusMethodInit,
                      FcitxInputBusMethodReset,
                      FcitxInputBusMethodDoInput,
                      NULL,
                      NULL,
                      FcitxInputBusMethodSave,
                      FcitxInputBusMethodReload,
                      NULL,
                      im->iPriority,
                      im->langCode);
    //Reply the caller with object path
    DBusObjectPathVTable vtable = {NULL, &FcitxInputBusIMEventHandler, NULL, NULL, NULL, NULL };
    dbus_connection_register_object_path(inputbus->conn, dbusim->objectPath, &vtable, arg);
    
    return NULL;
}

DBusHandlerResult FcitxInputBusNewMethod(DBusConnection *conn,
        DBusMessage *msg,
        void *arg)
{
    const char* unique_name, *name, *icon_name, *lang_code;
    int priority;
    DBusError err;
    dbus_error_init(&err);
    dbus_bool_t ret = dbus_message_get_args(msg, &err,
                                            DBUS_TYPE_STRING, &unique_name,
                                            DBUS_TYPE_STRING, &name,
                                            DBUS_TYPE_STRING, &icon_name,
                                            DBUS_TYPE_INT32, &priority,
                                            DBUS_TYPE_STRING, &lang_code,
                                            DBUS_TYPE_INVALID);
    if (!ret) {
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, ":P");
        dbus_connection_send(conn, retmsg, NULL);
        dbus_message_unref(retmsg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    dbus_error_free(&err);

    FcitxInputBus* inputbus = (FcitxInputBus*) arg;

    //Build an instance for this input method, use unique_name to identify it
    FcitxDBusInputMethod d, *dbusim;
    memset(&d, 0, sizeof(FcitxDBusInputMethod));
    utarray_push_back(&inputbus->inputMethodTable, &d);
    dbusim = (FcitxDBusInputMethod*)utarray_back(&inputbus->inputMethodTable);
    dbusim->owner = inputbus;
    dbusim->name = strdup(unique_name);
    dbusim->nameOwner = strdup(dbus_message_get_sender(msg));
    // /org/fcitx/Fcitx/InputBus/${unique_name}
    return DBUS_HANDLER_RESULT_HANDLED;
}
DBusHandlerResult FcitxInputBusUpdateCandidate(
    DBusConnection *conn,
    DBusMessage *msg,
    void *user_data)
{
    char *obj_path = strdup(dbus_message_get_path(msg));
    char *unique_name = basename(obj_path);
    FcitxInputBus* inputbus = (FcitxInputBus*) user_data;
    FcitxDBusInputMethod* dbusim = FcitxInputBusFindDBusInputMethodByName(inputbus, unique_name);

    DBusMessage *retmsg = NULL;
    if (dbusim == NULL) {
        //Not found, reply error
        retmsg = dbus_message_new_error(msg, DBUS_ERROR_FAILED, ":P");
        dbus_connection_send(conn, retmsg, NULL);
        goto update_cand_end;
    }
    if (strcmp(dbusim->nameOwner, dbus_message_get_sender(msg)) != 0) {
        retmsg = dbus_message_new_error(msg, DBUS_ERROR_FAILED, ":P");
        dbus_connection_send(conn, retmsg, NULL);
        goto update_cand_end;
    }
    FcitxIM* im = FcitxInstanceGetCurrentIM(inputbus->owner);

    if (dbusim != im->klass) {
        //Input method not active, reply error
        retmsg = dbus_message_new_error(msg, DBUS_ERROR_AUTH_FAILED, "Input method not active.");
        dbus_connection_send(conn, retmsg, NULL);
        goto update_cand_end;
    }

    char **cands;
    int ncands;
    DBusError err;
    dbus_error_init(&err);
    dbus_message_get_args(msg, &err,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &cands, &ncands,
                          DBUS_TYPE_INVALID);
    FcitxInstanceCleanInputWindowDown(inputbus->owner);
    dbus_error_free(&err);

    int i;
    FcitxInputState *input = FcitxInstanceGetInputState(inputbus->owner);
    for (i = 0; i < ncands; i++) {
        FcitxCandidateWord cand;
        cand.callback = FcitxInputBusMethodCommitCallback;
        cand.owner = inputbus;
        cand.priv = dbusim;
        cand.strExtra = NULL;
        cand.strWord = strdup(cands[i]);
        FcitxCandidateWordAppend(
            FcitxInputStateGetCandidateList(input),
            &cand);
        if (i == 0)
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", cand.strWord);
    }
update_cand_end:
    free(obj_path);
    if (retmsg) dbus_message_unref(retmsg);
    return DBUS_HANDLER_RESULT_HANDLED;
}
DBusHandlerResult FcitxInputBusSetKeyMasks(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    FcitxInputBus *inputBus = (FcitxInputBus *)user_data;
    DBusMessageIter iter;
    DBusMessage *retmsg;
    if (!dbus_message_iter_init(msg, &iter))
        goto invalid_args;
    char *obj_path = strdup(dbus_message_get_path(msg));
    char *unique_name = basename(obj_path);
    FcitxDBusInputMethod *dbusim = FcitxInputBusFindDBusInputMethodByName(inputBus, unique_name);

    utarray_free(&dbusim->keyMasks);
    utarray_init(&dbusim->keyMasks, &inputBusKey_icd);

    int current_type;
    while ((current_type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
        if (current_type != DBUS_TYPE_STRUCT)
            goto invalid_args;
        DBusMessageIter subiter;
        FcitxInputBusKey *key = (FcitxInputBusKey *)fcitx_utils_malloc0(sizeof(FcitxInputBusKey));
        dbus_message_iter_recurse(&iter, &subiter);
        if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->sym);
        dbus_message_iter_next(&subiter);
        if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->state);
        dbus_message_iter_next(&subiter);
        if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->ret);
        utarray_push_back(&dbusim->keyMasks, key);
        dbus_message_iter_next(&iter);
    }
    free(obj_path);
    return DBUS_HANDLER_RESULT_HANDLED;

invalid_args:
    retmsg = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, ":P");
    dbus_connection_send(conn, retmsg, NULL);
    dbus_message_unref(retmsg);
    free(obj_path);
    utarray_free(&dbusim->keyMasks);
    return DBUS_HANDLER_RESULT_HANDLED;

}

inline static DBusHandlerResult FcitxInputBusIntrospect(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    FcitxInputBus* inputbus = (FcitxInputBus*) user_data;
    DBusMessage *reply = dbus_message_new_method_return(msg);

    dbus_message_append_args(reply, DBUS_TYPE_STRING, &inputbus_introspection_xml, DBUS_TYPE_INVALID);
    dbus_connection_send(inputbus->conn, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

inline static DBusHandlerResult FcitxInputBusIMIntrospect(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    DBusMessage *retmsg = dbus_message_new_method_return(msg);
    dbus_message_append_args(retmsg, DBUS_TYPE_STRING, &inputbus_im_introspection_xml, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, retmsg, NULL);
    dbus_message_unref(retmsg);
    return DBUS_HANDLER_RESULT_HANDLED;
}

FcitxDBusInputMethod* FcitxInputBusFindDBusInputMethodByName(
    FcitxInputBus* inputbus,
    const char* name
)
{
    UT_array* inputMethodTable = &inputbus->inputMethodTable;
    FcitxDBusInputMethod *dbusim = NULL;
    for (dbusim = (FcitxDBusInputMethod *) utarray_front(inputMethodTable);
            dbusim != NULL;
            dbusim = (FcitxDBusInputMethod *) utarray_next(inputMethodTable, dbusim)) {
        if (strcmp(name, dbusim->name) == 0) {
            break;
        }
    }
    return dbusim;
}

DBusHandlerResult FcitxInputBusIMEventHandler(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    DBusHandlerResult ret;
    //Interface needed: NewInputMethod, UpdateCandidate, UpdateUI, Commit
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "UpdateCandidate"))
        ret = FcitxInputBusUpdateCandidate(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "NewInputMethod"))
        ret = FcitxInputBusNewMethod(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "CommitString"))
        ret = FcitxInputBusCommitString(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "SetKeyMasks"))
        ret = FcitxInputBusSetKeyMasks(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
        ret = FcitxInputBusIMIntrospect(conn, msg, user_data);
    else {
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, "The method you call doesn't exists");
        dbus_connection_send(conn, retmsg, NULL);
        dbus_message_unref(retmsg);
        ret = DBUS_HANDLER_RESULT_HANDLED;
    }
    dbus_connection_flush(conn);
    return ret;
}

DBusHandlerResult FcitxInputBusEventHandler(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    DBusHandlerResult ret;
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "NewInputMethod"))
        ret = FcitxInputBusNewMethod(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
        ret = FcitxInputBusIntrospect(conn, msg, user_data);
    else {
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, "The method you call doesn't exists");
        dbus_connection_send(conn, retmsg, NULL);
        dbus_message_unref(retmsg);
        ret = DBUS_HANDLER_RESULT_HANDLED;
    }
    dbus_connection_flush(conn);
    return ret;
}
// vim:set sw=4 et:

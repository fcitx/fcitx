#include <dbus/dbus.h>
#include <fcitx/addon.h>
#include <fcitx/ime.h>
#include <fcitx/candidate.h>
#include <fcitx/instance.h>
#include <fcitx/module.h>
#include <fcitx/module/dbus/dbusstuff.h>
#include <fcitx-config/hotkey.h>
#include <fcitx-utils/log.h>
#include <libgen.h>

#include "inputbus.h"

/*
 * This module should be intergrate with fcitx
 *
 * Fcitx LoadModule first load this module, initialize something. 
 * 
 * After that LoadAllIM look at category, if it is new "DBus",
 * Then Invoke Function will call this FcitxInputBusNewMethod,
 * in order to register.
 * 
 * The reason to do this:
 * 1. Let fcitx configuration tool can see the dbus input method.
 * 2. start input method from fcitx side, so no input method would
 * be start if it was not used.
 * 3. fcitx can explicit send shutdown to unused input method, with
 * new update im list hook (check if they were in the list, if not, send
 * shutdown signal to them).
 * 
 * Not completed, need a example/template to make it run.
 */


typedef struct _FcitxDBusInputMethod {
    const char *name;
    FcitxInstance *owner;
} FcitxDBusInputMethod;

typedef struct _FcitxInputBus {
    DBusConnection *conn;
    FcitxInstance *owner;
    UT_array inputMethodTable;
} FcitxInputBus;

static void *FcitxInputBusCreate(FcitxInstance *);
static DBusHandlerResult FcitxInputBusDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *arg);
static void* FcitxInputBusNewMethod(void* arg, FcitxModuleFunctionArg args);
static FcitxDBusInputMethod* FcitxInputBusFindDBusInputMethodByName(
    FcitxInputBus* inputbus,
    const char* name
);

static const UT_icd dbusim_icd = { sizeof(FcitxDBusInputMethod), NULL, NULL, NULL };

FCITX_EXPORT_API
FcitxModule module = {
    FcitxInputBusCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

void *FcitxInputBusCreate(FcitxInstance *instance) {
    FcitxInputBus    *inputbus = (FcitxInputBus *)fcitx_malloc0(sizeof(FcitxInputBus));
    FcitxAddon* inputbusaddon = GetAddonByName(FcitxInstanceGetAddons(instance), FCITX_INPUTBUS_NAME);
    if (!inputbus)
        return NULL;

    FcitxModuleFunctionArg arg;
    
    //So the dbus module has been successfully initialized, right? Thus no error handling...
    inputbus->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
    if (inputbus->conn == NULL)
    {
        free(inputbus);
        return NULL;
    }
    
    inputbus->owner = instance;
    utarray_init(&inputbus->inputMethodTable, &dbusim_icd);
    
    DBusObjectPathVTable vtable = {NULL, &FcitxInputBusDBusEventHandler, NULL, NULL, NULL, NULL };

    dbus_connection_register_object_path(inputbus->conn, FCITX_INPUTBUS_PATH, &vtable, inputbus);
    
    AddFunction(inputbusaddon, FcitxInputBusNewMethod);
    return inputbus;
}

boolean FcitxInputBusMethodInit(void *user_data) {
    return true;
}

void FcitxInputBusMethodReset(void *user_data) {

}

INPUT_RETURN_VALUE FcitxInputBusMethodDoInput(void *user_data, FcitxKeySym sym, unsigned int state) {
    return IRV_TO_PROCESS;
}

void FcitxInputBusMethodSave(void *user_data) {
}

void FcitxInputBusMethodReload(void *user_data) {
}

void* FcitxInputBusNewMethod(void* arg, FcitxModuleFunctionArg args) {
    const char* unique_name = args.args[0];
    const char* name = args.args[1];
    const char* icon_name = args.args[2];
    const char* lang_code = args.args[3];
    int priority = *(int*)args.args[4];
    FcitxInputBus* inputbus = (FcitxInputBus*) arg;

    //Build an instance for this input method, use unique_name to identify it
    FcitxDBusInputMethod d, *dbusim;
    memset(&d, 0, sizeof(FcitxDBusInputMethod));
    utarray_push_back(&inputbus->inputMethodTable, &d);
    dbusim = (FcitxDBusInputMethod*)utarray_back(&inputbus->inputMethodTable);
    dbusim->owner = inputbus->owner;
    dbusim->name = strdup(unique_name);
    //Register this input method to Fcitx
    FcitxRegisterIMv2(inputbus->owner,
                      inputbus,
                      unique_name,
                      name,
                      icon_name,
                      FcitxInputBusMethodInit,
                      FcitxInputBusMethodReset,
                      FcitxInputBusMethodDoInput,
                      NULL,
                      NULL,
                      FcitxInputBusMethodSave,
                      FcitxInputBusMethodReload,
                      dbusim,
                      priority,
                      lang_code);
    return NULL;
}
INPUT_RETURN_VALUE FcitxInputBusMethodCommitCallback(void *user_data, CandidateWord *cand) {
    return IRV_TO_PROCESS;
}
DBusHandlerResult FcitxInputBusUpdateCandidate(
    DBusConnection *conn,
    DBusMessage *msg,
    void *user_data) {
    char *obj_path = strdup(dbus_message_get_path(msg));
    char *unique_name = basename(obj_path);
    FcitxInputBus* inputbus = (FcitxInputBus*) user_data;
    FcitxDBusInputMethod* dbusim = FcitxInputBusFindDBusInputMethodByName(inputbus, unique_name);

    if (dbusim == NULL) {
        //Not found, reply error
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_OBJECT, ":P");
        dbus_connection_send(conn, msg, NULL);
        dbus_message_unref(retmsg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    FcitxIM* im = GetCurrentIM(inputbus->owner);
    
    if (dbusim != im->priv) {
        //Input method not active, reply error
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_AUTH_FAILED, "Input method not active.");
        dbus_connection_send(conn, msg, NULL);
        dbus_message_unref(retmsg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    char **cands;
    int ncands;
    DBusError err;
    dbus_error_init(&err);
    dbus_message_get_args(msg, &err,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &cands, &ncands,
                          DBUS_TYPE_INVALID);
    CleanInputWindowDown(inputbus->owner);
    dbus_error_free(&err);

    int i;
    FcitxInputState *input = FcitxInstanceGetInputState(inputbus->owner);
    for (i=0; i<ncands; i++) {
        CandidateWord cand;
        cand.callback = FcitxInputBusMethodCommitCallback;
        cand.owner = inputbus;
        cand.priv = dbusim;
        cand.strExtra = NULL;
        cand.strWord = strdup(cands[i]);
        CandidateWordAppend(
            FcitxInputStateGetCandidateList(input),
            &cand);
        if (i == 0)
            AddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", cand.strWord);
    }
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

DBusHandlerResult FcitxInputBusDBusEventHandler(DBusConnection *conn, DBusMessage *msg, void *user_data) {
    DBusHandlerResult ret;
    //Interface needed: NewInputMethod, UpdateCandidate, UpdateUI, Commit
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "UpdateCandidate"))
        ret = FcitxInputBusUpdateCandidate(conn, msg, user_data);
    else {
        DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, "The method you call doesn't exists");
        dbus_connection_send(conn, retmsg, NULL);
        dbus_message_unref(retmsg);
        ret = DBUS_HANDLER_RESULT_HANDLED;
    }
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    return ret;
}


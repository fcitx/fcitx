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
typedef struct _FcitxInputBus {
    DBusConnection *conn;
    FcitxInstance *owner;
    UT_array inputMethodTable;
} FcitxInputBus;

typedef struct _FcitxInputBusKey{
    FcitxKeySym sym;
    unsigned int state;
    int ret;
} FcitxInputBusKey;

typedef struct _FcitxDBusInputMethod {
    const char *name, *nameOwner, *objectPath; //Name owner for dbus name owner, tracked via NameOwnerChange signal, as input method identification
    UT_array keyMasks;
    FcitxInputBus *owner;
} FcitxDBusInputMethod;

static void *FcitxInputBusCreate(FcitxInstance *);
static DBusHandlerResult FcitxInputBusDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *arg);
static DBusHandlerResult FcitxInputBusNewMethod(DBusConnection *, DBusMessage *, void *);
static FcitxDBusInputMethod* FcitxInputBusFindDBusInputMethodByName(
    FcitxInputBus* inputbus,
    const char* name
);

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
    FcitxDBusInputMethod *input_method = (FcitxDBusInputMethod *)user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Input");
    dbus_message_append_args(signal, DBUS_TYPE_UINT32, &sym, DBUS_TYPE_UINT32, &state, DBUS_TYPE_INVALID);
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
    return IRV_TO_PROCESS;
}

void FcitxInputBusMethodSave(void *user_data) {
    FcitxDBusInputMethod *input_method = (FcitxDBusInputMethod *)user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Save");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
}

void FcitxInputBusMethodReload(void *user_data) {
    FcitxDBusInputMethod *input_method = (FcitxDBusInputMethod *)user_data;
    DBusMessage *signal = dbus_message_new_signal(input_method->objectPath, "org.fcitx.Fcitx.InputBus", "Reload");
    dbus_connection_send(input_method->owner->conn, signal, NULL);
    dbus_connection_flush(input_method->owner->conn);
    dbus_message_unref(signal);
}
DBusHandlerResult FcitxInputBusCommitString(DBusConnection *conn,
        DBusMessage *msg,
        void *user_data) {
    DBusMessage *retmsg = dbus_message_new_error(msg, DBUS_ERROR_NOT_SUPPORTED, "Not implemented yet");
    dbus_connection_send(conn, retmsg, NULL);
    return DBUS_HANDLER_RESULT_HANDLED;
}
INPUT_RETURN_VALUE FcitxInputBusMethodCommitCallback(void *user_data, CandidateWord *cand) {
    return IRV_TO_PROCESS;
}

DBusHandlerResult FcitxInputBusNewMethod(DBusConnection *conn,
        DBusMessage *msg,
        void *arg) {
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
    if(!ret){
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
    char *obj_path = (char *)fcitx_malloc0(sizeof(char)*(strlen(FCITX_INPUTBUS_OBJECTPATH_PREFIX)+strlen(unique_name)+1));
    sprintf(obj_path, "%s%s",FCITX_INPUTBUS_OBJECTPATH_PREFIX, unique_name);
    dbusim->objectPath =obj_path;

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
    //Reply the caller with object path
    DBusMessage *retmsg = dbus_message_new_method_return(msg);    dbus_message_append_args(msg,
            DBUS_TYPE_OBJECT_PATH, &obj_path,
            DBUS_TYPE_INVALID);
    dbus_connection_send(conn, retmsg, NULL);
    dbus_message_unref(retmsg);
    return DBUS_HANDLER_RESULT_HANDLED;
}
DBusHandlerResult FcitxInputBusUpdateCandidate(
        DBusConnection *conn,
        DBusMessage *msg,
        void *user_data) {
    char *obj_path = strdup(dbus_message_get_path(msg));
    char *unique_name = basename(obj_path);
    FcitxInputBus* inputbus = (FcitxInputBus*) user_data;
    FcitxDBusInputMethod* dbusim = FcitxInputBusFindDBusInputMethodByName(inputbus, unique_name);
    
    DBusMessage *retmsg = NULL;
    if (dbusim == NULL) {
        //Not found, reply error
        retmsg = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_OBJECT, ":P");
        dbus_connection_send(conn, retmsg, NULL);
        goto update_cand_end;
    }
    if (strcmp(dbusim->nameOwner, dbus_message_get_sender(msg)) != 0){
        retmsg = dbus_message_new_error(msg, DBUS_ERROR_BAD_ADDRESS, ":P");
        dbus_connection_send(conn, retmsg, NULL);
        goto update_cand_end;
    }
    FcitxIM* im = GetCurrentIM(inputbus->owner);

    if (dbusim != im->priv) {
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
update_cand_end:
    free(obj_path);
    if (retmsg) dbus_message_unref(retmsg);
    return DBUS_HANDLER_RESULT_HANDLED;
}
DBusHandlerResult FcitxInputBusSetKeyMasks(DBusConnection *conn, DBusMessage *msg, void *user_data){
    FcitxInputBus *inputBus = (FcitxInputBus *)user_data;
    DBusMessageIter iter;
    DBusMessage *retmsg;
    if(!dbus_message_iter_init(msg, &iter))
        goto invalid_args;
    char *obj_path = strdup(dbus_message_get_path(msg));
    char *unique_name = basename(obj_path);
    FcitxDBusInputMethod *dbusim = FcitxInputBusFindDBusInputMethodByName(inputBus, unique_name);
    
    utarray_free(&dbusim->keyMasks);
    utarray_init(&dbusim->keyMasks, &inputBusKey_icd);

    int current_type;
    while ((current_type = dbus_message_iter_get_arg_type (&iter)) != DBUS_TYPE_INVALID){
        if(current_type !=DBUS_TYPE_STRUCT)
            goto invalid_args;
        DBusMessageIter subiter;
        FcitxInputBusKey *key = (FcitxInputBusKey *)fcitx_malloc0(sizeof(FcitxInputBusKey));
        dbus_message_iter_recurse(&iter, &subiter);
        if(dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->sym);
        dbus_message_iter_next(&subiter);
        if(dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->state);
        dbus_message_iter_next(&subiter);
        if(dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_UINT32)
            goto invalid_args;
        dbus_message_iter_get_basic(&subiter, &key->ret);
        utarray_push_back(&dbusim->keyMasks, key);
        dbus_message_iter_next (&iter);
    }
    free(obj_path);
    return DBUS_HANDLER_RESULT_HANDLED;

invalid_args:
    retmsg = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, ":P");
    dbus_connection_send(conn, msg, NULL);
    dbus_message_unref(msg);
    free(obj_path);
    utarray_free(&dbusim->keyMasks);
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
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "NewInputMethod"))
	ret = FcitxInputBusNewMethod(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "CommitString"))
        ret = FcitxInputBusCommitString(conn, msg, user_data);
    if (dbus_message_is_method_call(msg, "org.fcitx.Fcitx.InputBus", "SetKeyMasks"))
        ret = FcitxInputBusSetKeyMasks(conn, msg, user_data);
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
// vim:set sw=4 et:

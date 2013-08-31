#include "module/wayland/fcitx-wayland.h"
#include "client-protocol.h"
#include <fcitx/ui-internal.h>
#include <xkbcommon/xkbcommon.h>
#include <fcitx/hook.h>
#include <sys/mman.h>

#define GetWaylandIC(context) ((FcitxWaylandIC*) (context)->privateic)

typedef struct _FcitxWaylandIMFrontend {
    int frontendid;
    FcitxInstance* owner;
    struct wl_display* display;
    void* input_method;
    struct xkb_keymap* keymap;
    struct xkb_state* state;
    uint32_t shift_mask;
    uint32_t lock_mask;
    uint32_t control_mask;
    uint32_t mod1_mask;
    uint32_t mod2_mask;
    uint32_t mod3_mask;
    uint32_t mod4_mask;
    uint32_t mod5_mask;
    uint32_t super_mask;
    uint32_t hyper_mask;
    uint32_t meta_mask;
    uint32_t modifiers;
    struct xkb_context* xkb_context;
} FcitxWaylandIMFrontend;

typedef struct _FcitxWaylandIC {
    int serial;
    struct wl_input_method_context* context;
    struct wl_keyboard* keyboard;
    FcitxWaylandIMFrontend* im;
    char* surroundingText;
    size_t cursor_pos;
    size_t anchor_pos;
    boolean lastPreeditIsEmpty;
} FcitxWaylandIC;


static void* WaylandIMCreate(FcitxInstance* instance, int frontendid);
static boolean WaylandIMDestroy(void* arg);
void WaylandIMCreateIC(void* arg, FcitxInputContext* context, void *priv);
boolean WaylandIMCheckIC(void* arg, FcitxInputContext* context, void* priv);
void WaylandIMDestroyIC(void* arg, FcitxInputContext* context);
static void WaylandIMEnableIM(void* arg, FcitxInputContext* ic);
static void WaylandIMCloseIM(void* arg, FcitxInputContext* ic);
static void WaylandIMCommitString(void* arg, FcitxInputContext* ic, const char* str);
static void WaylandIMForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void WaylandIMUpdatePreedit(void* arg, FcitxInputContext* ic);
static void WaylandIMDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size);
static boolean WaylandIMGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int* cursor, unsigned int* anchor);

FCITX_DEFINE_PLUGIN(fcitx_waylandim, frontend, FcitxFrontend) = {
    WaylandIMCreate,
    WaylandIMDestroy,
    WaylandIMCreateIC,
    WaylandIMCheckIC,
    WaylandIMDestroyIC,
    WaylandIMEnableIM,
    WaylandIMCloseIM,
    WaylandIMCommitString,
    WaylandIMForwardKey,
    NULL,
    NULL,
    WaylandIMUpdatePreedit,
    NULL,
    NULL,
    NULL,
    NULL,
    WaylandIMDeleteSurroundingText,
    WaylandIMGetSurroundingText
};

static void
input_method_activate (void                           *data,
                       struct wl_input_method         *input_method,
                       struct wl_input_method_context *context)
{
    FcitxWaylandIMFrontend* waylandim = data;

    FcitxInstanceCreateIC(waylandim->owner, waylandim->frontendid, context);
}

static void
input_method_deactivate (void                           *data,
                         struct wl_input_method         *input_method,
                         struct wl_input_method_context *context)
{
    FcitxWaylandIMFrontend* waylandim = data;

    FcitxInstanceDestroyIC(waylandim->owner, waylandim->frontendid, context);
}

static const struct wl_input_method_listener input_method_listener = {
    input_method_activate,
    input_method_deactivate
};

static void
registry_handle_global (void               *data,
                        struct wl_registry *registry,
                        uint32_t            name,
                        const char         *interface,
                        uint32_t            version)
{
    FcitxWaylandIMFrontend* waylandim = data;
    if (!fcitx_utils_strcmp0 (interface, "wl_input_method")) {
        waylandim->input_method =
            wl_registry_bind (registry, name, &wl_input_method_interface, 1);
        wl_input_method_add_listener (waylandim->input_method,
                                      &input_method_listener, waylandim);
    }
}
static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry,
                               uint32_t            name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};


static void
input_method_keyboard_keymap (void               *data,
                              struct wl_keyboard *wl_keyboard,
                              uint32_t            format,
                              int32_t             fd,
                              uint32_t            size)
{
    FcitxInputContext *ic = data;
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);
    FcitxWaylandIMFrontend* waylandim = waylandic->im;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    if (waylandim->keymap) {
        xkb_map_unref(waylandim->keymap);
        waylandim->keymap = NULL;
    }

    char* buffer = malloc(1024);
    int bufferSize = 1024;
    int bufferPtr = 0;
    int readSize;
    while ((readSize = read(fd, &buffer[bufferPtr], bufferSize - bufferPtr)) > 0) {
        bufferPtr += readSize;
        if (bufferPtr == bufferSize) {
            bufferSize *= 2;
            buffer = realloc(buffer, bufferSize);
        }
    }
    buffer[bufferPtr] = 0;

    if (!buffer)
    {
        return;
    }

    FcitxLog(INFO, "%s", buffer);

    waylandim->keymap =
        xkb_keymap_new_from_string (waylandim->xkb_context,
                                 buffer,
                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                 0);

    close(fd);

    if (!waylandim->keymap) {
        return;
    }

    waylandim->state = xkb_state_new (waylandim->keymap);
    if (!waylandim->state) {
        xkb_keymap_unref (waylandim->keymap);
        waylandim->keymap = NULL;
        return;
    }

    waylandim->shift_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Shift");
    waylandim->lock_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Lock");
    waylandim->control_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Control");
    waylandim->mod1_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Mod1");
    waylandim->mod2_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Mod2");
    waylandim->mod3_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Mod3");
    waylandim->mod4_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Mod4");
    waylandim->mod5_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Mod5");
    waylandim->super_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Super");
    waylandim->hyper_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Hyper");
    waylandim->meta_mask =
        1 << xkb_map_mod_get_index (waylandim->keymap, "Meta");
}


static void
input_method_keyboard_key (void               *data,
                           struct wl_keyboard *wl_keyboard,
                           uint32_t            serial,
                           uint32_t            time,
                           uint32_t            key,
                           uint32_t            keystate)
{
    FcitxInputContext *callic = data;
    FcitxWaylandIC* waylandic = GetWaylandIC(callic);
    FcitxWaylandIMFrontend* waylandim = waylandic->im;

    do {
        if (!waylandim->state) {
            break;
        }

        // EVDEV OFFSET
        uint32_t code = key + 8;
        const xkb_keysym_t *syms;
        int num_syms = xkb_key_get_syms (waylandim->state, code, &syms);

        unsigned int originsym = XKB_KEY_NoSymbol;
        if (num_syms == 1) {
            originsym = syms[0];
        }

        uint32_t originstate = waylandim->modifiers;
        FcitxKeyEventType event = FCITX_PRESS_KEY;
        if (keystate == WL_KEYBOARD_KEY_STATE_RELEASED) {
            event = FCITX_RELEASE_KEY;
        }

        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(waylandim->owner);
        if (ic != callic) {
            FcitxInstanceSetCurrentIC(waylandim->owner, callic);
            FcitxUIOnInputFocus(waylandim->owner);
        }
        ic = callic;
        FcitxKeySym sym;
        unsigned int state;

        state = originstate & FcitxKeyState_SimpleMask;
        state &= FcitxKeyState_UsedMask;
        FcitxHotkeyGetKey(originsym, state, &sym, &state);
        FcitxLog(DEBUG,
                "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%u ",
                (event == FCITX_RELEASE_KEY), state, code, sym);

        if (originsym == 0) {
            break;
        }

        FcitxInputState* input = FcitxInstanceGetInputState(waylandim->owner);
        FcitxInputStateSetKeyCode(input, code);
        FcitxInputStateSetKeySym(input, originsym);
        FcitxInputStateSetKeyState(input, originstate);
        INPUT_RETURN_VALUE retVal = FcitxInstanceProcessKey(waylandim->owner, event,
                                            time,
                                            sym, state);
        FcitxInputStateSetKeyCode(input, 0);
        FcitxInputStateSetKeySym(input, 0);
        FcitxInputStateSetKeyState(input, 0);

        if (retVal & IRV_FLAG_FORWARD_KEY || retVal == IRV_TO_PROCESS) {
            break;
        } else {
            return;
        }

    } while(0);

    wl_input_method_context_key (waylandic->context,
                                serial,
                                time,
                                key,
                                keystate);
}


static void
input_method_keyboard_modifiers (void               *data,
                                 struct wl_keyboard *wl_keyboard,
                                 uint32_t            serial,
                                 uint32_t            mods_depressed,
                                 uint32_t            mods_latched,
                                 uint32_t            mods_locked,
                                 uint32_t            group)
{
    FcitxInputContext *ic = data;
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);
    FcitxWaylandIMFrontend* waylandim = waylandic->im;
    struct wl_input_method_context *context = waylandic->context;
    xkb_mod_mask_t mask;

    xkb_state_update_mask (waylandim->state, mods_depressed,
                           mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods (waylandim->state,
                                     XKB_STATE_DEPRESSED |
                                     XKB_STATE_LATCHED);

    waylandim->modifiers = 0;
    if (mask & waylandim->shift_mask)
        waylandim->modifiers |= FcitxKeyState_Shift;
    if (mask & waylandim->lock_mask)
        waylandim->modifiers |= FcitxKeyState_CapsLock;
    if (mask & waylandim->control_mask)
        waylandim->modifiers |= FcitxKeyState_Ctrl;
    if (mask & waylandim->mod1_mask)
        waylandim->modifiers |= FcitxKeyState_Alt;
    if (mask & waylandim->super_mask)
        waylandim->modifiers |= FcitxKeyState_Super;
    if (mask & waylandim->hyper_mask)
        waylandim->modifiers |= FcitxKeyState_Hyper;
    if (mask & waylandim->meta_mask)
        waylandim->modifiers |= FcitxKeyState_Meta;

    wl_input_method_context_modifiers (context, serial,
                                       mods_depressed, mods_depressed,
                                       mods_latched, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    input_method_keyboard_keymap,
    NULL, /* enter */
    NULL, /* leave */
    input_method_keyboard_key,
    input_method_keyboard_modifiers
};


static void
handle_surrounding_text (void                           *data,
                         struct wl_input_method_context *context,
                         const char                     *text,
                         uint32_t                        cursor,
                         uint32_t                        anchor)
{
    FcitxInputContext* ic = (FcitxInputContext*) data;
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);

    size_t cursor_pos = fcitx_utf8_strnlen (text, cursor);
    size_t anchor_pos = fcitx_utf8_strnlen (text, anchor);
    if (!waylandic->surroundingText || strcmp(waylandic->surroundingText, text) != 0 || cursor_pos != waylandic->cursor_pos || anchor_pos != waylandic->anchor_pos)
    {
        fcitx_utils_free(waylandic->surroundingText);
        waylandic->surroundingText = strdup(text);
        waylandic->cursor_pos = cursor_pos;
        waylandic->anchor_pos = anchor_pos;
        FcitxInstanceNotifyUpdateSurroundingText(waylandic->im->owner, ic);
    }
}

static void
handle_reset (void                           *data,
              struct wl_input_method_context *context)
{
    FcitxInputContext* ic = (FcitxInputContext*) data;
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);

    FcitxInstanceResetInput(waylandic->im->owner);
}
static void
handle_content_type (void                           *data,
                     struct wl_input_method_context *context,
                     uint32_t                        hint,
                     uint32_t                        purpose)
{
}

static void
handle_invoke_action (void                           *data,
                      struct wl_input_method_context *context,
                      uint32_t                        button,
                      uint32_t                        index)
{
}

static void
handle_commit_state (void                           *data,
                     struct wl_input_method_context *context,
                     uint32_t                        serial)
{
    FcitxInputContext* ic = (FcitxInputContext*) data;
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);

    waylandic->serial = serial;
}

static void
handle_preferred_language (void                           *data,
                           struct wl_input_method_context *context,
                           const char                     *language)
{
}

static const struct wl_input_method_context_listener context_listener = {
    handle_surrounding_text,
    handle_reset,
    handle_content_type,
    handle_invoke_action,
    handle_commit_state,
    handle_preferred_language
};

void WaylandIMCreateIC(void* arg, FcitxInputContext* ic, void* priv)
{
    FcitxWaylandIC* waylandic = fcitx_utils_new(FcitxWaylandIC);
    ic->privateic = waylandic;
    ic->contextCaps = CAPACITY_PREEDIT | CAPACITY_SURROUNDING_TEXT | CAPACITY_CLIENT_UNFOCUS_COMMIT;
    struct wl_input_method_context* context = priv;
    waylandic->serial = 0;
    waylandic->context = context;
    waylandic->im = (FcitxWaylandIMFrontend*) arg;

    wl_input_method_context_add_listener (context, &context_listener, ic);
    waylandic->keyboard = wl_input_method_context_grab_keyboard (context);
    wl_keyboard_add_listener(waylandic->keyboard,
                             &keyboard_listener,
                             ic);
    wl_input_method_context_set_user_data(context, ic);

    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(waylandic->im->owner);
    if (config->shareState == ShareState_PerProgram) {
        FcitxInstanceSetICStateFromSameApplication(waylandic->im->owner, waylandic->im->frontendid, ic);
    }
}

void WaylandIMDestroyIC(void* arg, FcitxInputContext* context)
{
    FcitxWaylandIC* waylandic = GetWaylandIC(context);
    wl_input_method_context_destroy(waylandic->context);
    free(waylandic);
    context->privateic = NULL;
}

boolean WaylandIMDestroy(void* arg)
{
    return true;
}



void* WaylandIMCreate(FcitxInstance* instance, int frontendid)
{
    FcitxWaylandIMFrontend* waylandim = fcitx_utils_malloc0(sizeof(FcitxWaylandIMFrontend));
    waylandim->frontendid = frontendid;
    waylandim->owner = instance;

    waylandim->display = FcitxWaylandGetDisplay(instance);

    if (!waylandim->display) {
        FcitxLog(WARNING, "Wayland is not running.");
        free(waylandim);
        return NULL;
    }

    waylandim->xkb_context = xkb_context_new(0);
    struct wl_registry* registry = wl_display_get_registry(waylandim->display);
    wl_registry_add_listener(registry, &registry_listener, waylandim);

    return waylandim;
}

boolean WaylandIMCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    FcitxWaylandIC* waylandic = GetWaylandIC(context);
    if (waylandic->context == priv) {
        return true;
    }

    return false;
}

void WaylandIMDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size)
{
    return;
}

boolean WaylandIMGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int* cursor, unsigned int* anchor)
{
    FCITX_UNUSED(arg);
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);

    if (!waylandic->surroundingText)
        return false;

    if (str)
        *str = strdup(waylandic->surroundingText);

    if (cursor)
        *cursor = waylandic->cursor_pos;

    if (anchor)
        *anchor = waylandic->anchor_pos;

    return true;
}

void WaylandIMForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);

    wl_input_method_context_keysym (waylandic->context,
                                    waylandic->serial,
                                    0,
                                    sym,
                                    event == FCITX_PRESS_KEY ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED,
                                    state);
}

void WaylandIMEnableIM(void* arg, FcitxInputContext* ic)
{

}

void WaylandIMCloseIM(void* arg, FcitxInputContext* ic)
{

}

void WaylandIMCommitString(void* arg, FcitxInputContext* ic, const char* str)
{
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);
    wl_input_method_context_commit_string (waylandic->context,
                                           waylandic->serial,
                                           str);
}

void WaylandIMUpdatePreedit(void* arg, FcitxInputContext* ic)
{
    FcitxWaylandIMFrontend* im = arg;
    FcitxInputState* input = FcitxInstanceGetInputState(im->owner);
    FcitxMessages* clientPreedit = FcitxInputStateGetClientPreedit(input);
    int i = 0;
    for (i = 0; i < FcitxMessagesGetMessageCount(clientPreedit) ; i ++) {
        char* str = FcitxMessagesGetMessageString(clientPreedit, i);
        if (!fcitx_utf8_check_string(str))
            return;
    }

    /* a small optimization, don't need to update empty preedit */
    FcitxWaylandIC* waylandic = GetWaylandIC(ic);
    if (waylandic->lastPreeditIsEmpty && FcitxMessagesGetMessageCount(clientPreedit) == 0)
        return;

    waylandic->lastPreeditIsEmpty = (FcitxMessagesGetMessageCount(clientPreedit) == 0);

    int cursor = FcitxInputStateGetClientCursorPos(input);
    wl_input_method_context_preedit_cursor (waylandic->context,
                                            cursor);
    char* strPreedit = FcitxUIMessagesToCString(FcitxInputStateGetClientPreedit(input));
    char* str = FcitxInstanceProcessOutputFilter(im->owner, strPreedit);
    if (str) {
        free(strPreedit);
        strPreedit = str;
    }
    char* strPreeditForCommit = FcitxUIMessagesToCStringForCommit(FcitxInputStateGetClientPreedit(input));
    str = FcitxInstanceProcessCommitFilter(im->owner, strPreeditForCommit);
    if (str) {
        free(strPreeditForCommit);
        strPreeditForCommit = str;
    }
    wl_input_method_context_preedit_string(waylandic->context,
                                           waylandic->serial,
                                           strPreedit,
                                           strPreeditForCommit
                                          );

    // wl_input_method_context_preedit_styling();
}

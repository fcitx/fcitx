#ifndef CLIENT_IM_H
#define CLIENT_IM_H


#include <gio/gio.h>
#include <fcitx/ime.h>
#include <fcitx/frontend.h>

G_BEGIN_DECLS

typedef struct _FcitxInputMethod        FcitxInputMethod;
typedef struct _FcitxInputMethodClass   FcitxInputMethodClass;

#define FCITX_TYPE_INPUTMETHOD         (fcitx_inputmethod_get_type ())
#define FCITX_INPUTMETHOD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FCITX_TYPE_INPUTMETHOD, FcitxInputMethod))
#define FCITX_INPUTMETHOD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FCITX_TYPE_INPUTMETHOD, FcitxInputMethodClass))
#define FCITX_INPUTMETHOD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FCITX_TYPE_INPUTMETHOD, FcitxInputMethodClass))


typedef struct _FcitxPreeditItem {
    gchar* string;
    gint32 type;
} FcitxPreeditItem;

struct _FcitxInputMethod {
    GObject parent_instance;
    GDBusProxy* improxy;
    GDBusProxy* icproxy;
    char servicename[64];
    char icname[64];
    int id;
    guint watch_id;
};

struct _FcitxInputMethodClass {
    GObjectClass parent_class;
};

typedef struct _FcitxIMItem {
    gchar* name;
    gchar* unique_name;
    gchar* langcode;
    gboolean enable;
} FcitxIMItem;


FcitxInputMethod* fcitx_inputmethod_new();

gboolean fcitx_inputmethod_is_valid(FcitxInputMethod* im);
int fcitx_inputmethod_process_key_sync(FcitxInputMethod* im, guint32 keyval, guint32 keycode, guint32 state, FcitxKeyEventType type, guint32 t);
void fcitx_inputmethod_process_key(FcitxInputMethod* im, GAsyncReadyCallback cb, gpointer user_data, guint32 keyval, guint32 keycode, guint32 state, FcitxKeyEventType type, guint32 t);

void fcitx_inputmethod_focusin(FcitxInputMethod* im);
void fcitx_inputmethod_focusout(FcitxInputMethod* im);
void fcitx_inputmethod_set_cusor_rect(FcitxInputMethod* im, int x, int y, int w, int h);
void fcitx_inputmethod_set_surrounding_text(FcitxInputMethod* im, gchar* text, guint cursor, guint anchor);
void fcitx_inputmethod_set_capacity(FcitxInputMethod* im, FcitxCapacityFlags flags);
void fcitx_inputmethod_reset(FcitxInputMethod* im);

GType        fcitx_inputmethod_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif //CLIENT_IM_H
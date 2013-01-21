#include "xim.h"
#include <fcitx/module.h>

typedef enum _XimCallType {
    XCT_FORWARD,
    XCT_COMMIT,
    XCT_CALLCALLBACK,
    XCT_PREEDIT_START,
    XCT_PREEDIT_END
} XimCallType;

typedef struct _XimQueue XimQueue;

void XimQueueInit(FcitxXimFrontend *xim);
void XimConsumeQueue(FcitxXimFrontend *xim);
void XimPendingCall(FcitxXimFrontend *xim, XimCallType type, XPointer ptr);
void XimQueueDestroy(FcitxXimFrontend *xim);

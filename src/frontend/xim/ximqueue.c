#include <X11/Xlib.h>
#include <IMdkit.h>
#include <Xi18n.h>

#include "fcitx/module.h"
#include "module/x11/x11stuff.h"
#include "ximqueue.h"

struct _XimQueue {
    XimCallType type;
    XPointer ptr;
};

static const UT_icd ptr_icd = { sizeof(XimQueue), NULL, NULL, NULL };

void XimQueueInit(FcitxXimFrontend* xim)
{
    utarray_new(xim->queue, &ptr_icd);
}

void XimCosumeQueue(void* arg, FcitxModuleFunctionArg args)
{
    FcitxXimFrontend* xim = arg;
    XimQueue* item;
    for (item = (XimQueue*) utarray_front(xim->queue);
         item != NULL;
         item = (XimQueue*) utarray_next(xim->queue, item))
    {
        FcitxModuleInvokeFunction(xim->x11addon, FCITX_X11_PROCESSREMAINEVENT, args);
        switch(item->type) {
        case XCT_FORWARD:
            IMForwardEvent(xim->ims, item->ptr);
            break;
        case XCT_CALLCALLBACK:
            {
                IMCallCallback(xim->ims, item->ptr);
                IMPreeditCBStruct* pcb = (IMPreeditCBStruct*) item->ptr;
                if (pcb->major_code == XIM_PREEDIT_DRAW) {
                    XFree(pcb->todo.draw.text->string.multi_byte);
                    free(pcb->todo.draw.text);
                }
            }
            break;
        case XCT_COMMIT:
            {
                IMCommitString(xim->ims, item->ptr);
                IMCommitStruct* cms = (IMCommitStruct*) item->ptr;
                XFree(cms->commit_string);
            }
            break;
        case XCT_PREEDIT_START:
            IMPreeditStart(xim->ims, item->ptr);
            break;
        case XCT_PREEDIT_END:
            IMPreeditEnd(xim->ims, item->ptr);
            break;
        }
        free(item->ptr);
    }

    utarray_clear(xim->queue);
}

void XimPendingCall(FcitxXimFrontend* xim, XimCallType type, XPointer ptr)
{
    XimQueue item;
    item.type = type;
    item.ptr = ptr;
    utarray_push_back(xim->queue, &item);
}
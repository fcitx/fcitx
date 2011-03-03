#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "core/fcitx.h"
#include "core/backend.h"
#include "ximhandler.h"
#include "IC.h"
#include "xim.h"

extern FcitxXimBackend xim;

static void SetTrackPos(IMChangeICStruct * call_data);

static void DoForwardEvent(IMForwardEventStruct * call_data);

Bool XIMOpenHandler(IMOpenStruct * call_data)
{
    return True;
}


Bool XIMGetICValuesHandler(IMChangeICStruct * call_data)
{
    GetIC(call_data);

    return True;
}

Bool XIMSetICValuesHandler(IMChangeICStruct * call_data)
{
    SetIC(call_data);
    SetTrackPos(call_data);

    return True;
}

Bool XIMSetFocusHandler(IMChangeFocusStruct * call_data)
{
    CurrentIC = FindIC(backend.backendid, &call_data->icid);

    return True;
}

Bool XIMUnsetFocusHandler(IMChangeICStruct * call_data)
{
    if (CurrentIC && CurrentXimIC->id == call_data->icid)
    {

    }

    return True;
}

Bool XIMCloseHandler(IMOpenStruct * call_data)
{

    return True;
}

Bool XIMCreateICHandler(IMChangeICStruct * call_data)
{
    CreateIC(backend.backendid, call_data);

    if (!CurrentIC) {
        CurrentIC = FindIC(backend.backendid, &call_data->icid);
    }

    return True;
}

Bool XIMDestroyICHandler(IMChangeICStruct * call_data)
{
    if (CurrentIC == FindIC(backend.backendid, &call_data->icid)) {
    }

    DestroyIC(backend.backendid, &call_data->icid);

    return True;
}

Bool XIMTriggerNotifyHandler(IMTriggerNotifyStruct * call_data)
{
    FcitxInputContext* ic = FindIC(backend.backendid, &call_data->icid);
    if (ic == NULL)
        return True;

    CurrentIC = ic;
    return True;
}


void SetTrackPos(IMChangeICStruct * call_data)
{
    Bool flag = False;
    if (CurrentIC == NULL)
        return;
    if (CurrentIC != FindIC(backend.backendid, &call_data->icid))
        return;

    if (true) {
        int i;
        XICAttribute *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

        for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
            if (!strcmp(XNSpotLocation, pre_attr->name)) {
                flag = True;
                
                CurrentIC->offset_x = (*(XPoint *) pre_attr->value).x;
                CurrentIC->offset_y = (*(XPoint *) pre_attr->value).y;
            }
        }
    }
}

void XIMProcessKey(IMForwardEventStruct * call_data)
{
    DoForwardEvent(call_data);
}

void DoForwardEvent(IMForwardEventStruct * call_data)
{
    IMForwardEventStruct fe;
    memset(&fe, 0, sizeof(fe));

    fe.major_code = XIM_FORWARD_EVENT;
    fe.icid = call_data->icid;
    fe.connect_id = call_data->connect_id;
    fe.sync_bit = 0;
    fe.serial_number = 0L;
    fe.event = call_data->event;

    IMForwardEvent(xim.ims, (XPointer) & fe);
}
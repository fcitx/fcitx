/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "config.h"

#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <libintl.h>
#include <pthread.h>
#include <semaphore.h>
#include <getopt.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#include "instance.h"
#include "fcitx-utils/log.h"
#include "ime-internal.h"
#include "ui.h"
#include "addon.h"
#include "module.h"
#include "frontend.h"
#include "fcitx-utils/utils.h"
#include "candidate.h"
#include "ui-internal.h"
#include "fcitx-internal.h"
#include "instance-internal.h"
#include "module-internal.h"
#include "addon-internal.h"
#include "setjmp.h"

#define CHECK_ENV(env, value, icase) (!getenv(env) \
                                      || (icase ? \
                                              (0 != strcmp(getenv(env), (value))) \
                                              : (0 != strcasecmp(getenv(env), (value)))))

FCITX_GETTER_REF(FcitxInstance, Addons, addons, UT_array)
FCITX_GETTER_REF(FcitxInstance, UIMenus, uimenus, UT_array)
FCITX_GETTER_REF(FcitxInstance, UIStats, uistats, UT_array)
FCITX_GETTER_REF(FcitxInstance, UIComplexStats, uicompstats, UT_array)
FCITX_GETTER_REF(FcitxInstance, IMEs, imes, UT_array)
FCITX_GETTER_REF(FcitxInstance, AvailIMEs, availimes, UT_array)
FCITX_GETTER_REF(FcitxInstance, ReadFDSet, rfds, fd_set)
FCITX_GETTER_REF(FcitxInstance, WriteFDSet, wfds, fd_set)
FCITX_GETTER_REF(FcitxInstance, ExceptFDSet, efds, fd_set)
FCITX_GETTER_VALUE(FcitxInstance, CurrentUI, ui, FcitxAddon*)
FCITX_GETTER_VALUE(FcitxInstance, MaxFD, maxfd, int)
FCITX_SETTER(FcitxInstance, MaxFD, maxfd, int)
FCITX_GETTER_VALUE(FcitxInstance, GlobalConfig, config, FcitxGlobalConfig*)
FCITX_GETTER_VALUE(FcitxInstance, Profile, profile, FcitxProfile*)
FCITX_GETTER_VALUE(FcitxInstance, InputState, input, FcitxInputState*)
FCITX_GETTER_VALUE(FcitxInstance, IsDestroying, destroy, boolean)

static const UT_icd stat_icd = {
    sizeof(FcitxUIStatus), NULL, NULL, NULL
};
static const UT_icd compstat_icd = {
    sizeof(FcitxUIComplexStatus), NULL, NULL, NULL
};
static const UT_icd timeout_icd = {
    sizeof(TimeoutItem), NULL, NULL, NULL
};
static const UT_icd icdata_icd = {
    sizeof(FcitxICDataInfo), NULL, NULL, NULL
};
static void FcitxInitThread(FcitxInstance* inst);
static void ToggleRemindState(void* arg);
static boolean GetRemindEnabled(void* arg);
static boolean ProcessOption(FcitxInstance* instance, int argc, char* argv[]);
static void Usage();
static void Version();
static void* RunInstance(void* arg);
static void FcitxInstanceInitBuiltContext(FcitxInstance* instance);
static void FcitxInstanceShowRemindStatusChanged(void* arg, const void* value);
static void FcitxInstanceRealEnd(FcitxInstance* instance);

/**
 * 显示命令行参数
 */
void Usage()
{
    printf("Usage: fcitx [OPTION]\n"
           "\t-r, --replace\t\ttry replace existing fcitx, need module support.\n"
           "\t-d\t\t\trun as daemon(default)\n"
           "\t-D\t\t\tdon't run as daemon\n"
           "\t-s[sleep time]\t\toverride delay start time in config file, 0 for immediate start\n"
           "\t-v\t\t\tdisplay the version information and exit\n"
           "\t-u, --ui\t\tspecify the user interface to use\n"
           "\t--enable\t\tspecify a comma separated list for addon that will override the enable option\n"
           "\t--disable\t\tspecify a comma separated list for addon that will explicitly disabled,\n"
           "\t\t\t\t\tpriority is lower than --enable, can use all for disable all module\n"
           "\t-h, --help\t\tdisplay this help and exit\n");
}

/**
 * 显示版本
 */
void Version()
{
    printf("fcitx version: %s\n", VERSION);
}

FCITX_EXPORT_API
FcitxInstance* FcitxInstanceCreate(sem_t *sem, int argc, char* argv[])
{
    return FcitxInstanceCreateWithFD(sem, argc, argv, -1);
}

FCITX_EXPORT_API
FcitxInstance* FcitxInstanceCreatePause(sem_t *sem, int argc, char* argv[], int fd)
{
    FcitxInstance* instance = fcitx_utils_malloc0(sizeof(FcitxInstance));
    FcitxAddonsInit(&instance->addons);
    FcitxInstanceInitIM(instance);
    FcitxFrontendsInit(&instance->frontends);
    InitFcitxModules(&instance->modules);
    InitFcitxModules(&instance->eventmodules);
    utarray_init(&instance->uistats, &stat_icd);
    utarray_init(&instance->uicompstats, &compstat_icd);
    utarray_init(&instance->uimenus, fcitx_ptr_icd);
    utarray_init(&instance->timeout, &timeout_icd);
    utarray_init(&instance->icdata, &icdata_icd);
    instance->input = FcitxInputStateCreate();
    instance->sem = sem;
    instance->config = fcitx_utils_malloc0(sizeof(FcitxGlobalConfig));
    instance->profile = fcitx_utils_malloc0(sizeof(FcitxProfile));
    instance->globalIMName = strdup("");
    instance->fd = -1;
    if (fd >= 0) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        instance->fd = fd;
    }

    if (!FcitxGlobalConfigLoad(instance->config))
        goto error_exit;

    FcitxCandidateWordSetPageSize(instance->input->candList, instance->config->iMaxCandWord);

    if (!ProcessOption(instance, argc, argv))
        goto error_exit;

    instance->timeStart = time(NULL);
    instance->globalState = instance->config->defaultIMState;
    instance->totaltime = 0;

    FcitxInitThread(instance);
    if (!FcitxProfileLoad(instance->profile, instance))
        goto error_exit;
    if (FcitxAddonGetConfigDesc() == NULL)
        goto error_exit;
    if (GetIMConfigDesc() == NULL)
        goto error_exit;

    FcitxAddonsLoad(&instance->addons);
    FcitxInstanceFillAddonOwner(instance, NULL);
    FcitxInstanceResolveAddonDependency(instance);
    FcitxInstanceInitBuiltInHotkey(instance);
    FcitxInstanceInitBuiltContext(instance);
    FcitxModuleLoad(instance);
    if (instance->loadingFatalError)
        return instance;
    if (!FcitxInstanceLoadAllIM(instance)) {
        FcitxInstanceEnd(instance);
        return instance;
    }

    FcitxInstanceInitIMMenu(instance);
    FcitxUIRegisterMenu(instance, &instance->imMenu);
    FcitxUIRegisterStatus(instance, instance, "remind",
                           instance->profile->bUseRemind ? _("Use remind") :  _("No remind"),
                          _("Toggle Remind"), ToggleRemindState, GetRemindEnabled);
    FcitxUISetStatusVisable(instance, "remind",  false);

    FcitxUILoad(instance);

    instance->iIMIndex = FcitxInstanceGetIMIndexByName(instance, instance->profile->imName);

    FcitxInstanceSwitchIMByIndex(instance, instance->iIMIndex);

    if (!FcitxInstanceLoadFrontend(instance)) {
        FcitxInstanceEnd(instance);
        return instance;
    }

    return instance;

error_exit:
    FcitxInstanceEnd(instance);
    return instance;
}

FCITX_EXPORT_API
void FcitxInstanceStart(FcitxInstance* instance)
{
    if (!instance->loadingFatalError) {
        instance->initialized = true;
        /* make in order to use block X, query is not good here */
        if (pthread_create(&instance->pid, NULL, RunInstance, instance) != 0)
            instance->initialized = false;
    }
}


FCITX_EXPORT_API
FcitxInstance* FcitxInstanceCreateWithFD(sem_t *sem, int argc, char* argv[], int fd)
{
    FcitxInstance* instance = FcitxInstanceCreatePause(sem, argc, argv, fd);

    FcitxInstanceStart(instance);

    return instance;

}

FCITX_EXPORT_API
void FcitxInstanceSetRecheckEvent(FcitxInstance* instance)
{
    instance->uiflag |= UI_EVENT_CHECK;
}

FCITX_EXPORT_API
jmp_buf FcitxRecover;

void* RunInstance(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    uint64_t curtime = 0;
    while (1) {
        FcitxAddon** pmodule;
        uint8_t signo = 0;
        if (instance->fd >= 0) {
            while (read(instance->fd, &signo, sizeof(char)) > 0) {
                if (signo == SIGINT || signo == SIGTERM || signo == SIGQUIT || signo == SIGXCPU)
                    FcitxInstanceEnd(instance);
                else if (signo == SIGHUP)
                    fcitx_utils_launch_restart();
                else if (signo == SIGUSR1)
                    FcitxInstanceReloadConfig(instance);
            }
        }
        do {
            instance->uiflag = UI_NONE;
            for (pmodule = (FcitxAddon**) utarray_front(&instance->eventmodules);
                    pmodule != NULL;
                    pmodule = (FcitxAddon**) utarray_next(&instance->eventmodules, pmodule)) {
                FcitxModule* module = (*pmodule)->module;
                module->ProcessEvent((*pmodule)->addonInstance);
            }
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            curtime = (current_time.tv_sec * 1000LL) + (current_time.tv_usec / 1000LL);

            unsigned int idx = 0;
            while(idx < utarray_len(&instance->timeout))
            {
                TimeoutItem* ti = (TimeoutItem*) utarray_eltptr(&instance->timeout, idx);
                uint64_t id = ti->idx;
                if (ti->time + ti->milli <= curtime) {
                    ti->callback(ti->arg);
                    ti = (TimeoutItem*) utarray_eltptr(&instance->timeout, idx);
                    /* faster remove */
                    if (ti && ti->idx == id)
                        utarray_remove_quick(&instance->timeout, idx);
                    else {
                        FcitxInstanceRemoveTimeoutById(instance, id);
                        idx = 0;
                    }
                }
                else {
                    idx++;
                }
            }

            if (instance->uiflag & UI_MOVE)
                FcitxUIMoveInputWindowReal(instance);

            if (instance->uiflag & UI_UPDATE)
                FcitxUIUpdateInputWindowReal(instance);
        } while (instance->uiflag != UI_NONE);

        setjmp(FcitxRecover);

        if (instance->destroy) {
            FcitxInstanceRealEnd(instance);
            break;
        }

        FD_ZERO(&instance->rfds);
        FD_ZERO(&instance->wfds);
        FD_ZERO(&instance->efds);

        instance->maxfd = 0;
        if (instance->fd > 0) {
            instance->maxfd = instance->fd;
            FD_SET(instance->fd, &instance->rfds);
        }
        for (pmodule = (FcitxAddon**) utarray_front(&instance->eventmodules);
                pmodule != NULL;
                pmodule = (FcitxAddon**) utarray_next(&instance->eventmodules, pmodule)) {
            FcitxModule* module = (*pmodule)->module;
            module->SetFD((*pmodule)->addonInstance);
        }
        if (instance->maxfd == 0)
            break;
        struct timeval tval;
        struct timeval* ptval = NULL;
        if (utarray_len(&instance->timeout) != 0) {
            unsigned long int min_time = LONG_MAX;
            TimeoutItem* ti;
            for (ti = (TimeoutItem*)utarray_front(&instance->timeout);ti;
                 ti = (TimeoutItem*)utarray_next(&instance->timeout, ti)) {
                if (ti->time + ti->milli - curtime < min_time) {
                    min_time = ti->time + ti->milli - curtime;
                }
            }
            tval.tv_usec = (min_time % 1000) * 1000;
            tval.tv_sec = min_time / 1000;
            ptval = &tval;
        }
        select(instance->maxfd + 1, &instance->rfds, &instance->wfds,
               &instance->efds, ptval);
    }
    return NULL;
}

FCITX_EXPORT_API
void FcitxInstanceEnd(FcitxInstance* instance)
{
    /* avoid duplicate destroy */
    if (instance->destroy)
        return;

    if (!instance->initialized) {
        if (!instance->loadingFatalError) {
            if (!instance->quietQuit)
                FcitxLog(ERROR, "Exiting.");
            instance->loadingFatalError = true;
            sem_post(instance->sem);
        }
        return;
    }

    instance->destroy = true;
}

FCITX_EXPORT_API
void FcitxInstanceRealEnd(FcitxInstance* instance) {

    FcitxProfileSave(instance->profile);
    FcitxInstanceSaveAllIM(instance);

    if (instance->uinormal && instance->uinormal->ui->Destroy)
        instance->uinormal->ui->Destroy(instance->uinormal->addonInstance);

    if (instance->uifallback && instance->uifallback->ui->Destroy)
        instance->uifallback->ui->Destroy(instance->uifallback->addonInstance);

    instance->uifallback = NULL;
    instance->ui = NULL;
    instance->uinormal = NULL;

    /* handle exit */
    FcitxAddon** pimclass;
    FcitxAddon** pfrontend;
    FcitxFrontend* frontend;
    FcitxInputContext* rec = NULL;

    for (pimclass = (FcitxAddon**)utarray_front(&instance->imeclasses);
         pimclass != NULL;
         pimclass = (FcitxAddon**)utarray_next(&instance->imeclasses, pimclass)
        ) {
        if ((*pimclass)->imclass->Destroy)
            (*pimclass)->imclass->Destroy((*pimclass)->addonInstance);
    }

    for (rec = instance->ic_list; rec != NULL; rec = rec->next) {
        pfrontend = (FcitxAddon**)utarray_eltptr(&instance->frontends,
                                                 (unsigned int)rec->frontendid);
        frontend = (*pfrontend)->frontend;
        frontend->CloseIM((*pfrontend)->addonInstance, rec);
    }

    for (rec = instance->ic_list; rec != NULL; rec = rec->next) {
        pfrontend = (FcitxAddon**)utarray_eltptr(&instance->frontends,
                                                 (unsigned int)rec->frontendid);
        frontend = (*pfrontend)->frontend;
        frontend->DestroyIC((*pfrontend)->addonInstance, rec);
    }

    for (pfrontend = (FcitxAddon**)utarray_front(&instance->frontends);
         pfrontend != NULL;
         pfrontend = (FcitxAddon**)utarray_next(&instance->frontends, pfrontend)
        ) {
        if (pfrontend == NULL)
            continue;
        FcitxFrontend* frontend = (*pfrontend)->frontend;
        frontend->Destroy((*pfrontend)->addonInstance);
    }

    FcitxAddon** pmodule;
    for (pmodule = (FcitxAddon**) utarray_front(&instance->modules);
         pmodule != NULL;
         pmodule = (FcitxAddon**) utarray_next(&instance->modules, pmodule)) {
        if (pmodule == NULL)
            return;
        FcitxModule* module = (*pmodule)->module;
        if (module->Destroy)
            module->Destroy((*pmodule)->addonInstance);
    }

    sem_post(instance->sem);
}

void FcitxInitThread(FcitxInstance* inst)
{
    int rc;
    rc = pthread_mutex_init(&inst->fcitxMutex, NULL);
    if (rc != 0)
        FcitxLog(ERROR, _("pthread mutex init failed"));
}

FCITX_EXPORT_API
int FcitxInstanceLock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_lock(&inst->fcitxMutex);
    return 0;
}

FCITX_EXPORT_API
int FcitxInstanceUnlock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_unlock(&inst->fcitxMutex);
    return 0;
}

void ToggleRemindState(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    instance->profile->bUseRemind = !instance->profile->bUseRemind;
    FcitxUISetStatusString(instance, "remind",
                           instance->profile->bUseRemind ? _("Use remind") :  _("No remind"),
                          _("Toggle Remind"));
    FcitxProfileSave(instance->profile);
}

boolean GetRemindEnabled(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    return instance->profile->bUseRemind;
}

boolean ProcessOption(FcitxInstance* instance, int argc, char* argv[])
{
    struct option longOptions[] = {
        {"ui", 1, 0, 0},
        {"replace", 0, 0, 0},
        {"enable", 1, 0, 0},
        {"disable", 1, 0, 0},
        {"help", 0, 0, 0},
        {NULL, 0, 0, 0}
    };

    int optionIndex = 0;
    int c;
    char* uiname = NULL;
    boolean runasdaemon = true;
    int             overrideDelay = -1;
    while ((c = getopt_long(argc, argv, "ru:dDs:hv", longOptions, &optionIndex)) != EOF) {
        switch (c) {
        case 0: {
            switch (optionIndex) {
            case 0:
                uiname = strdup(optarg);
                break;
            case 1:
                instance->tryReplace = true;
                break;
            case 2:
                {
                    if (instance->enableList)
                        fcitx_utils_free_string_list(instance->enableList);
                    instance->enableList = fcitx_utils_split_string(optarg, ',');
                }
                break;
            case 3:
                {
                    if (instance->disableList)
                        fcitx_utils_free_string_list(instance->disableList);
                    instance->disableList = fcitx_utils_split_string(optarg, ',');
                }
                break;
            default:
                instance->quietQuit = true;
                Usage();
                return false;
            }
        }
        break;
        case 'r':
            instance->tryReplace = true;
            break;
        case 'u':
            uiname = strdup(optarg);
            break;
        case 'd':
            runasdaemon = true;
            break;
        case 'D':
            runasdaemon = false;
            break;
        case 's':
            overrideDelay = atoi(optarg);
            break;
        case 'h':
            Usage();
            return false;
        case 'v':
            Version();
            return false;
            break;
        default:
            Usage();
            return false;
        }
    }

    if (uiname)
        instance->uiname = uiname;
    else
        instance->uiname = NULL;

    if (runasdaemon)
        fcitx_utils_init_as_daemon();

    if (overrideDelay < 0)
        overrideDelay = instance->config->iDelayStart;

    if (overrideDelay > 0)
        sleep(overrideDelay);

    return true;
}

FCITX_EXPORT_API
FcitxInputContext* FcitxInstanceGetCurrentIC(FcitxInstance* instance)
{
    return instance->CurrentIC;
}

FCITX_EXPORT_API
boolean FcitxInstanceSetCurrentIC(FcitxInstance* instance, FcitxInputContext* ic)
{
    FcitxContextState prevstate = FcitxInstanceGetCurrentState(instance);
    boolean changed = (instance->CurrentIC != ic);

    instance->CurrentIC = ic;

    FcitxContextState nextstate = FcitxInstanceGetCurrentState(instance);

    if (!((prevstate == IS_CLOSED && nextstate == IS_CLOSED) || (prevstate != IS_CLOSED && nextstate != IS_CLOSED))) {
        if (prevstate == IS_CLOSED)
            instance->timeStart = time(NULL);
        else
            instance->totaltime += difftime(time(NULL), instance->timeStart);
    }

    return changed;
}

void FcitxInstanceInitBuiltContext(FcitxInstance* instance)
{
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, FCT_Hotkey, FCF_ResetOnInputMethodChange);
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, FCT_Hotkey, FCF_ResetOnInputMethodChange);
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_IM_KEYBOARD_LAYOUT, FCT_String, FCF_ResetOnInputMethodChange);
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_IM_LANGUAGE, FCT_String, FCF_None);
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_SHOW_REMIND_STATUS, FCT_Boolean, FCF_ResetOnInputMethodChange);
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT, FCT_Boolean, FCF_ResetOnInputMethodChange);

    FcitxInstanceWatchContext(instance, CONTEXT_SHOW_REMIND_STATUS, FcitxInstanceShowRemindStatusChanged, instance);
}

void FcitxInstanceShowRemindStatusChanged(void* arg, const void* value)
{
    const boolean* b = value;
    FcitxInstance* instance = arg;
    FcitxUISetStatusVisable(instance, "remind",  *b);
}

FCITX_EXPORT_API
boolean FcitxInstanceIsTryReplace(FcitxInstance* instance)
{
    return instance->tryReplace;
}

FCITX_EXPORT_API
void FcitxInstanceResetTryReplace(FcitxInstance* instance)
{
    instance->tryReplace = false;
}

FCITX_EXPORT_API
uint64_t FcitxInstanceAddTimeout(FcitxInstance* instance, long int milli, FcitxTimeoutCallback callback , void* arg)
{
    if (milli < 0)
        return 0;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    TimeoutItem item;
    item.arg = arg;
    item.callback =callback;
    item.milli = milli;
    item.idx = ++instance->timeoutIdx;
    item.time = (current_time.tv_sec * 1000LL) + (current_time.tv_usec / 1000LL);
    utarray_push_back(&instance->timeout, &item);

    return item.idx;
}

FCITX_EXPORT_API
boolean FcitxInstanceCheckTimeoutByFunc(FcitxInstance* instance, FcitxTimeoutCallback callback)
{
    TimeoutItem* ti;
    for (ti = (TimeoutItem*) utarray_front(&instance->timeout);
         ti != NULL;
         ti = (TimeoutItem*) utarray_next(&instance->timeout, ti))
    {
        if (ti->callback == callback)
            return true;
    }
    return false;
}

FCITX_EXPORT_API
boolean FcitxInstanceCheckTimeoutById(FcitxInstance *instance, uint64_t id)
{
    TimeoutItem *ti;
    for (ti = (TimeoutItem*)utarray_front(&instance->timeout);ti;
         ti = (TimeoutItem*)utarray_next(&instance->timeout, ti)) {
        if (ti->idx == id)
            return true;
    }
    return false;
}

FCITX_EXPORT_API boolean
FcitxInstanceRemoveTimeoutByFunc(FcitxInstance* instance,
                                 FcitxTimeoutCallback callback)
{
    TimeoutItem* ti;
    for (ti = (TimeoutItem*) utarray_front(&instance->timeout);
         ti != NULL;
         ti = (TimeoutItem*) utarray_next(&instance->timeout, ti)) {
        if (ti->callback == callback) {
            unsigned int idx = utarray_eltidx(&instance->timeout, ti);
            utarray_remove_quick(&instance->timeout, idx);
            return true;
        }
    }
    return false;
}

FCITX_EXPORT_API
boolean FcitxInstanceRemoveTimeoutById(FcitxInstance* instance, uint64_t id)
{
    if (id == 0)
        return false;
    TimeoutItem* ti;
    for (ti = (TimeoutItem*) utarray_front(&instance->timeout);
         ti != NULL;
         ti = (TimeoutItem*) utarray_next(&instance->timeout, ti)) {
        if (ti->idx == id) {
            unsigned int idx = utarray_eltidx(&instance->timeout, ti);
            utarray_remove_quick(&instance->timeout, idx);
            return true;
        }
    }
    return false;
}

FCITX_EXPORT_API
int FcitxInstanceWaitForEnd(FcitxInstance* instance) {
    if (instance->initialized) {
        return pthread_join(instance->pid, NULL);
    }
    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;

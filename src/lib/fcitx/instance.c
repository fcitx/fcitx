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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <libintl.h>
#include <pthread.h>
#include <semaphore.h>
#include <getopt.h>

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

#define CHECK_ENV(env, value, icase) (!getenv(env) \
                                      || (icase ? \
                                              (0 != strcmp(getenv(env), (value))) \
                                              : (0 != strcasecmp(getenv(env), (value)))))

FCITX_GETTER_REF(FcitxInstance, Addons, addons, UT_array)
FCITX_GETTER_REF(FcitxInstance, UIMenus, uimenus, UT_array)
FCITX_GETTER_REF(FcitxInstance, UIStats, uistats, UT_array)
FCITX_GETTER_REF(FcitxInstance, IMEs, imes, UT_array)
FCITX_GETTER_REF(FcitxInstance, AvailIMEs, availimes, UT_array)
FCITX_GETTER_REF(FcitxInstance, ReadFDSet, rfds, fd_set)
FCITX_GETTER_REF(FcitxInstance, WriteFDSet, wfds, fd_set)
FCITX_GETTER_REF(FcitxInstance, ExceptFDSet, efds, fd_set)
FCITX_GETTER_VALUE(FcitxInstance, MaxFD, maxfd, int)
FCITX_SETTER(FcitxInstance, MaxFD, maxfd, int)
FCITX_GETTER_VALUE(FcitxInstance, Config, config, FcitxConfig*)
FCITX_GETTER_VALUE(FcitxInstance, Profile, profile, FcitxProfile*)
FCITX_GETTER_VALUE(FcitxInstance, InputState, input, FcitxInputState*)

const UT_icd stat_icd = {sizeof(FcitxUIStatus), 0, 0, 0};
const UT_icd menup_icd = {sizeof(FcitxUIMenu*), 0, 0, 0};
static void FcitxInitThread(FcitxInstance* inst);
static void ToggleRemindState(void* arg);
static boolean GetRemindEnabled(void* arg);
static boolean ProcessOption(FcitxInstance* instance, int argc, char* argv[]);
static void Usage();
static void Version();
static void* RunInstance(void* arg);

/**
 * @brief 显示命令行参数
 */
void Usage()
{
    printf("Usage: fcitx [OPTION]\n"
           "\t-d\t\t\trun as daemon(default)\n"
           "\t-D\t\t\tdon't run as daemon\n"
           "\t-s[sleep time]\t\toverride delay start time in config file, 0 for immediate start\n"
           "\t-v\t\t\tdisplay the version information and exit\n"
           "\t-u, --ui\t\tspecify the user interface to use\n"
           "\t-h, --help\t\tdisplay this help and exit\n");
}

/**
 * @brief 显示版本
 */
void Version()
{
    printf("fcitx version: %s\n", VERSION);
}

FCITX_EXPORT_API
FcitxInstance* CreateFcitxInstance(sem_t *sem, int argc, char* argv[])
{
    FcitxInstance* instance = fcitx_malloc0(sizeof(FcitxInstance));
    InitFcitxAddons(&instance->addons);
    InitFcitxIM(instance);
    InitFcitxFrontends(&instance->frontends);
    InitFcitxModules(&instance->eventmodules);
    utarray_init(&instance->uistats, &stat_icd);
    utarray_init(&instance->uimenus, &menup_icd);
    instance->input = CreateFcitxInputState();
    instance->sem = sem;
    instance->config = fcitx_malloc0(sizeof(FcitxConfig));
    instance->profile = fcitx_malloc0(sizeof(FcitxProfile));

    if (!LoadConfig(instance->config))
        goto error_exit;

    CandidateWordSetPageSize(instance->input->candList, instance->config->iMaxCandWord);

    if (!ProcessOption(instance, argc, argv))
        goto error_exit;

    instance->timeStart = time(NULL);
    instance->globalState = instance->config->defaultIMState;
    instance->totaltime = 0;

    FcitxInitThread(instance);
    if (!LoadProfile(instance->profile, instance))
        goto error_exit;
    if (GetAddonConfigDesc() == NULL)
        goto error_exit;

    LoadAddonInfo(&instance->addons);
    AddonResolveDependency(instance);
    InitBuiltInHotkey(instance);
    LoadModule(instance);
    if (!LoadAllIM(instance)) {
        EndInstance(instance);
        return instance;
    }

    InitIMMenu(instance);
    RegisterMenu(instance, &instance->imMenu);
    RegisterStatus(instance, instance, "remind", _("Remind"), _("Remind"), ToggleRemindState, GetRemindEnabled);

    LoadUserInterface(instance);

    instance->iIMIndex = GetIMIndexByName(instance, instance->profile->imName);

    SwitchIM(instance, instance->iIMIndex);

    if (!LoadFrontend(instance)) {
        EndInstance(instance);
        return instance;
    }

    if (instance->config->bFirstRun) {
        instance->config->bFirstRun = false;
        SaveConfig(instance->config);

        const char *imname = "fcitx";
        char strTemp[PATH_MAX];
        snprintf(strTemp, PATH_MAX, "@im=%s", imname);
        strTemp[PATH_MAX - 1] = '\0';

        if ((getenv("XMODIFIERS") != NULL && CHECK_ENV("XMODIFIERS", strTemp, true)) ||
                (CHECK_ENV("GTK_IM_MODULE", "xim", false) && CHECK_ENV("GTK_IM_MODULE", "fcitx", false))
                || (CHECK_ENV("QT_IM_MODULE", "xim", false) && CHECK_ENV("QT_IM_MODULE", "fcitx", false))) {
            char *msg[12];
            msg[0] = _("Please check your environment varibles.");
            msg[1] = _("You can use tools provided by your distribution.");
            msg[2] = _("Or You may need to set environment varibles below to make fcitx work correctly.");
            msg[3] = "export XMODIFIERS=\"@im=fcitx\"";
            msg[4] = "export QT_IM_MODULE=xim";
            msg[5] = "export GTK_IM_MODULE=xim";
            msg[6] = _("Or (Depends on you install im module or not)");
            msg[7] = "export XMODIFIERS=\"@im=fcitx\"";
            msg[8] = "export QT_IM_MODULE=fcitx";
            msg[9] = "export GTK_IM_MODULE=fcitx";
            msg[10] = _("If you use login manager like gdm or kdm, put those lines in your ~/.xprofile.");
            msg[11] = _("If you use ~/.xinitrc and startx, put those lines in ~/.xinitrc.");

            DisplayMessage(instance, _("Setting Hint"), msg, 12);
        }
    }
    /* make in order to use block X, query is not good here */
    pthread_create(&instance->pid, NULL, RunInstance, instance);

    return instance;

error_exit:
    EndInstance(instance);
    return instance;

}

void* RunInstance(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    while (1) {
        FcitxAddon** pmodule;
        do {
            instance->uiflag = UI_NONE;
            for (pmodule = (FcitxAddon**) utarray_front(&instance->eventmodules);
                    pmodule != NULL;
                    pmodule = (FcitxAddon**) utarray_next(&instance->eventmodules, pmodule)) {
                FcitxModule* module = (*pmodule)->module;
                module->ProcessEvent((*pmodule)->addonInstance);
            }

            if (instance->uiflag & UI_MOVE)
                MoveInputWindowReal(instance);

            if (instance->uiflag & UI_UPDATE)
                UpdateInputWindowReal(instance);
        } while (instance->uiflag != UI_NONE);

        FD_ZERO(&instance->rfds);
        FD_ZERO(&instance->wfds);
        FD_ZERO(&instance->efds);

        instance->maxfd = 0;
        for (pmodule = (FcitxAddon**) utarray_front(&instance->eventmodules);
                pmodule != NULL;
                pmodule = (FcitxAddon**) utarray_next(&instance->eventmodules, pmodule)) {
            FcitxModule* module = (*pmodule)->module;
            module->SetFD((*pmodule)->addonInstance);
        }
        if (instance->maxfd == 0)
            break;
        select(instance->maxfd + 1, &instance->rfds, &instance->wfds, &instance->efds, NULL);
    }
    return NULL;
}

FCITX_EXPORT_API
void EndInstance(FcitxInstance* instance)
{
    SaveAllIM(instance);

    /* handle exit */
    FcitxAddon** pimclass;
    FcitxAddon** pfrontend;
    FcitxFrontend* frontend;
    FcitxInputContext* rec = NULL;

    for (pimclass = (FcitxAddon**) utarray_front(&instance->imeclasses);
            pimclass != NULL;
            pimclass = (FcitxAddon**) utarray_next(&instance->imeclasses, pimclass)
        ) {
        if ((*pimclass)->imclass->Destroy)
            (*pimclass)->imclass->Destroy((*pimclass)->addonInstance);
    }

    for (rec = instance->ic_list; rec != NULL; rec = rec->next) {
        pfrontend = (FcitxAddon**) utarray_eltptr(&instance->frontends, rec->frontendid);
        frontend = (*pfrontend)->frontend;
        frontend->CloseIM((*pfrontend)->addonInstance, rec);
    }

    for (rec = instance->ic_list; rec != NULL; rec = rec->next) {
        pfrontend = (FcitxAddon**) utarray_eltptr(&instance->frontends, rec->frontendid);
        frontend = (*pfrontend)->frontend;
        frontend->DestroyIC((*pfrontend)->addonInstance, rec);
    }

    int frontendid = 0;
    for (pfrontend = (FcitxAddon**) utarray_front(&instance->frontends);
            pfrontend != NULL;
            pfrontend = (FcitxAddon**) utarray_next(&instance->frontends, pfrontend)
        ) {
        if (pfrontend == NULL)
            return;
        FcitxFrontend* frontend = (*pfrontend)->frontend;
        frontend->Destroy((*pfrontend)->addonInstance);
        frontendid++;
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
int FcitxLock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_lock(&inst->fcitxMutex);
    return 0;
}

FCITX_EXPORT_API
int FcitxUnlock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_unlock(&inst->fcitxMutex);
    return 0;
}

void ToggleRemindState(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    instance->profile->bUseRemind = !instance->profile->bUseRemind;
    SaveProfile(instance->profile);
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
        {"help", 0, 0, 0}
    };

    int optionIndex = 0;
    int c;
    char* uiname = NULL;
    boolean runasdaemon = true;
    int             overrideDelay = -1;
    while ((c = getopt_long(argc, argv, "u:dDs:hv", longOptions, &optionIndex)) != EOF) {
        switch (c) {
        case 0: {
            switch (optionIndex) {
            case 0:
                uiname = strdup(optarg);
                break;
            default:
                Usage();
                return false;
            }
        }
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
        InitAsDaemon();

    if (overrideDelay < 0)
        overrideDelay = instance->config->iDelayStart;

    if (overrideDelay > 0)
        sleep(overrideDelay);

    return true;
}

FCITX_EXPORT_API
FcitxInputContext* GetCurrentIC(FcitxInstance* instance)
{
    return instance->CurrentIC;
}

FCITX_EXPORT_API
boolean SetCurrentIC(FcitxInstance* instance, FcitxInputContext* ic)
{
    IME_STATE prevstate = GetCurrentState(instance);
    boolean changed = (instance->CurrentIC != ic);
    instance->CurrentIC = ic;

    IME_STATE nextstate = GetCurrentState(instance);

    if (!((prevstate == IS_CLOSED && nextstate == IS_CLOSED) || (prevstate != IS_CLOSED && nextstate != IS_CLOSED))) {
        if (prevstate == IS_CLOSED)
            instance->timeStart = time(NULL);
        else
            instance->totaltime += difftime(time(NULL), instance->timeStart);
    }

    return changed;
}

FCITX_EXPORT_API
void FcitxInstanceIncreateInputCharacterCount(FcitxInstance* instance, int count)
{
    instance += count;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;

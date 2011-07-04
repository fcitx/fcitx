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

#include "instance.h"
#include <fcitx-utils/cutils.h>
#include <limits.h>
#include "ime-internal.h"
#include "ui.h"
#include <libintl.h>
#include <pthread.h>
#include "addon.h"
#include "module.h"
#include "backend.h"
#include <semaphore.h>
#include <getopt.h>
#include <fcitx-utils/utils.h>
#include <unistd.h>


#define CHECK_ENV(env, value, icase) (!getenv(env) \
        || (icase ? \
            (0 != strcmp(getenv(env), (value))) \
            : (0 != strcasecmp(getenv(env), (value)))))

const UT_icd stat_icd = {sizeof(FcitxUIStatus), 0, 0, 0};
const UT_icd menup_icd = {sizeof(FcitxUIMenu*), 0, 0, 0};
static void FcitxInitThread(FcitxInstance* inst);
static void ToggleLegendState(void* arg);
static boolean GetLegendEnabled(void* arg);
static void ProcessOption(FcitxInstance* instance, int argc, char* argv[]);
static void Usage();
static void Version();

/**
 * @brief 显示命令行参数
 */
void Usage ()
{
    printf("Usage: fcitx [OPTION]\n"
           "\t-d\t\trun as daemon(default)\n"
           "\t-D\t\tdon't run as daemon\n"
           "\t-s[sleep time]\toverride delay start time in config file, 0 for immediate start\n"
           "\t-v\t\tdisplay the version information and exit\n"
           "\t-u, --ui\t\tspecify the user interface to use\n"
           "\t-h\t\tdisplay this help and exit\n");
}

/**
 * @brief 显示版本
 */
void Version ()
{
    printf ("fcitx version: %s\n", VERSION);
}



FcitxInstance* CreateFcitxInstance(sem_t *sem, int argc, char* argv[])
{
    FcitxInstance* instance = fcitx_malloc0(sizeof(FcitxInstance));
    InitFcitxAddons(&instance->addons);
    InitFcitxIM(instance);
    InitFcitxBackends(&instance->backends);
    utarray_init(&instance->uistats, &stat_icd);
    utarray_init(&instance->uimenus, &menup_icd);
    instance->messageDown = InitMessages();
    instance->messageUp = InitMessages();
    instance->sem = sem;
    
    LoadConfig(&instance->config);
    ProcessOption(instance, argc, argv);
    
    FcitxInitThread(instance);
    LoadProfile(&instance->profile);
    LoadAddonInfo(&instance->addons);
    AddonResolveDependency(instance);
    InitBuiltInHotkey(instance);
    LoadModule(instance);
    LoadAllIM(instance);
    
    InitIMMenu(instance);
    RegisterMenu(instance, &instance->imMenu);
    RegisterStatus(instance, instance, "legend", "Legend", "Legend", ToggleLegendState, GetLegendEnabled);
    
    LoadUserInterface(instance);

    SwitchIM(instance, instance->iIMIndex);
    
    LoadBackend(instance);
    
    if (instance->config.bFirstRun)
    {
        instance->config.bFirstRun = false;
        SaveConfig(&instance->config);
        
        const char *imname = "fcitx";
        char strTemp[PATH_MAX];
        snprintf(strTemp, PATH_MAX, "@im=%s", imname);
        strTemp[PATH_MAX - 1] = '\0';

        if ((getenv("XMODIFIERS") != NULL && CHECK_ENV("XMODIFIERS", strTemp, true)) ||
            CHECK_ENV("GTK_IM_MODULE", "xim", false) || CHECK_ENV("QT_IM_MODULE", "xim", false)) {
            char *msg[7];
            msg[0] = _("Please check your environment varibles.");
            msg[1] = _("You may need to set environment varibles below to make fcitx work correctly.");
            msg[2] = "export XMODIFIERS=\"@im=fcitx\"";
            msg[3] = "export QT_IM_MODULE=xim";
            msg[4] = "export GTK_IM_MODULE=xim";
            msg[5] = _("If you use login manager like gdm or kdm, put those lines in your ~/.xprofile.");
            msg[6] = _("If you use ~/.xinitrc and startx, put those lines in ~/.xinitrc.");

            DisplayMessage(instance, _("Setting Hint"), msg, 7);
        }
    }
    /* make in order to use block X, query is not good here */
    RunModule(instance);

    return instance;
}

void EndInstance(FcitxInstance* instance)
{
    sem_post(instance->sem);
}

Messages* GetMessageUp(FcitxInstance *instance)
{
    return instance->messageUp;
}

Messages* GetMessageDown(FcitxInstance *instance)
{
    return instance->messageDown;
}

void FcitxInitThread(FcitxInstance* inst)
{
    int rc;
    rc = pthread_mutex_init(&inst->fcitxMutex, NULL);
    if (rc != 0)
        FcitxLog(ERROR, _("pthread mutex init failed"));
}


int FcitxLock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_lock(&inst->fcitxMutex);
    return 0;
}

int FcitxUnlock(FcitxInstance* inst)
{
    if (inst->bMutexInited)
        return pthread_mutex_unlock(&inst->fcitxMutex);
    return 0;
}

void ToggleLegendState(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    instance->profile.bUseLegend = !instance->profile.bUseLegend;
}

boolean GetLegendEnabled(void* arg)
{
    FcitxInstance* instance = (FcitxInstance*) arg;
    return instance->profile.bUseLegend;
}

void ProcessOption(FcitxInstance* instance, int argc, char* argv[])
{
    struct option longOptions[] ={
        {"ui", 1, 0, 0}
    };
    
    int optionIndex = 0;
    int c;
    char* uiname = NULL;
    boolean runasdaemon = true;
    int             overrideDelay = -1;
    while((c = getopt_long(argc, argv, "u:dDs:hv", longOptions, &optionIndex)) != EOF)
    {
        switch(c)
        {
            case 0:
                {
                    switch(optionIndex)
                    {
                        case 0:
                            uiname = strdup(optarg);
                            break;
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
                exit(0);
            case 'v':
                Version();
                exit(0);
                break;
        }
    }
    
    if (uiname)
        instance->uiname = uiname;
    else
        instance->uiname = NULL;
    
    if (runasdaemon)
        InitAsDaemon();    
    
    if (overrideDelay < 0)
        overrideDelay = instance->config.iDelayStart;

    if (overrideDelay > 0)
        sleep(overrideDelay);
}

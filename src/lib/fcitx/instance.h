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

#include "fcitx/ui.h"
#include "fcitx-utils/utarray.h"
#include <fcitx/configfile.h>
#include <fcitx/profile.h>
#include "addon.h"
#include "ime.h"
#include <semaphore.h>

struct FcitxInputContext;

typedef struct FcitxInstance {
    pthread_mutex_t fcitxMutex;
    int iIMIndex;
    int bShowCursor;
    Messages* messageUp;
    Messages* messageDown;
    UT_array uistats;
    UT_array uimenus;
    FcitxAddon* ui;
    FcitxInputState input;
    boolean bMutexInited;
    FcitxUIMenu imMenu;
    
    /* Fcitx is not good at multi process, so put a readonlyMode in it */
    boolean readonlyMode;
    
    /* config file */
    FcitxConfig config;
    FcitxProfile profile;
    UT_array addons;
    UT_array imeclasses;
    UT_array imes;
    UT_array backends;
    
    struct FcitxInputContext *CurrentIC;
    struct FcitxInputContext *ic_list;
    struct FcitxInputContext *free_list;
    sem_t* sem;
} FcitxInstance;

Messages* GetMessageUp(FcitxInstance* instance);
Messages* GetMessageDown(FcitxInstance* instance);

FcitxInstance* CreateFcitxInstance(sem_t *sem);
int FcitxLock(FcitxInstance* instance);
int FcitxUnlock(FcitxInstance* instance);
void EndInstance(FcitxInstance* instance);
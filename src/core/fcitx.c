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

/**
 * @mainpage Fcitx
 * fcitx is a lightweight Input Method Framework, written by C.
 * It can be used under X11 to support international input.
 * 
 */

/**
 * @file fcitx.c
 * @brief Main Function for fcitx
 */

#include <locale.h>
#include <libintl.h>
#include <unistd.h>
#include <semaphore.h>

#include "fcitx/configfile.h"
#include "fcitx/addon.h"
#include "fcitx/module.h"
#include "fcitx/ime-internal.h"
#include "fcitx/backend.h"
#include "fcitx/profile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "errorhandler.h"

FcitxInstance* instance;

static void WaitForEnd(sem_t *sem, int count)
{
    while (count)
    {
        sem_wait(sem);
        count --;
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", LOCALEDIR);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");
    SetMyExceptionHandler();
    
    int instanceCount = 1;
    
    sem_t sem;
    sem_init(&sem, 0, 0);
    
    instance = CreateFcitxInstance(&sem, argc, argv);
    
    WaitForEnd(&sem, instanceCount);
	return 0;
}
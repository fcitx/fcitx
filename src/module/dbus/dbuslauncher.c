/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "dbuslauncher.h"
#include "fcitx-utils/utils.h"

#ifndef DBUS_LAUNCH
#define DBUS_LAUNCH "dbus-launch"
#endif

#define BUFSIZE 1024

DBusDaemonProperty DBusLaunch(const char* configFile)
{
    const char *config_file_args[2];
    if (configFile) {
        config_file_args[0] = "--config-file=";
        config_file_args[1] = configFile;
    } else {
        config_file_args[0] = "";
        config_file_args[1] = "";
    }
    fcitx_local_cat_strings(command, DBUS_LAUNCH" --binary-syntax ",
                            config_file_args[0], config_file_args[1]);
    DBusDaemonProperty result = {0, NULL};
    FILE *fp = popen(command, "r");

    do {
        if (!fp)
            break;

        char buffer[BUFSIZE];
        size_t sz = fread(buffer, sizeof(char), BUFSIZE, fp);
        if (sz == 0)
            break;
        char* p = buffer;
        while(*p)
            p++;
        size_t addrlen = p - buffer;
        if (sz != addrlen + sizeof(pid_t) + sizeof(long) + 1)
            break;

        /* skip \0 */
        p++;
        pid_t *pid = (pid_t*) p;
        result.pid = *pid;
        result.address = strdup(buffer);
    } while(0);

    if (fp)
        pclose(fp);

    return result;
}

int DBusKill(DBusDaemonProperty* prop)
{
    if (prop->pid)
        return kill(prop->pid, SIGTERM);
    else
        return 0;
}

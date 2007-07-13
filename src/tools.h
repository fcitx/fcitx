/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
#ifndef _TOOLS_H
#define _TOOLS_H

#include <stdio.h>
#include "ime.h"

void            LoadConfig (Bool bMode);
void            SaveConfig (void);
void            LoadProfile (void);
void            SaveProfile (void);
void            SetHotKey (char *strKey, HOTKEYS * hotkey);
int             CalculateRecordNumber (FILE * fpDict);
void            SetSwitchKey (char *str);
void            SetTriggerKeys (char *str);
Bool            CheckHZCharset (char *strHZ);
Bool   		MyStrcmp(char *str1, char *str2);

/*
#if defined(DARWIN)
int             ReverseInt (unsigned int pc_int);
#endif
*/
#endif

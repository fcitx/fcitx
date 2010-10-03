/***************************************************************************
 *   Copyright (C) 2008~2010 by Zealot.Hoi                                 *
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

#ifdef _ENABLE_DBUS

#include <dbus/dbus.h>
#include "ui/InputWindow.h"
#include "core/ime.h"
#include "core/xim.h"

extern IM *im;

extern INT8 iState;
extern Bool bVK;
extern Bool bUseAA;
extern Bool bUseMatchingKey;
extern Bool bUsePinyin;
extern Bool bUseBold;

typedef struct Property_ {
    char *key;
    char *label;
    char *icon;
    char *tip;
} Property;

extern DBusConnection *conn;

Bool InitDBus();

void KIMExecDialog(char *prop);
void KIMExecMenu(char *props[],int n);
void KIMRegisterProperties(char *props[],int n);
void KIMUpdateProperty(char *prop);
void KIMRemoveProperty(char *prop);
void KIMEnable(Bool toEnable);
void KIMShowAux(Bool toShow);
void KIMShowPreedit(Bool toShow);
void KIMShowLookupTable(Bool toShow);
void KIMUpdateLookupTable(char *labels[], int nLabel, char *texts[], int nText, Bool has_prev, Bool has_next);
void KIMUpdatePreeditCaret(int position);
void KIMUpdatePreeditText(char *text);
void KIMUpdateAux(char *text);
void KIMUpdateSpotLocation(int x,int y);
void KIMUpdateScreen(int id);

void updateMessages();

void registerProperties();
void updateProperty(Property *prop);
void triggerProperty(char *propKey);

char* property2string(Property *prop);

void updatePropertyByConnectID(CARD16 connect_id);

void DBusLoop(void *val);
void MyDBusEventHandler();

#endif 

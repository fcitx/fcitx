#ifndef _TOOLS_H
#define _TOOLS_H

#include <stdio.h>
#include "ime.h"

#define MAX_HZ_SAVED	10

void            LoadConfig (Bool bMode);
void            SaveConfig (void);
void            LoadProfile (void);
void            SaveProfile (void);
void            SetHotKey (char *strKey, HOTKEYS * hotkey);
int             CalculateRecordNumber (FILE * fpDict);
void            SetSwitchKey (char *str);

#endif

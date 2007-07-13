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
/*
#if defined(DARWIN)
int             ReverseInt (unsigned int pc_int);
#endif
*/
#endif

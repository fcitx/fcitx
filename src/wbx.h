#ifndef _WBX_H
#define _WBX_H

#include <X11/Xlib.h>
#include "ime.h"

#define WUBI_DICT_FILENAME	"wbx.mb"
#define WUBI_FH_FILENAME	"wbfh.mb"

#define WB_CODE_LENGTH		4
#define WB_PHRASE_MAX_LENGTH	10

#define WB_FH_MAX_LENGTH	10

typedef struct _WBRECORD {
    char            strCode[WB_CODE_LENGTH + 1];
    char            *strHZ;
    //char            strHZ[WB_PHRASE_MAX_LENGTH * 2 + 1];
    struct _WBRECORD *next;
    struct _WBRECORD *prev;
} WBRECORD;

typedef struct _WBFH {
    char            strWBFH[WB_FH_MAX_LENGTH*2 + 1];
} WBFH;

Bool            LoadWBDict (void);
void            SaveWubiDict (void);
INPUT_RETURN_VALUE DoWBInput (int iKey);
INPUT_RETURN_VALUE WBGetCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE WBGetLegendCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE WBGetFHCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE WBGetPinyinCandWords (SEARCH_MODE mode);
void            ResetWBStatus (void);
char           *WBGetLegendCandWord (int iIndex);
char           *WBGetFHCandWord (int iIndex);
Bool            HasZ (void);
int             CompareWBCode (char *strUser, char *strDict);
int             WBFindFirstMatchCode (void);
WBRECORD       *FindWuBiCode (char *strHZ, Bool bMode);
void            AdjustWBOrderByIndex (int iIndex);
void            DelWBPhraseByIndex (int iIndex);
void            InsertWBPhrase (char *strCode, char *strHZ);
char           *WBGetCandWord (int iIndex);
void            WBCreateNewPhrase (void);
void            CreatePhraseWBCode (void);

Bool		WBPhraseTips(char *strPhrase);

#endif

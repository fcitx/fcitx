#ifndef _ERBI_H
#define _ERBI_H

#include <X11/Xlib.h>
#include "ime.h"

#define ERBI_DICT_FILENAME	"erbi.mb"
#define ERBI_FH_FILENAME	"ebfh.mb"

#define EB_CODE_LENGTH		4
#define EB_PHRASE_MAX_LENGTH	10

#define EB_FH_MAX_LENGTH	10

typedef struct _EBRECORD {
    char            strCode[EB_CODE_LENGTH + 1];
    char            *strHZ;
    //char            strHZ[EB_PHRASE_MAX_LENGTH * 2 + 1];
    struct _EBRECORD *next;
    struct _EBRECORD *prev;
} EBRECORD;

typedef struct _EBFH {
    char            strEBFH[EB_FH_MAX_LENGTH*2 + 1];
} EBFH;

Bool            LoadEBDict (void);
void            SaveErbiDict (void);
INPUT_RETURN_VALUE DoEBInput (int iKey);
INPUT_RETURN_VALUE EBGetCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE EBGetLegendCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE EBGetFHCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE EBGetPinyinCandWords (SEARCH_MODE mode);
void            ResetEBStatus (void);
char           *EBGetLegendCandWord (int iIndex);
char           *EBGetFHCandWord (int iIndex);
Bool            EBHasZ (void);
int             CompareEBCode (char *strUser, char *strDict);
int             EBFindFirstMatchCode (void);
EBRECORD       *FindErBiCode (char *strHZ, Bool bMode);
void            AdjustEBOrderByIndex (int iIndex);
void            DelEBPhraseByIndex (int iIndex);
void            InsertEBPhrase (char *strCode, char *strHZ);
char           *EBGetCandWord (int iIndex);
void            EBCreateNewPhrase (void);
void            CreatePhraseEBCode (void);

Bool		EBPhraseTips(char *strPhrase);

#endif

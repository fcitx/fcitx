#ifndef _PUNC_H
#define _PUNC_H

#include <X11/Xlib.h>

#define PUNC_DICT_FILENAME	"punc.mb"
#define MAX_PUNC_NO		2
#define MAX_PUNC_LENGTH		4

typedef struct TCHNPUNC {
    int            ASCII;
    char            strChnPunc[MAX_PUNC_NO][MAX_PUNC_LENGTH + 1];
    unsigned        iCount:2;
    unsigned        iWhich:2;
} ChnPunc;

Bool            LoadPuncDict (void);
int             IsPunc (int iKey);

#endif

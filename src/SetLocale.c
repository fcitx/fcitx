#include "SetLocale.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlocale.h>

char            strUserLocale[20];

extern Bool     bIsUtf8;
extern char    *strDefaultLocale;
extern Bool     bUseGBK;

void GetLocale (void)
{
    if (strUserLocale[0] != '\0')
	strDefaultLocale = strUserLocale;
    else {
	strDefaultLocale = (char *) getenv ("LC_CTYPE");
	if (!strDefaultLocale)
	    strDefaultLocale = (char *) getenv ("LANG");
    }

    if (!strcasecmp (strDefaultLocale, "zh_CN.UTF8") || !strcasecmp (strDefaultLocale, "zh_CN.UTF-8"))
	bIsUtf8 = True;
}

void SetLocale (void)
{
//    GetLocale ();
    setlocale (LC_CTYPE, strDefaultLocale);
}

Bool CheckLocale (char *strHZ)
{
    if (!bUseGBK) {
	//GB2312的汉字编码规则为：第一个字节的值在0xA1到0xFE之间(实际为0xF7)，第二个字节的值在0xA1到0xFE之间
	//由于查到的资料说法不一，懒得核实，就这样吧
	int             i;

	for (i = 0; i < strlen (strHZ); i++) {
	    if ((unsigned char) strHZ[i] < (unsigned char) 0xA1 || (unsigned char) strHZ[i] > (unsigned char) 0xF7 || (unsigned char) strHZ[i + 1] < (unsigned char) 0xA1 || (unsigned char) strHZ[i + 1] > (unsigned char) 0xFE)
		return False;
	    i++;
	}
    }

    return True;
}

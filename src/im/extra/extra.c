/***************************************************************************
 *   Copyright (C) 2010~2010 by dgod                                       *
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

#include "core/addon.h"
#include "im/extra/extra.h"
#include "ui/InputWindow.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"

#include <dlfcn.h>
#include <limits.h>
#include <iconv.h>

extern IM             *im;

extern Display *dpy;

static char CandTableEngine[10][MAX_CAND_LEN+1];
static char CodeTipsEngine[10][MAX_TIPS_LEN+1];
static char StringGetEngine[MAX_USER_INPUT+1];

extern INT8 iIMCount;
extern INT8 iInCap;

extern int      iCandWordCount;
extern int      iCandPageCount;
extern int	iCurrentCandPage;
extern char     strStringGet[];
extern char     strCodeInput[];
extern int      iCodeInputCount;
extern Bool	bCursorAuto;

#define GetCurrentEIM() ((im[gs.iIMIndex].addonInfo)?im[gs.iIMIndex].addonInfo->im.eim:NULL)

void UnloadExtraIM(int index)
{
    FcitxAddon* addon = im[index].addonInfo;
    if (!addon)
        return;
    EXTRA_IM *eim = GetCurrentEIM();
    if (!eim)
        return;
    if (eim->Destroy)
        eim->Destroy();
    if (addon->im.handle)
        dlclose(addon->im.handle);
    addon->im.index = 0;
    addon->im.handle = NULL;
    addon->im.eim = NULL;
}

static void ExtraReset(void)
{
	EXTRA_IM *eim = GetCurrentEIM();
	inputWindow.bShowCursor=False;
	bCursorAuto=False;
	if(!eim) return;
	if(eim->CandWordMax) eim->CandWordMax=fc.iMaxCandWord;
	if(eim->Reset) eim->Reset();
	eim->StringGet[0]=0;
	eim->CodeInput[0]=0;
	eim->CaretPos=-1;
	eim->CandWordMax=fc.iMaxCandWord;
	eim->Reset();
}

static void DisplayEIM(EXTRA_IM *im)
{
	int i;
	char strTemp[3];

	if ( fc.bPointAfterNumber )
	{
		strTemp[1] = '.';
		strTemp[2] = '\0';
	}
	else strTemp[1]='\0';

	SetMessageCount(&messageDown, 0);
	for (i = 0; i < im->CandWordCount; i++)
	{
		strTemp[0] = i + 1 + '0';
		if (i == 9) strTemp[0] = '0';
        AddMessageAtLast(&messageDown, MSG_INDEX, strTemp);

        AddMessageAtLast(&messageDown, (i!=im->SelectIndex)? MSG_OTHER:MSG_FIRSTCAND, im->CandTable[i]);
		if(im->CodeTips && im->CodeTips[i] && im->CodeTips[i][0])
            AddMessageAtLast(&messageDown, MSG_CODE, im->CodeTips[i]);
		if (i != 9)
            MessageConcatLast(&messageDown, " ");
	}

	SetMessageCount(&messageUp, 0);
	if(im->StringGet[0] || im->CodeInput[0])
	{
        AddMessageAtLast(&messageUp, MSG_INPUT, "%s%s", im->StringGet, im->CodeInput);

		inputWindow.bShowCursor=True;
		iCodeInputCount=strlen(im->CodeInput);
		if(im->CaretPos!=-1)
			iCursorPos=im->CaretPos;
		else
			iCursorPos=strlen(im->StringGet) + strlen(im->CodeInput);
		bCursorAuto=True;
	}

	iCandWordCount=im->CandWordCount;
	iCandPageCount=im->CandPageCount;
	iCurrentCandPage=im->CurCandPage;
	if(iCandPageCount) iCandPageCount--;
}

static INPUT_RETURN_VALUE ExtraDoInput(unsigned int sym, unsigned int state, int count)
{
	EXTRA_IM *eim = GetCurrentEIM();
	INPUT_RETURN_VALUE ret=IRV_DO_NOTHING;
	if(!eim) return IRV_TO_PROCESS;
	if(eim->DoInput)
		ret=eim->DoInput(sym, state, count);

	if(ret==IRV_GET_CANDWORDS||ret==IRV_GET_CANDWORDS_NEXT)
	{
		strcpy(strStringGet,eim->StringGet);
		eim->StringGet[0]=0;
		if(ret==IRV_GET_CANDWORDS_NEXT)
		{
			DisplayEIM(eim);
		}
		else
		{
			SetMessageCount(&messageDown, 0);
			SetMessageCount(&messageUp, 0);
		}
	}
	else if(ret==IRV_DO_NOTHING && (eim->CandWordCount ||
		eim->StringGet[0] || eim->CodeInput[0]))
	{
		DisplayEIM(eim);
		ret=IRV_DISPLAY_CANDWORDS;
	}
	else if(ret==IRV_DISPLAY_CANDWORDS)
	{
		DisplayEIM(eim);
	}
	else if(ret==IRV_ENG)
	{
		iCodeInputCount=strlen(strCodeInput);
		iInCap=3;
		ret=IRV_DONOT_PROCESS;
	}

	return ret;
}

static INPUT_RETURN_VALUE ExtraGetCandWords(SEARCH_MODE sm)
{
	EXTRA_IM *eim = GetCurrentEIM();
	INPUT_RETURN_VALUE ret=IRV_DO_NOTHING;
	if(!eim) return IRV_TO_PROCESS;
	if(eim->GetCandWords)
		ret=eim->GetCandWords(sm);
	if(ret==IRV_DISPLAY_CANDWORDS)
		DisplayEIM(eim);
	return ret;
}

static char *ExtraGetCandWord(int index)
{
	EXTRA_IM *eim = GetCurrentEIM();
	if(!eim) return 0;
	if(eim->GetCandWord)
	{
		char *ret = eim->GetCandWord(index);
        if (ret)
        {
            SetMessageCount(&messageDown, 0);
            SetMessageCount(&messageUp, 0);
            return ret;
        }
        else
            DisplayEIM(eim);
	}
	return 0;
}

/* the result is slow, why? */
char *GetClipboardString(Display *disp)
{
	Atom sel;
	Window w;
	int ret;
	Atom target;
	Atom type;
	int fmt;
	unsigned long n,after;
	unsigned char *pret=0;
	static char result[1024];

	sel = XInternAtom(disp, "PRIMARY", 0);
	target = XInternAtom(disp,"UTF8_STRING",0);
	w=XGetSelectionOwner(disp,sel);
	if(w==None)
	{
		return NULL;
	}
	XConvertSelection(disp,sel,target,sel,w,CurrentTime);
	ret=XGetWindowProperty(disp,w,sel,0,1023,False,target,&type,&fmt,&n,&after,&pret);
	if(ret!=Success || !pret || fmt!=8)
	{
		return NULL;
	}
	memcpy(result,pret,n);
	XFree(pret);
	result[n]=0;
	return result;
}

int InitExtraIM(EXTRA_IM *eim,char *arg)
{
	eim->CodeInput=strCodeInput;
	eim->StringGet=StringGetEngine;
	eim->CandTable=CandTableEngine;
	eim->CodeTips=CodeTipsEngine;
	eim->CandWordMax=fc.iMaxCandWord;
	eim->CaretPos=-1;
    eim->fc = (void*)&fc;
    eim->profile = (void*)&fcitxProfile;

	if(eim->Init(arg))
	{
		FcitxLog(ERROR, _("ExtraIM: init fail"));
		return -1;
	}

	return 0;
}

void LoadExtraIM()
{
    FcitxAddon *addon;
    for ( addon = (FcitxAddon *) utarray_front(addons);
          addon != NULL;
          addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->category == AC_INPUTMETHOD)
        {
            char *modulePath;
            switch (addon->type)
            {
                case AT_SHAREDLIBRARY:
                    {
                        FILE *fp = GetLibFile(addon->module, "r", &modulePath);
                        void *handle;
                        EXTRA_IM* eim;
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("ExtraIM: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        eim=dlsym(handle,"EIM");
                        if(!eim || !eim->Init)
                        {
                            FcitxLog(ERROR, _("ExtraIM: bad im"));
                            dlclose(handle);
                            break;
                        }
                        if(InitExtraIM(eim,addon->module))
                        {
                            dlclose(handle);
                            return;
                        }
                        RegisterNewIM(eim->Name, eim->IconName,ExtraReset,ExtraDoInput,ExtraGetCandWords,ExtraGetCandWord,NULL,NULL,NULL,NULL, addon);
                        addon->im.handle = handle;
                        addon->im.index = iIMCount - 1;
                        addon->im.eim = eim;
                    }
                    break;
                default:
                    break;
            }
            free(modulePath);
        }
    }
}

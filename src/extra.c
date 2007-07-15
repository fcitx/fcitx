/*!
 * \file extra.c
 * \author dgod
 */

#include "extra.h"
#include "InputWindow.h"

#include <dlfcn.h>
#include <limits.h>

static EXTRA_IM *EIMS[EIM_MAX];
static void *EIM_handle[EIM_MAX];
static char EIM_file[EIM_MAX][PATH_MAX];
static INT8 EIM_index[EIM_MAX];

extern INT8 iIMCount;
extern INT8 iIMIndex;
extern INT8 iInCap;

extern MESSAGE  messageUp[];
extern uint     uMessageUp;
extern MESSAGE  messageDown[];
extern uint     uMessageDown;
extern Bool	bPointAfterNumber;

extern int      iMaxCandWord;
extern int      iCandWordCount;
extern int      iCandPageCount;
extern int	iCurrentCandPage;
extern char     strStringGet[];
extern char     strCodeInput[];
extern int      iCodeInputCount;
extern Bool     bShowCursor;

static void ResetAll(void)
{
	int i;
	for(i=0;i<EIM_MAX;i++)
	{
		if(EIMS[i])
		{
			if(EIMS[i]->Destroy)
				EIMS[i]->Destroy();
			EIMS[i]=NULL;
		}
		if(EIM_handle[i])
		{
			dlclose(EIM_handle[i]);
			EIM_handle[i]=NULL;
		}
		if(EIM_file[i][0])
		{
			EIM_file[i][0]=0;
		}
		EIM_index[i]=0;
	}
}

static void ExtraReset(void)
{
	int i;
	EXTRA_IM *eim=NULL;
	for(i=0;i<EIM_MAX;i++)
	{
		if(EIM_index[i]==iIMIndex)
		{
			eim=EIMS[i];
			break;
		}
	}
	bShowCursor=False;
	if(!eim) return;
	if(eim->CandWordMax) *eim->CandWordMax=iMaxCandWord;
	if(eim->Reset) eim->Reset();
}

static void DisplayEIM(EXTRA_IM *im)
{
	int i;
	char strTemp[3];

	if ( bPointAfterNumber )
	{
		strTemp[1] = '.';
		strTemp[2] = '\0';
	}
	else strTemp[1]='\0';

	uMessageDown = 0;
	for (i = 0; i < *im->CandWordCount; i++)
	{
		strTemp[0] = i + 1 + '0';
		if (i == 9) strTemp[0] = '0';
		strcpy(messageDown[uMessageDown].strMsg, strTemp);
		messageDown[uMessageDown++].type = MSG_INDEX;	
		
		strcpy(messageDown[uMessageDown].strMsg, im->CandTable[i]);
		messageDown[uMessageDown].type = (i!=*im->SelectIndex)? MSG_OTHER:MSG_FIRSTCAND;
		if(im->CodeTips && im->CodeTips[i] && im->CodeTips[i][0])
		{
			uMessageDown++;
			strcpy(messageDown[uMessageDown].strMsg,im->CodeTips[i]);
			messageDown[uMessageDown].type=MSG_CODE;
		}
		if (i != 9)
			strcat (messageDown[uMessageDown].strMsg, " ");
		uMessageDown++;

	}    

	uMessageUp=0;   
	if(im->CodeInput[0])
	{ 
		uMessageUp = 1;	
		strcpy (messageUp[0].strMsg, im->CodeInput);
		messageUp[0].type = MSG_INPUT;

		bShowCursor=True;
		iCodeInputCount=strlen(im->CodeInput);
	}

	iCandWordCount=*im->CandWordCount;
	iCandPageCount=*im->CandPageCount;
	iCurrentCandPage=*im->CurCandPage;
	if(iCandPageCount) iCandPageCount--;
}


static INPUT_RETURN_VALUE ExtraDoInput(int key)
{
	int i;
	EXTRA_IM *eim=NULL;
	INPUT_RETURN_VALUE ret=IRV_DO_NOTHING;
	for(i=0;i<EIM_MAX;i++)
	{
		if(EIM_index[i]==iIMIndex)
		{
			eim=EIMS[i];
			break;
		}
	}
	if(!eim) return IRV_TO_PROCESS;
	if(key==ENTER)
	{
		strcpy(strCodeInput,eim->CodeInput);
		iCodeInputCount=strlen(strCodeInput);
	}
	if(key==' ')
	{
		if(!eim->CodeInput[0])
			return IRV_TO_PROCESS;
		strcpy(strStringGet,eim->GetCandWord(*eim->SelectIndex));
		uMessageDown=0;
		uMessageUp=0;
		return IRV_GET_CANDWORDS;
	}
	if(key>='0' && key<= '9')
	{
		int index;
		if(key=='0') index=9;
		else index=key-'0'-1;
		if(index>=*eim->CandWordCount)
		{
			if(eim->CodeInput[0])
				return IRV_DO_NOTHING;
			else return IRV_TO_PROCESS;
		}
		strcpy(strStringGet,eim->GetCandWord(index));
		uMessageDown=0;
		uMessageUp=0;
		return IRV_GET_CANDWORDS;
	}
	if(key == (XK_BackSpace & 0x00FF))
		key='\b';
	if(eim->DoInput)
		ret=eim->DoInput(key);
	if(ret==IRV_GET_CANDWORDS||ret==IRV_GET_CANDWORDS_NEXT)
	{
		strcpy(strStringGet,eim->StringGet);
		if(ret==IRV_GET_CANDWORDS_NEXT)
		{
			DisplayEIM(eim);
		}
		else
		{
			uMessageDown=0;
			uMessageUp=0;
		}
	}
	else if(ret==IRV_DISPLAY_CANDWORDS)
	{
		DisplayEIM(eim);
	}
	else if(ret==IRV_ENG)
	{
		strcpy(strCodeInput,eim->CodeInput);
		iCodeInputCount=strlen(strCodeInput);
		iInCap=3;
		ret=IRV_DONOT_PROCESS;
	}

	return ret;
}

static INPUT_RETURN_VALUE ExtraGetCandWords(SEARCH_MODE sm)
{
	int i;
	EXTRA_IM *eim=NULL;
	INPUT_RETURN_VALUE ret=IRV_DO_NOTHING;
	for(i=0;i<EIM_MAX;i++)
	{
		if(EIM_index[i]==iIMIndex)
		{
			eim=EIMS[i];
			break;
		}
	}
	if(!eim) return IRV_TO_PROCESS;
	if(eim->GetCandWords)
		ret=eim->GetCandWords(sm);
	if(ret==IRV_DISPLAY_CANDWORDS)
		DisplayEIM(eim);
	return ret;
}

static char *ExtraGetCandWord(int index)
{
	int i;
	EXTRA_IM *eim=NULL;
	for(i=0;i<EIM_MAX;i++)
	{
		if(EIM_index[i]==iIMIndex)
		{
			eim=EIMS[i];
			break;
		}
	}
	if(!eim) return 0;
	if(eim->GetCandWord)
	{
		uMessageDown=0;
		uMessageUp=0;
		return eim->GetCandWord(index);
	}
	return 0;
}

void LoadExtraIM(char *fn)
{
	void *handle;
	int i;
	EXTRA_IM *eim;
	char path[PATH_MAX];

	if(strchr(fn,'/')) strcpy(path,fn);
	else sprintf(path,PKGDATADIR"/data/%s",fn);

	for(i=0;i<EIM_MAX;i++)
	{
		if(EIM_file[i][0] && !strcmp(EIM_file[i],path))
		{
			ResetAll();
			break;
		}
	}
	for(i=0;i<EIM_MAX;i++)
	{
		if(!EIMS[i])
		{
			break;
		}
	}
	if(i==EIM_MAX)
	{
		printf("eim: no space\n");
		return;
	}
	handle=dlopen(path,RTLD_LAZY);
	if(!handle)
	{
		printf("eim: open fail\n");
		return;
	}
	eim=dlsym(handle,"EIM");
	if(!eim || !eim->Init || eim->Init())
	{
		printf("eim: init fail\n");
		dlclose(handle);
		return;
	}
	RegisterNewIM(eim->Name,ExtraReset,ExtraDoInput,ExtraGetCandWords,ExtraGetCandWord,NULL,NULL,NULL,NULL);
	EIMS[i]=eim;
	EIM_handle[i]=handle;
	strcpy(EIM_file[i],fn);
	EIM_index[i]=iIMCount-1;
	if(eim->CandWordMax) *eim->CandWordMax=iMaxCandWord; 
}


/* Created by Anjuta version 1.1.97 */
/*	This file will not be overwritten */

/*Program closes with a mouse click or keypress */

#include <langinfo.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "version.h"

#include "main.h"
#include "MyErrorsHandlers.h"
#include "tools.h"
#include "ui.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "ime.h"
#include "wbx.h"
#include "erbi.h"
#include "punc.h"
#include "py.h"
#include "sp.h"

extern Display *dpy;
extern Window   inputWindow;
extern Window   mainWindow;

extern INT8     iIMIndex;
extern Bool     bBackground;
extern Bool     bIsUtf8;
extern HIDE_MAINWINDOW hideMainWindow;

int main (int argc, char *argv[])
{
    XEvent          event;
    char           *imname = NULL;
    int             i;
    
    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-name"))
	    imname = argv[++i];
//	else if (!strcmp (argv[i], "-kl"))
//	    filter_mask = (KeyPressMask | KeyReleaseMask);
	else if (!strcmp (argv[i], "-nb"))
	    bBackground = False;
	else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-?")) {
	    Usage();
	    return 0;
	}
	else if (!strcmp (argv[i], "-v" ) ) {
	    Version();
	    return 0;
	}
    }

    SetMyExceptionHandler ();

    setlocale (LC_ALL, "");
    bIsUtf8 = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0);
    
    LoadConfig (True);    
    InitX ();    
    CreateFont ();
    CalculateInputWindowHeight();
    LoadProfile ();

    if (!LoadPuncDict ())
	fprintf (stderr, "无法打开中文标点文件，将无法输入中文标点！\n");

    /* 加入输入法 */
    RegisterNewIM(NAME_OF_PINYIN,ResetPYStatus,DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL,PYInit);
    RegisterNewIM(NAME_OF_SHUANGPIN,ResetPYStatus,DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL,SPInit);
    RegisterNewIM(NAME_OF_WUBI,ResetWBStatus,DoWBInput, WBGetCandWords, WBGetCandWord, WBGetLegendCandWord, WBPhraseTips,PYInit);
    RegisterNewIM(NAME_OF_ERBI,ResetEBStatus,DoEBInput, EBGetCandWords, EBGetCandWord, EBGetLegendCandWord, EBPhraseTips,PYInit);
    
    CreateMainWindow ();
    InitGC (mainWindow);
    if (!InitXIM (imname, mainWindow))
	exit (4);

    CreateInputWindow ();
    ResetInput ();
    
    SwitchIME (iIMIndex);    
    DisplayMainWindow ();

    //以后台方式运行
    if (bBackground) {
	pid_t           id;

	id = fork ();
	if (id == -1) {
	    fprintf (stderr,"无法以后台方式运行 FCITX！\n");
	    exit (1);
	}
	else if (id > 0)
	    exit (0);
    }

    for (;;) {
	XNextEvent (dpy, &event);

	if (XFilterEvent (&event, None) == True)
	    continue;
	MyXEventHandler (inputWindow, &event);
    }

    return 0;
}

void Usage()
{
	printf("fcitx usage:\n -name imename: \t specify the imename\n -nb :\t\t\t run as foreground \n -v:\t\t\t display the version information and exit.\n -h:\t\t\t display this help page and exit\n");
}

void Version()
{
	printf("fcitx version: %s\n", FCITX_VERSION);
}

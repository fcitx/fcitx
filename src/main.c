/* Created by Anjuta version 1.1.97 */
/*	This file will not be overwritten */

/*Program closes with a mouse click or keypress */

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
#include "SetLocale.h"
#include "ime.h"
#include "wbx.h"
#include "punc.h"
#include "py.h"

extern Display *dpy;
extern Window   inputWindow;
extern Window   mainWindow;

extern          INPUT_RETURN_VALUE (*DoInput) (int);
extern char    *(*GetCandWord) (int);
extern          INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE);
char           *(*GetLegendCandWord) (int);

extern IME      imeIndex;
extern Bool     bBackground;
extern HIDE_MAINWINDOW hideMainWindow;
extern Bool     bRunLocal;

int main (int argc, char *argv[])
{
    XEvent          event;
    char           *imname = NULL;
    int             i;

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-name"))
	    imname = argv[++i];
/*	else if (!strcmp (argv[i], "-kl"))
	    filter_mask = (KeyPressMask | KeyReleaseMask);*/
	else if (!strcmp (argv[i], "-local"))
	    bRunLocal = True;
	else if (!strcmp (argv[i], "-nb"))
	    bBackground = False;
    }

    SetMyExceptionHandler ();

    InitX ();

    LoadProfile ();
    LoadConfig (True);
    GetLocale ();
    CreateFont ();
    SetLocale ();    

    if (!LoadPuncDict ())
	fprintf (stderr, "无法打开中文标点文件，将无法输入中文标点！\n");

    CreateMainWindow ();
    InitGC (mainWindow);
    if (!InitXIM (imname, mainWindow))
	exit (4);

    CreateInputWindow ();
    ResetInput ();
    SwitchIME (imeIndex);
    DisplayMainWindow ();

    //以后台方式运行
    if (bBackground) {
	pid_t           id;

	id = fork ();
	if (id == -1) {
	    fprintf (stderr, "无法以后台方式运行 FCITX！\n");
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

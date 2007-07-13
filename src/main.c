/* 
 * 小企鹅中文输入法(Free Chinese Input Toys for X, FCITX)
 * 由Yuking(yuking_net@sohu.com)编写
 * 协议: GPL
 *
 * FCITX( A Chinese XIM Input Method) by Yuking (yuking_net@sohu.com)
 * Licence: GPL
 * 
 */

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
#include "table.h"
#include "punc.h"
#include "py.h"
#include "sp.h"

extern Display *dpy;
extern Window   inputWindow;
extern Window   mainWindow;

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
	else if (!strcmp (argv[i], "-nb"))
	    bBackground = False;
	else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-?")) {
	    Usage ();
	    return 0;
	}
	else if (!strcmp (argv[i], "-v")) {
	    Version ();
	    return 0;
	}
    }

    setlocale(LC_CTYPE,"");
    bIsUtf8 = (strcmp (nl_langinfo (CODESET), "UTF-8") == 0);

    LoadConfig (True);

    if (!InitX () )
    	exit(1);

    CreateFont ();
    CalculateInputWindowHeight ();
    LoadProfile ();

    if (!LoadPuncDict ())
	printf ("无法打开中文标点文件，将无法输入中文标点！\n");

    CreateMainWindow ();
    InitGC (mainWindow);
    CreateInputWindow ();

    SetIM ();

    if (!InitXIM (imname, inputWindow))
	exit (4);

    DisplayMainWindow ();

    //以后台方式运行
    if (bBackground) {
	pid_t           id;

	id = fork ();
	if (id == -1) {
	    printf ("无法以后台方式运行 FCITX！\n");
	    exit (1);
	}
	else if (id > 0)
	    exit (0);
    }

    SetMyExceptionHandler ();
    XFlush(dpy);
    for (;;) {
    	XNextEvent (dpy, &event);

	if (XFilterEvent (&event, None) == True)
	    continue;

	MyXEventHandler (&event);
    }

    return 0;
}

void Usage ()
{
    printf ("fcitx usage:\n -name imename: \t specify the imename\n -nb :\t\t\t run as foreground \n -v:\t\t\t display the version information and exit.\n -h:\t\t\t display this help page and exit\n");
}

void Version ()
{
    printf ("fcitx version: %s\n", FCITX_VERSION);
}

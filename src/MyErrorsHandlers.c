#include "MyErrorsHandlers.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <wait.h>

#include "wbx.h"
#include "py.h"

XErrorHandler   oldXErrorHandler;

extern BYTE     iWBChanged;
extern BYTE     iNewPYPhraseCount;
extern BYTE     iOrderCount;
extern BYTE     iNewFreqCount;

void SetMyExceptionHandler (void)
{
    int             signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++)
	signal (signo, OnException);
}

void OnException (int signo)
{
    //   int             status;
    fprintf (stderr, "\nFCITX -- Get Signal No.: %d\n", signo);

    if (iWBChanged)
	SaveWubiDict ();
    if (iNewPYPhraseCount)
	SavePYUserPhrase ();
    if (iOrderCount)
	SavePYIndex ();
    if (iNewFreqCount)
	SavePYFreq ();

    if (signo != SIGCHLD && signo != SIGQUIT && signo != SIGWINCH) {
	fprintf (stderr, "FCITX -- Exit Signal No.: %d\n\n", signo);
	exit (0);
    }
}

void SetMyXErrorHandler (void)
{
    oldXErrorHandler = XSetErrorHandler (MyXErrorHandler);
}

int MyXErrorHandler (Display * dpy, XErrorEvent * event)
{
    char            str[1025];

    if (iWBChanged)
	SaveWubiDict ();
    if (iNewPYPhraseCount)
	SavePYUserPhrase ();
    if (iOrderCount)
	SavePYIndex ();
    if (iNewFreqCount)
	SavePYFreq ();

    XGetErrorText (dpy, event->error_code, str, 1024);
    fprintf (stderr, "fcitx: %s\n", str);

    if (event->error_code != 3 && event->error_code != BadMatch)	// xterm will generate 3
	oldXErrorHandler (dpy, event);

    return 0;
}

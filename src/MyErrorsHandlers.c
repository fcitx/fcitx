#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MyErrorsHandlers.h"

#include <stdio.h>
#include <signal.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_WAIT_H
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include "ime.h"
#include "tools.h"

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

XErrorHandler   oldXErrorHandler;

extern Bool     bLumaQQ;

void SetMyExceptionHandler (void)
{
    int             signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++)
	signal (signo, OnException);
}

void OnException (int signo)
{
    fprintf (stderr, "\nFCITX -- Get Signal No.: %d\n", signo);

    if (signo == SIGHUP) {
	SetIM ();
	LoadConfig (False);

	if (bLumaQQ)
	    ConnectIDResetReset ();

	return;
    }

    if (signo != SIGSEGV)	//出现SIGSEGV表明程序自己有问题，此时如果还执行保存操作，可能会损坏输入法文件
	SaveIM ();

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
    char            str[256];

    SaveIM ();

    XGetErrorText (dpy, event->error_code, str, 255);
    fprintf (stderr, "fcitx: %s\n", str);

    if (event->error_code != 3 && event->error_code != BadMatch)	// xterm will generate 3
	oldXErrorHandler (dpy, event);

    return 0;
}

#ifdef _ENABLE_TRAY

#include <stdio.h>

#include "TrayWindow.h"
#include "xim.h"
#include "inactive.xpm"
#include "active.xpm"

Bool tray_mapped = False;
tray_win_t tray;
Bool bUseTrayIcon = True;

extern Display *dpy;
extern int iScreen;
extern Atom Atoms[6];
extern CARD16 connect_id;

Bool ProcessTrayIcon(char** data, XImage** image, Pixmap* pixmapshape, Pixmap* pixmap);

Bool CreateTrayWindow() {
    tray_init(dpy, &tray);
    XVisualInfo* vi = tray_get_visual(dpy, &tray);
    Window dock = tray_get_dock(dpy);
    if (vi && vi->visual) {
        Window p = DefaultRootWindow (dpy);
        Colormap colormap = XCreateColormap(dpy, p, vi->visual, AllocNone);
        XSetWindowAttributes wsa;
        wsa.background_pixmap = 0;
        wsa.colormap = colormap;
        wsa.background_pixel = 0;
        wsa.border_pixel = 0;
        tray.window = XCreateWindow(dpy, p, -1, -1, 1, 1,
                0, vi->depth, InputOutput, vi->visual,
                CWBackPixmap|CWBackPixel|CWBorderPixel|CWColormap, &wsa);
    }
    else {
        tray.window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy), \
            0, 0, 1, 1, 0, \
            BlackPixel (dpy, DefaultScreen (dpy)), \
            WhitePixel (dpy, DefaultScreen (dpy)));
        XSetWindowBackgroundPixmap(dpy, tray.window, ParentRelative);
    }
    if (tray.window == (Window) NULL)
        return False;
    // GNOME, NET WM Specification
    tray_send_opcode(dpy, dock, CurrentTime, SYSTEM_TRAY_REQUEST_DOCK,
            0, 0);

    XSizeHints size_hints;
    size_hints.flags = PWinGravity | PBaseSize;
    size_hints.base_width = 22;
    size_hints.base_height = 22;
    XSetWMNormalHints(dpy, tray.window, &size_hints);

    if (!ProcessTrayIcon(inactive_xpm, &tray.icon[INACTIVE_ICON], &tray.icon_mask[INACTIVE_ICON],
				&tray.picon[INACTIVE_ICON])) {
        fprintf(stderr, "failed to get inactive icon image\n");
        return False;
    }

    if (!ProcessTrayIcon(active_xpm, &tray.icon[ACTIVE_ICON], &tray.icon_mask[ACTIVE_ICON],
				&tray.picon[ACTIVE_ICON])) {
        fprintf(stderr, "failed to get active icon image\n");
        return False;
    }

    /* GCs for copy and drawing on mask */
    XGCValues gv;
    gv.foreground = BlackPixel(dpy, DefaultScreen(dpy));
    gv.background = BlackPixel(dpy, DefaultScreen(dpy));
    tray.gc = XCreateGC(dpy, tray.window, GCBackground|GCForeground, &gv);

	XSetClipMask(dpy, tray.gc, tray.icon_mask[0]);

    XSelectInput (dpy, tray.window, ExposureMask | KeyPressMask | \
            ButtonPressMask | ButtonReleaseMask | StructureNotifyMask \
            | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);
    return True;
}

Bool ProcessTrayIcon(char** data, XImage** image, Pixmap* pixmapshape, Pixmap* pixmap)
{
	XImage* dummy = NULL;
	XpmAttributes attr;
	attr.valuemask = XpmColormap | XpmDepth | XpmCloseness;
	attr.colormap = DefaultColormap(dpy, iScreen);
	attr.depth = DefaultDepth(dpy, iScreen);
	attr.closeness = 40000;
	attr.exactColors = False;
	if (XpmCreateImageFromData(dpy, data, image, &dummy, &attr))
		return False;
	if (XpmCreatePixmapFromData(dpy, tray.window,data, pixmap,
				pixmapshape, &attr))
		return False;
	if (dummy)
		XDestroyImage(dummy);

	return True;
}

void DrawTrayWindow(int f_state, int x, int y, int w, int h) {
    if ( !bUseTrayIcon )
	return;
	
    if (!tray_mapped) {
        tray_mapped = True;
        if (!tray_find_dock(dpy, tray.window))
            return;
    }

    if (!tray_get_visual(dpy, &tray))
    {
        XClearArea (dpy, tray.window, x, y, w, h, False);
        XSetClipOrigin(dpy, tray.gc, x, y);
        XPutImage(dpy, tray.window, tray.gc, tray.icon[f_state],
                x, y, x, y, w, h);
    }
    else
    {
        Picture trayw, mask, icon;
        
        XWindowAttributes attrs;
        
        XGetWindowAttributes( dpy, tray.window , &attrs );
        XRenderPictFormat*  format = XRenderFindVisualFormat( dpy, attrs.visual );

        XRenderPictFormat *pformat = XRenderFindStandardFormat( dpy, PictStandardA1 );
        XRenderPictFormat *pformat2 = XRenderFindStandardFormat( dpy, PictStandardRGB24 );

        XRenderPictureAttributes pattrs;
        
        trayw = XRenderCreatePicture(dpy, tray.window,
                format, 0, NULL);
        
        mask = XRenderCreatePicture(dpy, tray.icon_mask[f_state],
                pformat, 0, NULL);

        pattrs.alpha_map = mask;
        icon = XRenderCreatePicture(dpy, tray.picon[f_state],
                pformat2, CPAlphaMap, &pattrs);



        XRenderComposite(dpy, PictOpOver, icon, None, trayw, 
                x,y , x,y,x,y,w,h);
        XRenderFreePicture(dpy,trayw);
        XRenderFreePicture(dpy,mask);
        XRenderFreePicture(dpy,icon);

    }
}

void tray_win_deinit(tray_win_t *f_tray) {
    ;
}

void tray_event_handler(XEvent* event)
{
    if (!bUseTrayIcon)
        return;
    switch (event->type) {
		case ClientMessage:
			if (event->xclient.message_type == Atoms[ATOM_MANAGER]
			 && event->xclient.data.l[1] == Atoms[ATOM_SELECTION])
			{
                tray_find_dock(dpy, tray.window);
			}
			break;

		case Expose:
			if (event->xexpose.window == tray.window) {
                if (ConnectIDGetState (connect_id) == IS_CHN)
                    DrawTrayWindow (ACTIVE_ICON, 0, 0, TRAY_ICON_HEIGHT, TRAY_ICON_WIDTH );
                else
                    DrawTrayWindow (INACTIVE_ICON, 0, 0, TRAY_ICON_HEIGHT, TRAY_ICON_WIDTH );
			}
			break;
		case DestroyNotify:
            tray_mapped = False;
			break;
	}
}

void tray_win_redraw(void) {
    tray_find_dock(dpy, tray.window);
}
#endif

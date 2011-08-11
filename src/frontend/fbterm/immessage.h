/*
 *   Copyright ? 2008-2009 dragchan <zgchan317@gmail.com>
 *   This file is part of FbTerm.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/**
 *
FbTerm implements a lightweight client-server input method architecture. Instead of processing user input and
drawing input method UI itself, FbTerm acts as a client and requests the input method server to do all these works.


Input Method Messages

On startup, FbTerm forks a child process and executes the input method server program in it. FbTerm and the input
method server will communicate each other with a unix socket pair created by FbTerm. When IM server startup, it sends
a Connect message to FbTerm, indicates that the server has got ready. FbTerm response a FbTermInfo message, tell
IM server things like current screen size, font size, rotation etc, help IM server draw it's UI. Of course,
IM server may ignore these hints.

When FbTerm exit, it sends a Disconnect to IM server, indicates it to exit.

 +------+           +------+            +------+           +------+
 |FbTerm|           |  IM  |            |FbTerm|           |  IM  |
 +--.---+           +---.--+            +--.---+           +---.--+
    |                   |                  |                   |
    |                   |<init>      <exit>|                   |
    |     Connect       |                  |    Disconnect     |
    |<------------------|                  |------------------>|
    |                   |                  |                   |
    |    FbTermInfo     |                  |                   |<exit>
    |------------------>|                  |                   |
    |                   |                  |                   |
    |                   |                  |                   |


If user presses the input method state switch shortcut (e.g. Ctrl + Space), FbTerm sends a Active message to
IM server, and a CursorPosition message with current cursor position. After receiving the Active message,
IM server may want to draw it's UI, e.g. a IM status bar, on frame buffer screen. In order to avoid FbTerm corrupting
it's UI, IM server first sends a SetWin message, tell FbTerm the screen areas occupied by its status bar, and waits FbTerm
to response a AckWin message. Now IM server can begin to draw the status bar.

While user pressing Ctrl + Space again, FbTerm sends a Deactive message to IM server. IM server should response a
SetWin message with a empty screen area to hide its status bar and ask FbTerm to redraw screen.

 +------+           +------+            +------+           +------+
 |FbTerm|           |  IM  |            |FbTerm|           |  IM  |
 +--.---+           +---.--+            +--.---+           +---.--+
    |                   |                  |                   |
    |                   |                  |                   |
    |      Active       |                  |     Deactive      |
    |------------------>|                  |------------------>|
    |                   |                  |                   |
    |  CursorPosition   |                  |      SetWin       |
    |------------------>|                  |<------------------|
    |                   |                  |                   |
    |      SetWin       |                  |      AckWin       |
    |<------------------|                  |------------------>|
    |                   |                  |                   |
    |      AckWin       |                  |                   |
    |------------------>|                  |                   |
    |                   |<draw UI>         |                   |
    |                   |                  |                   |


When IM state is on, FbTerm doesn't process any keyboard input except its shortcuts and Alt-Fn, it redirects them
to IM server with SendKey messages. After receiving SendKey message, IM server may draw it's preedit and candidate UI.
As described above, it should first send SetWin messages to tell FbTerm the areas occupied by preedit and candidate UI
and wait for responsed AckWin messages before drawing UI.

IM server sends the converted texts in a PutText message back to FbTerm, and FbTerm will write them to the running
program. In the common case, the running program will change cursor position, FbTerm notifies it to IM server with a
CursorPosition message. If IM server provides a XIM "over the spot" style UI, it may response SetWin messages to move
and redraw it's UI.

 +------+           +------+            +------+           +------+
 |FbTerm|           |  IM  |            |FbTerm|           |  IM  |
 +--.---+           +---.--+            +--.---+           +---.--+
    |                   |                  |                   |
    |                   |                  |                   |
    |      SendKey      |                  |      PutText      |
    |------------------>|                  |<------------------|
    |                   |                  |                   |
    |      SetWin       |                  |  CursorPosition   |
    |<------------------|                  |------------------>|
    |                   |                  |                   |
    |      AckWin       |                  |      SetWin       |
    |------------------>|                  |<------------------|
    |                   |                  |                   |
    |                   |<draw UI>         |      AckWin       |
    |                   |                  |------------------>|
    |                   |                  |                   |
    |                   |                  |                   |<draw UI>


When IM state is on, user presses shortcuts for switching to other sub-window or Alt-Fn for other virtual console, FbTerm
sends a HideUI message to IM server, then waits for a AckHideUI message responsed from IM server before switching. Maybe
IM server is busy with the previous SendKey message when HideUI message is arrived, it continues it's work, then processes
this HideUI message, sends back a AckHideUI message to FbTerm.

When switching back, FbTerm sends a ShowUI message to IM server, indicates IM server to redraw it's UI.

 +------+           +------+            +------+           +------+
 |FbTerm|           |  IM  |            |FbTerm|           |  IM  |
 +--.---+           +---.--+            +--.---+           +---.--+
    |                   |                  |                   |
    |                   |                  |                   |
    |       HideUI      |                  |      ShowUI       |
    |------------------>|                  |------------------>|
    |                   |                  |                   |
    |     AckHideUI     |                  |      SetWin       |
    |<------------------|                  |<------------------|
    |                   |                  |                   |
    |<do switching>     |                  |      AckWin       |
    |                   |                  |------------------>|
    |                   |                  |                   |
    |                   |                  |                   |<draw UI>
    |                   |                  |                   |
    |                   |                  |                   |
    |                   |                  |                   |
 */

#ifndef IM_MESSAGE_H
#define IM_MESSAGE_H

typedef enum {
    Connect = 0, Disconnect,
    Active, Deactive,
    SendKey, PutText,
    SetWin, AckWin,
    CursorPosition, FbTermInfo, TermMode,
    ShowUI, HideUI, AckHideUI,
    FillRect, DrawText,
    Ping, AckPing
} MessageType;

typedef struct {
    unsigned fontHeight, fontWidth;
    unsigned screenHeight, screenWidth;
} Info;

#define NR_IM_WINS 10

typedef struct {
    unsigned x, y;
    unsigned w, h;
} Rectangle;

typedef struct {
    unsigned short type;  ///< message's type, @see MessageType
    unsigned short len;  ///< message's length, including head and body

    union {
        char keys[0]; ///< included in message SendKey
        char texts[0]; ///< included in message PutText
        char raw; ///< included in message Connect
        unsigned winid; ///< included in message ShowUI
        Info info; ///< included in message FbTermInfo

        struct {
            Rectangle rect;
            unsigned winid;
        } win; ///< included in message SetWin

        struct {
            char crWithLf;
            char applicKeypad;
            char cursorEscO;
        } term; ///< included in message TermMode;

        struct {
            unsigned x, y;
        } cursor; ///< included in message CursorPosition

        struct {
            Rectangle rect;
            unsigned char color;
        } fillRect; ///< included in message FillRect

        struct {
            unsigned x, y;
            unsigned char fc, bc;
            char texts[0];
        } drawText; ///< included in message DrawText
    };
} Message;

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0; 

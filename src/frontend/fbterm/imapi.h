/*
 *   Copyright © 2008-2009 dragchan <zgchan317@gmail.com>
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
 * FbTerm provides a set of wrapped API to simply the IM server development. The API encapsulates input method message sending,
 * receiving and processing.
 */

#ifndef IM_API_H
#define IM_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "immessage.h"

typedef void (*ActiveFun)();
typedef void (*DeactiveFun)();

/// @param winid indicates which window should be redrawn, -1 means redraw all UI window. @see set_im_window()
typedef void (*ShowUIFun)(unsigned winid);
typedef void (*HideUIFun)();
typedef void (*SendKeyFun)(char *keys, unsigned len);
typedef void (*CursorPositionFun)(unsigned x, unsigned y);
typedef void (*FbTermInfoFun)(Info *info);
typedef void (*TermModeFun)(char crlf, char appkey, char curo);

typedef struct {
    ActiveFun active; ///< called when receiving a Active message
    DeactiveFun deactive; ///< called when receiving a Deactive message
    ShowUIFun show_ui; ///< called when receiving a ShowUI message
    HideUIFun hide_ui; ///< called when receiving a HideUI message
    SendKeyFun send_key; ///< called when receiving a SendKey message
    CursorPositionFun cursor_position; ///< called when receiving a CursorPosition message
    FbTermInfoFun fbterm_info; ///< called when receiving a FbTermInfo message
    TermModeFun term_mode; ///< called when receiving a TermMode message
} ImCallbacks;

/**
 * @brief register message call-back functions:
 * @param callbacks
 */
extern void register_im_callbacks(ImCallbacks callbacks);

/**
 * @brief send message Connect to FbTerm
 * @param mode keyboard input mode
 *
 * FbTerm provides two kinds of keyboard input modes, IM server can choose a favorite one with parameter mode of
 * connect_fbterm().
 *
 * If mode equals zero, IM server will receive characters translated by kernel with current keymap in SendKey messages.
 * There are some disadvantages in this mode, for example, shortcuts composed with only modifiers are impossible.
 * ( shortcuts composed with modifiers and normal keys can create a map if they are not mapped in current kernel keymap,
 * maybe we need add new messages like RequestKeyMap and AckKeyMap used by IM server to ask FbTerm to do this work. )

 * If mode is non-zero, FbTerm sets keyboard mode to K_MEDIUMRAW after Active Message, IM server will receive raw keycode
 * from kernel and have full control for keyboard, except FbTerm's shortcuts, e.g. Ctrl + Space.
 * Under this mode, IM server should translate keycodes to keysyms according current kernel keymap, change keyboard led
 * state and translate keysyms to characters if it want to send user input back to FbTerm. Because some translation of
 * keysyms to characters are not fixed (e.g. cursor movement keysym K_LEFT can be 'ESC [ D' or 'ESC O D'), message TermMode
 * sent by FbTerm is used to help IM server do this translation.
 *
 * keycode.c/keycode.h provides some functions to translate keycode to keysym and keysym to characters.
 */
extern void connect_fbterm(char mode);

/**
 * @brief get the file descriptor of the unix socket used to transfer IM messages.
 * @return file id of the socket connected to FbTerm
 *
 * Because check_im_message() blocks until a message arrives, IM server can use select/poll to monitor other file
 * descriptors among with the one from get_im_socket().
 */
extern int get_im_socket();

/**
 * @brief receive IM messages from FbTerm and dispatch them to functions registered with register_im_callbacks()
 * @return zero if DisconnectIM messages has been received, otherwise non-zero
 */
extern int check_im_message();

/**
 * @brief send message PutText to FbTerm
 * @param text  translated text from user keyboard input, must be encoded with utf8
 * @param len   text's length
 */
extern void put_im_text(const char *text, unsigned len);

/**
 * @brief send message SetWin to FbTerm and wait until AckWin has arrived
 * @param winid the window id, must less than NR_IM_WINS
 * @param rect  the rectangle of this window area
 *
 * every UI window (e.g. status bar, candidate table, etc) should have a unique win id.
 */
extern void set_im_window(unsigned winid, Rectangle rect);

/**
 * The colors of xterm's 256 color mode supported by FbTerm can be used in fill_rect() and draw_text().
 * ColorType defines the first 16 colors with standard linux console.
 * view 256colors.png for all colors.
 */
typedef enum { Black = 0, DarkRed, DarkGreen, DarkYellow, DarkBlue, DarkMagenta, DarkCyan, Gray,
    DarkGray, Red, Green, Yellow, Blue, Magenta, Cyan, White } ColorType;

/**
 * @brief send message FillRect to FbTerm
 * @param rect  the rectangle to be filled
 * @param color
 */
extern void fill_rect(Rectangle rect, unsigned char color);

/**
 * @breif send message DrawText to FbTerm
 * @param x     x axes of top left point
 * @param y     y axes of top left point
 * @param fc    foreground color
 * @param bc    background color
 * @param text  text to be drawn, must be encoding with utf8
 * @param len   text's length
 */
extern void draw_text(unsigned x, unsigned y, unsigned char fc, unsigned char bc, const char *text, unsigned len);

#ifdef __cplusplus
}
#endif

#endif


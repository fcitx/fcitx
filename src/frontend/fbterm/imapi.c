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

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "imapi.h"

#define OFFSET(TYPE, MEMBER) ((size_t)(&(((TYPE *)0)->MEMBER)))
#define MSG(a) ((Message *)(a))

static int imfd = -1;
static ImCallbacks cbs;
static char pending_msg_buf[10240];
static unsigned pending_msg_buf_len = 0;
static int im_active = 0;

static void wait_message(MessageType type);

void register_im_callbacks(ImCallbacks callbacks)
{
    cbs = callbacks;
}

int get_im_socket()
{
    static char init = 0;
    if (!init) {
        init = 1;

        char *val = getenv("FBTERM_IM_SOCKET");
        if (val) {
            char *tail;
            int fd = strtol(val, &tail, 0);
            if (!*tail) imfd = fd;
        }
    }

    return imfd;
}

void connect_fbterm(char raw)
{
    get_im_socket();
    if (imfd == -1) return;

    Message msg;
    msg.type = Connect;
    msg.len = sizeof(msg);
    msg.raw = (raw ? 1 : 0);
    int ret = write(imfd, (char *)&msg, sizeof(msg));
}

void put_im_text(const char *text, unsigned len)
{
    if (imfd == -1 || !im_active || !text || !len || (OFFSET(Message, texts) + len > UINT16_MAX)) return;

    char buf[OFFSET(Message, texts) + len];

    MSG(buf)->type = PutText;
    MSG(buf)->len = sizeof(buf);
    memcpy(MSG(buf)->texts, text, len);

    int ret = write(imfd, buf, MSG(buf)->len);
}

void set_im_window(unsigned id, Rectangle rect)
{
    if (imfd == -1 || !im_active || id >= NR_IM_WINS) return;

    Message msg;
    msg.type = SetWin;
    msg.len = sizeof(msg);
    msg.win.winid = id;
    msg.win.rect = rect;

    int ret = write(imfd, (char *)&msg, sizeof(msg));
    wait_message(AckWin);
}

void fill_rect(Rectangle rect, unsigned char color)
{
    Message msg;
    msg.type = FillRect;
    msg.len = sizeof(msg);

    msg.fillRect.rect = rect;
    msg.fillRect.color = color;

    int ret = write(imfd, (char *)&msg, sizeof(msg));
}

void draw_text(unsigned x, unsigned y, unsigned char fc, unsigned char bc, const char *text, unsigned len)
{
    if (!text || !len) return;

    char buf[OFFSET(Message, drawText.texts) + len];

    MSG(buf)->type = DrawText;
    MSG(buf)->len = sizeof(buf);

    MSG(buf)->drawText.x = x;
    MSG(buf)->drawText.y = y;
    MSG(buf)->drawText.fc = fc;
    MSG(buf)->drawText.bc = bc;
    memcpy(MSG(buf)->drawText.texts, text, len);

    int ret = write(imfd, buf, MSG(buf)->len);
}

static int process_message(Message *msg)
{
    int exit = 0;

    switch (msg->type) {
    case Disconnect:
        close(imfd);
        imfd = -1;
        exit = 1;
        break;

    case FbTermInfo:
        if (cbs.fbterm_info) {
            cbs.fbterm_info(&msg->info);
        }
        break;

    case Active:
        im_active = 1;
        if (cbs.active) {
            cbs.active();
        }
        break;

    case Deactive:
        if (cbs.deactive) {
            cbs.deactive();
        }
        im_active = 0;
        break;

    case ShowUI:
        if (im_active && cbs.show_ui) {
            cbs.show_ui(msg->winid);
        }
        break;

    case HideUI: {
        if (im_active && cbs.hide_ui) {
            cbs.hide_ui();
        }

        Message msg;
        msg.type = AckHideUI;
        msg.len = sizeof(msg);
        int ret = write(imfd, (char *)&msg, sizeof(msg));
        break;
        }

    case SendKey:
        if (im_active && cbs.send_key) {
                cbs.send_key(msg->keys, msg->len - OFFSET(Message, keys));
        }
        break;

    case CursorPosition:
        if (im_active && cbs.cursor_position) {
            cbs.cursor_position(msg->cursor.x, msg->cursor.y);
        }
        break;

    case TermMode:
        if (im_active && cbs.term_mode) {
            cbs.term_mode(msg->term.crWithLf, msg->term.applicKeypad, msg->term.cursorEscO);
        }
        break;

    default:
        break;
    }

    return exit;
}

static int process_messages(char *buf, int len)
{
    char *cur = buf, *end = cur + len;
    int exit = 0;

    for (; cur < end && MSG(cur)->len <= (end - cur); cur += MSG(cur)->len) {
        exit |= process_message(MSG(cur));
    }

    return exit;
}

static void wait_message(MessageType type)
{
    int ack = 0;
    while (!ack) {
        char *cur = pending_msg_buf + pending_msg_buf_len;
        int len = read(imfd, cur, sizeof(pending_msg_buf) - pending_msg_buf_len);

        if (len == -1 && (errno == EAGAIN || errno == EINTR)) continue;
        else if (len <= 0) {
            close(imfd);
            imfd = -1;
            return;
        }

        pending_msg_buf_len += len;

        char *end = cur + len;
        for (; cur < end && MSG(cur)->len <= (end - cur); cur += MSG(cur)->len) {
            if (MSG(cur)->type == type) {
                memcpy(cur, cur + MSG(cur)->len, end - cur - MSG(cur)->len);
                pending_msg_buf_len -= MSG(cur)->len;

                ack = 1;
                break;
            }
        }
    }

    if (pending_msg_buf_len) {
        Message msg;
        msg.type = Ping;
        msg.len = sizeof(msg);
        int ret = write(imfd, (char *)&msg, sizeof(msg));
    }
}

int check_im_message()
{
    if (imfd == -1) return 0;

    char buf[sizeof(pending_msg_buf)];
    int len, exit = 0;

    if (pending_msg_buf_len) {
        len = pending_msg_buf_len;
        pending_msg_buf_len = 0;

        memcpy(buf, pending_msg_buf, len);
        exit |= process_messages(buf, len);
    }

    len = read(imfd, buf, sizeof(buf));

    if (len == -1 && (errno == EAGAIN || errno == EINTR)) return 1;
    else if (len <= 0) {
        close(imfd);
        imfd = -1;
        return 0;
    }

    exit |= process_messages(buf, len);

    return !exit;
}

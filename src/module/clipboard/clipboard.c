/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"
#include <libintl.h>
#include "config.h"
#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/candidate.h"
#include "fcitx/hook.h"
#include "fcitx/keys.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utf8.h"

#include "clipboard-internal.h"
/* #ifdef ENABLE_X11 */
#include "clipboard-x11.h"
/* #endif */

#define FCITX_CLIPBOARD_BLANK " \b\f\v\r\t\n"

CONFIG_DEFINE_LOAD_AND_SAVE(FcitxClipboard, FcitxClipboardConfig,
                            "fcitx-clipboard");

static void *ClipboardCreate(FcitxInstance *instance);
static void ClipboardDestroy(void *arg);
static void ClipboardReloadConfig(void *arg);
static void ApplyClipboardConfig(FcitxClipboard *clipboard);

DECLARE_ADDFUNCTIONS(Clipboard)

FCITX_DEFINE_PLUGIN(fcitx_clipboard, module, FcitxModule) = {
    .Create = ClipboardCreate,
    .Destroy = ClipboardDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = ClipboardReloadConfig
};

static const unsigned int cmodifiers[_CBCM_COUNT] = {
    [CBCM_NONE] = FcitxKeyState_None,
    [CBCM_ALT] = FcitxKeyState_Alt,
    [CBCM_CTRL] = FcitxKeyState_Ctrl,
    [CBCM_SHIFT] = FcitxKeyState_Shift
};

static boolean
ClipboardSelectionEqual(ClipboardSelectionStr *sel, const char *str, size_t len)
{
    return len == sel->len && !memcmp(sel->str, str, len);
}

static int
ClipboardSelectionClipboardFind(FcitxClipboard *clipboard,
                                const char *str, size_t len)
{
    unsigned int i;
    for (i = 0;i < clipboard->clp_hist_len;i++) {
        if (ClipboardSelectionEqual(clipboard->clp_hist_lst + i, str, len)) {
            return i;
        }
    }
    return -1;
}

static const char*
ClipboardGetClipboard(FcitxClipboard *clipboard, unsigned index, unsigned *len)
{
    if (index >= clipboard->clp_hist_len) {
        if (len)
            *len = 0;
        return NULL;
    }
    ClipboardSelectionStr *selection = clipboard->clp_hist_lst + index;
    if (len)
        *len = selection->len;
    return selection->str;
}

static void
ClipboardWriteHistory(FcitxClipboard *clipboard)
{
    FILE *fp;
    fp = FcitxXDGGetFileUserWithPrefix("clipboard", "history.dat", "w", NULL);
    if (!fp)
        return;
    if (!clipboard->config.save_history)
        goto out;
    fcitx_utils_write_uint32(fp, clipboard->clp_hist_len);
    fcitx_utils_write_uint32(fp, clipboard->primary.len);
    unsigned int i;
    for (i = 0;i < clipboard->clp_hist_len;i++) {
        fcitx_utils_write_uint32(fp, clipboard->clp_hist_lst[i].len);
    }
    if (clipboard->primary.len)
        fwrite(clipboard->primary.str, 1, clipboard->primary.len, fp);
    for (i = 0;i < clipboard->clp_hist_len;i++) {
        if (clipboard->clp_hist_lst[i].len)
            fwrite(clipboard->clp_hist_lst[i].str, 1,
                   clipboard->clp_hist_lst[i].len, fp);
    }
out:
    fclose(fp);
}

static void
ClipboardInitReadHistory(FcitxClipboard *clipboard)
{
    FILE *fp;
    if (!clipboard->config.save_history)
        return;
    fp = FcitxXDGGetFileUserWithPrefix("clipboard", "history.dat", "r", NULL);
    if (!fp)
        return;
    uint32_t len;
    if (!fcitx_utils_read_uint32(fp, &len))
        goto out;
    fcitx_utils_read_uint32(fp, &clipboard->primary.len);
    if (len > (uint32_t)clipboard->config.history_len) {
        clipboard->clp_hist_len = clipboard->config.history_len;
    } else {
        clipboard->clp_hist_len = len;
    }
    ClipboardSelectionStr *clp_hist_lst = clipboard->clp_hist_lst;
    unsigned int i;
    for (i = 0;i < clipboard->clp_hist_len;i++) {
        fcitx_utils_read_uint32(fp, &clp_hist_lst[i].len);
    }
    if (fseek(fp, (len + 2) * sizeof(uint32_t), SEEK_SET) < 0) {
        clipboard->clp_hist_len = 0;
        clipboard->primary.len = 0;
        goto out;
    }
    clipboard->primary.str = malloc(clipboard->primary.len + 1);
    fread(clipboard->primary.str, 1, clipboard->primary.len, fp);
    clipboard->primary.str[clipboard->primary.len] = '\0';
    for (i = 0;i < clipboard->clp_hist_len;i++) {
        clp_hist_lst[i].str = malloc(clp_hist_lst[i].len + 1);
        fread(clp_hist_lst[i].str, 1, clp_hist_lst[i].len, fp);
        clp_hist_lst[i].str[clp_hist_lst[i].len] = '\0';
    }
out:
    fclose(fp);
}

static INPUT_RETURN_VALUE
ClipboardCommitCallback(void *arg, FcitxCandidateWord *cand_word)
{
    FcitxClipboard *clipboard = arg;
    FcitxInstance *instance = clipboard->owner;
    FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance),
                              cand_word->priv);
    return IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
}

static boolean
ClipboardPreHook(void *arg, FcitxKeySym sym, unsigned int state,
                 INPUT_RETURN_VALUE *ret_val)
{
    FcitxClipboard *clipboard = arg;
    FcitxInstance *instance = clipboard->owner;
    if (!clipboard->active)
        return false;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxGlobalConfig *fc = FcitxInstanceGetGlobalConfig(instance);
    *ret_val = IRV_TO_PROCESS;
    int key;
    FcitxCandidateWord *cand_word;
    if (FcitxHotkeyIsHotKey(sym, state, fc->nextWord)) {
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        cand_word = FcitxCandidateWordGetNext(cand_list, cand_word);
        if (!cand_word) {
            FcitxCandidateWordSetPage(cand_list, 0);
            cand_word = FcitxCandidateWordGetFirst(cand_list);
        } else {
            FcitxCandidateWordSetFocus(
                cand_list, FcitxCandidateWordGetIndex(cand_list, cand_word));
        }
    } else if (FcitxHotkeyIsHotKey(sym, state, fc->prevWord)) {
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        cand_word = FcitxCandidateWordGetPrev(cand_list, cand_word);
        if (!cand_word) {
            FcitxCandidateWordSetPage(
                cand_list, FcitxCandidateWordPageCount(cand_list) - 1);
            cand_word = FcitxCandidateWordGetLast(cand_list);
        } else {
            FcitxCandidateWordSetFocus(
                cand_list, FcitxCandidateWordGetIndex(cand_list, cand_word));
        }
    } else if (FcitxHotkeyIsHotKey(sym, state,
                                   FcitxConfigPrevPageKey(instance, fc))) {
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        if (!FcitxCandidateWordGoPrevPage(cand_list)) {
            FcitxCandidateWordSetType(cand_word, MSG_CANDIATE_CURSOR);
            *ret_val = IRV_DO_NOTHING;
            return true;
        }
        cand_word = FcitxCandidateWordGetCurrentWindow(cand_list) +
            FcitxCandidateWordGetCurrentWindowSize(cand_list) - 1;
    } else if (FcitxHotkeyIsHotKey(sym, state,
                                   FcitxConfigNextPageKey(instance, fc))) {
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        if (!FcitxCandidateWordGoNextPage(cand_list)) {
            FcitxCandidateWordSetType(cand_word, MSG_CANDIATE_CURSOR);
            *ret_val = IRV_DO_NOTHING;
            return true;
        }
        cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
    } else if ((key = FcitxCandidateWordCheckChooseKey(cand_list,
                                                       sym, state)) >= 0) {
        *ret_val = FcitxCandidateWordChooseByIndex(cand_list, key);
        return true;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        *ret_val = FcitxCandidateWordChooseByTotalIndex(
            cand_list, FcitxCandidateWordGetIndex(cand_list, cand_word));
        return true;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
        *ret_val = IRV_FLAG_RESET_INPUT | IRV_FLAG_UPDATE_INPUT_WINDOW;
        return true;
    } else {
        *ret_val = IRV_DO_NOTHING;
        return true;
    }

    FcitxCandidateWordSetType(cand_word, MSG_CANDIATE_CURSOR);
    *ret_val = IRV_FLAG_UPDATE_INPUT_WINDOW;
    return true;
}

#define case_blank case ' ': case '\t': case '\b': case '\n': case '\f': \
case '\v': case '\r'
#define CLIPBOARD_CAND_SEP "  \xe2\x80\xa6  "

static char*
ClipboardSelectionStrip(FcitxClipboard *clipboard,
                        const char *str, uint32_t len)
{
    const char *begin = str + strspn(str, " \t\b\n\f\v\r");
    const char *end = str + len;
    for (;end >= begin;end--) {
        switch (*(end - 1)) {
        case_blank:
            continue;
        default:
            break;
        }
        break;
    }
    if (end <= begin)
        return strdup("");
    len = end - begin;
    char *res;
    char *p;
    if (len < (uint32_t)clipboard->config.cand_max_len) {
        res = fcitx_utils_set_str_with_len(NULL, begin, len);
        goto out;
    }
    const char *begin_end = begin + clipboard->cand_half_len;
    const char *end_begin = end - clipboard->cand_half_len;
    for (;begin_end < end_begin;begin_end++) {
        if (fcitx_utf8_valid_start(*begin_end))
            break;
    }
    for (;begin_end < end_begin;end_begin--) {
        if (fcitx_utf8_valid_start(*end_begin))
            break;
    }
    int begin_len = begin_end - begin;
    int end_len = end - end_begin;
    res = malloc(begin_len + end_len + strlen(CLIPBOARD_CAND_SEP) + 1);
    p = res;
    memcpy(p, begin, begin_len);
    p += begin_len;
    memcpy(p, CLIPBOARD_CAND_SEP, strlen(CLIPBOARD_CAND_SEP));
    p += strlen(CLIPBOARD_CAND_SEP);
    memcpy(p, end_begin, end_len);
    p += end_len;
    *p = '\0';
out:
    for (p = res;*p;p++) {
        switch (*p) {
        case_blank:
            *p = ' ';
        }
    }
    return res;
}

static void
ClipboardSetCandWord(FcitxClipboard *clipboard,
                     FcitxCandidateWord *cand_word, ClipboardSelectionStr *str)
{
    cand_word->strWord = ClipboardSelectionStrip(clipboard, str->str, str->len);
    cand_word->priv = fcitx_utils_set_str_with_len(NULL, str->str, str->len);
}

static boolean
ClipboardPostHook(void *arg, FcitxKeySym sym, unsigned int state,
                  INPUT_RETURN_VALUE *ret_val)
{
    FcitxClipboard *clipboard = arg;
    FcitxClipboardConfig *config = &clipboard->config;
    if (!((clipboard->primary.len && config->use_primary) ||
          clipboard->clp_hist_len))
        return false;
    FcitxInstance *instance = clipboard->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    if (FcitxInputStateGetRawInputBufferSize(input))
        return false;
    if (!FcitxHotkeyIsHotKey(sym, state, config->trigger_key))
        return false;
    clipboard->active = true;
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxMessages *msg;
    FcitxCandidateWord cand_word = {
        .callback = ClipboardCommitCallback,
        .wordType = MSG_OTHER,
        .owner = clipboard
    };
    FcitxInstanceCleanInputWindow(instance);
    FcitxCandidateWordSetLayoutHint(cand_list, CLH_Vertical);
    int page_size = config->cand_max_len;
    if (page_size > 10)
        page_size = 10;
    FcitxCandidateWordSetPageSize(cand_list, page_size);
    FcitxCandidateWordSetChooseAndModifier(
        cand_list, DIGIT_STR_CHOOSE, cmodifiers[config->choose_modifier]);
    FcitxCandidateWordSetOverrideDefaultHighlight(cand_list, false);
    if (clipboard->clp_hist_len) {
        ClipboardSetCandWord(clipboard, &cand_word, clipboard->clp_hist_lst);
        FcitxCandidateWordAppend(cand_list, &cand_word);
    }
    int primary_found;
    if (clipboard->primary.len && config->use_primary) {
        primary_found = ClipboardSelectionClipboardFind(
            clipboard, clipboard->primary.str, clipboard->primary.len);
        if (primary_found == 0)
            goto skip_primary;
        ClipboardSetCandWord(clipboard, &cand_word, &clipboard->primary);
        FcitxCandidateWordAppend(cand_list, &cand_word);
    } else {
        primary_found = -1;
    }
skip_primary:
    msg = FcitxInputStateGetAuxUp(input);
    FcitxInputStateSetShowCursor(input, false);
    FcitxMessagesSetMessageCount(msg, 0);
    FcitxMessagesAddMessageStringsAtLast(msg, MSG_TIPS,
                                         _("Select to paste"));
    unsigned int i;
    for (i = 1;i < clipboard->clp_hist_len;i++) {
        if ((int)i == primary_found)
            continue;
        ClipboardSetCandWord(clipboard, &cand_word,
                             clipboard->clp_hist_lst + i);
        FcitxCandidateWordAppend(cand_list, &cand_word);
    }
    *ret_val = IRV_FLAG_UPDATE_INPUT_WINDOW;
    FcitxCandidateWordSetType(FcitxCandidateWordGetFirst(cand_list),
                              MSG_CANDIATE_CURSOR);
    return true;
}

static void
ClipboardReset(void *arg)
{
    FcitxClipboard *clipboard = arg;
    clipboard->active = false;
}

static void*
ClipboardCreate(FcitxInstance *instance)
{
    FcitxClipboard *clipboard = fcitx_utils_new(FcitxClipboard);
    clipboard->owner = instance;

    if (!FcitxClipboardLoadConfig(&clipboard->config)) {
        ClipboardDestroy(clipboard);
        return NULL;
    }
    ClipboardInitReadHistory(clipboard);
/* #ifdef ENABLE_X11 */
    ClipboardInitX11(clipboard);
/* #endif */
    ApplyClipboardConfig(clipboard);

    FcitxKeyFilterHook key_hook = {
        .arg = clipboard,
        .func = ClipboardPreHook
    };
    FcitxInstanceRegisterPreInputFilter(instance, key_hook);

    key_hook.func = ClipboardPostHook;
    FcitxInstanceRegisterPostInputFilter(instance, key_hook);

    FcitxIMEventHook reset_hook = {
        .arg = clipboard,
        .func = ClipboardReset
    };
    FcitxInstanceRegisterResetInputHook(instance, reset_hook);
    FcitxClipboardAddFunctions(instance);
    return clipboard;
}

static void
ClipboardDestroy(void *arg)
{
    FcitxClipboard *clipboard = (FcitxClipboard*)arg;
    ClipboardWriteHistory(clipboard);
    fcitx_utils_free(clipboard->primary.str);
    free(arg);
}

#define CAND_MAX_LEN_MAX 127
#define CAND_MAX_LEN_MIN 13

static void
ApplyClipboardConfig(FcitxClipboard *clipboard)
{
    FcitxClipboardConfig *config = &clipboard->config;
    if (config->history_len < 1) {
        config->history_len = 1;
    } else if (config->history_len > CLIPBOARD_MAX_LEN) {
        config->history_len = CLIPBOARD_MAX_LEN;
    }
    while (clipboard->clp_hist_len > (uint32_t)config->history_len) {
        char *str = clipboard->clp_hist_lst[--clipboard->clp_hist_len].str;
        fcitx_utils_free(str);
    }
    if (fcitx_unlikely(config->choose_modifier >= _CBCM_COUNT))
        config->choose_modifier = _CBCM_COUNT - 1;
    ClipboardWriteHistory(clipboard);
    if (config->cand_max_len < CAND_MAX_LEN_MIN) {
        config->cand_max_len = CAND_MAX_LEN_MIN;
    } else if (config->cand_max_len > CAND_MAX_LEN_MAX) {
        config->cand_max_len = CAND_MAX_LEN_MAX;
    }
    clipboard->cand_half_len = (config->cand_max_len -
                                strlen(CLIPBOARD_CAND_SEP)) / 2;
}

static void
ClipboardReloadConfig(void* arg)
{
    FcitxClipboard *clipboard = (FcitxClipboard*)arg;
    FcitxClipboardLoadConfig(&clipboard->config);
    ApplyClipboardConfig(clipboard);
}

static boolean
ClipboardCheckBlank(FcitxClipboard *clipboard, const char *str)
{
    if (clipboard->config.ignore_blank) {
        if (!str[strspn(str, FCITX_CLIPBOARD_BLANK)])
            return false;
    }
    return true;
}

void
ClipboardSetPrimary(FcitxClipboard *clipboard, uint32_t len, const char *str)
{
    if (!(len && str && *str))
        return;
    if (!ClipboardCheckBlank(clipboard, str))
        return;
    if (clipboard->primary.len != len) {
        clipboard->primary.len = len;
        clipboard->primary.str = realloc(clipboard->primary.str, len + 1);
    }
    memcpy(clipboard->primary.str, str, len);
    clipboard->primary.str[len] = '\0';
}

void
ClipboardPushClipboard(FcitxClipboard *clipboard, uint32_t len, const char *str)
{
    if (!(len && str && *str))
        return;
    if (!ClipboardCheckBlank(clipboard, str))
        return;
    int i = ClipboardSelectionClipboardFind(clipboard, str, len);
    if (i == 0) {
        return;
    } else if (i > 0) {
        ClipboardSelectionStr sel = clipboard->clp_hist_lst[i];
        memmove(clipboard->clp_hist_lst + 1, clipboard->clp_hist_lst,
                i * sizeof(ClipboardSelectionStr));
        clipboard->clp_hist_lst[0] = sel;
        return;
    }
    char *new_str;
    if (clipboard->clp_hist_len < (uint32_t)clipboard->config.history_len) {
        clipboard->clp_hist_len++;
        new_str = NULL;
    } else {
        new_str = clipboard->clp_hist_lst[clipboard->clp_hist_len - 1].str;
    }
    memmove(clipboard->clp_hist_lst + 1, clipboard->clp_hist_lst,
            (clipboard->clp_hist_len - 1) * sizeof(ClipboardSelectionStr));
    new_str = fcitx_utils_set_str_with_len(new_str, str, len);
    clipboard->clp_hist_lst->len = len;
    clipboard->clp_hist_lst->str = new_str;
}

#include "fcitx-clipboard-addfunctions.h"

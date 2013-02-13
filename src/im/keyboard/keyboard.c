/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
#include "config.h"

#include <libintl.h>
#include <errno.h>
#include <iconv.h>
#include <ctype.h>

#if defined(ENABLE_ICU)
#include <unicode/unorm.h>
#endif

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/keys.h"
#include "fcitx/candidate.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/hook.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "module/spell/fcitx-spell.h"

#include "keyboard.h"
#if defined(ENABLE_LIBXML2)
#include "isocodes.h"
#endif
#include "fcitx-compose-data.h"

#define INVALID_COMPOSE_RESULT 0xffffffff

#define PTR_TRUE ((void*)0x1)
#define PTR_FALSE ((void*)0x0)

static void* FcitxKeyboardCreate(FcitxInstance* instance);
static boolean FcitxKeyboardInit(void *arg);
static void  FcitxKeyboardResetIM(void *arg);
static void FcitxKeyboardOnClose(void* arg, FcitxIMCloseEventType event);
static INPUT_RETURN_VALUE FcitxKeyboardDoInput(void *arg, FcitxKeySym,
                                               unsigned int);
static void  FcitxKeyboardSave(void *arg);
static void  FcitxKeyboardReloadConfig(void *arg);
static INPUT_RETURN_VALUE FcitxKeyboardGetCandWords(void* arg);
static void FcitxKeyboardCommitBuffer(FcitxKeyboard* keyboard);
static void FcitxKeyboardLayoutCreate(FcitxKeyboard* keyboard,
                                      const char* name,
                                      const char* langCode,
                                      const char* layoutString,
                                      const char* variantString
                                     );
static boolean LoadKeyboardConfig(FcitxKeyboard* keyboard, FcitxKeyboardConfig* fs);
static void SaveKeyboardConfig(FcitxKeyboardConfig* fs);
static INPUT_RETURN_VALUE FcitxKeyboardHotkeyToggleWordHint(void* arg);
static inline boolean IsValidSym(FcitxKeySym keysym, unsigned int state);
static inline boolean IsValidChar(uint32_t c);
static uint32_t checkCompactTable(FcitxKeyboardLayout* layout, const FcitxComposeTableCompact *table);
static uint32_t checkAlgorithmically(FcitxKeyboardLayout* layout);
static uint32_t processCompose(FcitxKeyboardLayout* layout, uint32_t keyval, uint32_t state);

static int
compare_seq_index(const void *key, const void *value)
{
    const uint32_t *keysyms = (const uint32_t *)key;
    const uint32_t *seq = (const uint32_t *)value;

    if (keysyms[0] < seq[0])
        return -1;
    else if (keysyms[0] > seq[0])
        return 1;
    return 0;
}

static int
compare_seq(const void *key, const void *value)
{
    int i = 0;
    const uint32_t *keysyms = (const uint32_t *)key;
    const uint32_t *seq = (const uint32_t *)value;

    while (keysyms[i]) {
        if (keysyms[i] < seq[i])
            return -1;
        else if (keysyms[i] > seq[i])
            return 1;
        i++;
    }

    return 0;
}

static const uint32_t fcitx_compose_ignore[] = {
    FcitxKey_Shift_L,
    FcitxKey_Shift_R,
    FcitxKey_Control_L,
    FcitxKey_Control_R,
    FcitxKey_Caps_Lock,
    FcitxKey_Shift_Lock,
    FcitxKey_Meta_L,
    FcitxKey_Meta_R,
    FcitxKey_Alt_L,
    FcitxKey_Alt_R,
    FcitxKey_Super_L,
    FcitxKey_Super_R,
    FcitxKey_Hyper_L,
    FcitxKey_Hyper_R,
    FcitxKey_Mode_switch,
    FcitxKey_ISO_Level3_Shift,
    FcitxKey_VoidSymbol
};

static inline
void Ucs4ToUtf8(iconv_t conv, uint32_t ucs4, char* utf8)
{
    size_t inbytes = sizeof(uint32_t);
    size_t outbytes = UTF8_MAX_LENGTH;
    IconvStr in = (IconvStr) &ucs4;
    iconv(conv, &in, &inbytes,  &utf8, &outbytes);
}

CONFIG_DESC_DEFINE(GetKeyboardConfigDesc, "fcitx-keyboard.desc")

FCITX_DEFINE_PLUGIN(fcitx_keyboard, ime2, FcitxIMClass2) = {
    FcitxKeyboardCreate,
    NULL,
    FcitxKeyboardReloadConfig,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#if 0
static const char* FcitxKeyboardGetPastStream(void* arg)
{
    FcitxKeyboard* keyboard = arg;
    /* buggy surrounding text will cause trouble */
#if 0
    char* surrounding = NULL;
    unsigned int cursor = 0, anchor = 0;
    if (FcitxInstanceGetSurroundingText(keyboard->owner, FcitxInstanceGetCurrentIC(keyboard->owner), &surrounding, &cursor, &anchor)) {
        int len = cursor + strlen(keyboard->buffer[0]) + 1;
        if (keyboard->lastLength < len) {
            free(keyboard->tempBuffer);
            while (keyboard->lastLength < len)
                keyboard->lastLength *= 2;
            keyboard->tempBuffer = fcitx_utils_malloc0(keyboard->lastLength);
        }

        if (cursor) {
            int i = 0;
            while (surrounding[i] && i < cursor) {
                keyboard->tempBuffer[i] = surrounding[i];
                i++;
            }
            keyboard->tempBuffer[i] = '\0';
        }
        else {
            keyboard->tempBuffer[0] = '\0';
        }
        strcat(keyboard->tempBuffer, keyboard->buffer);
        free(surrounding);
        return keyboard->tempBuffer;
    }
    else
#endif
    return keyboard->buffer[0];
}

static const char* FcitxKeyboardGetFutureStream(void* arg)
{
    const char* result = "";
#if 0
    FcitxKeyboard* keyboard = arg;
    char* surrounding = NULL;
    unsigned int cursor = 0, anchor = 0;
    do {
        if (!FcitxInstanceGetSurroundingText(keyboard->owner, FcitxInstanceGetCurrentIC(keyboard->owner), &surrounding, &cursor, &anchor))
            break;
        size_t surlen = strlen(surrounding);
        if (surlen <= cursor)
            break;
        int len = surlen - cursor + 1;
        if (keyboard->lastLength < len) {
            while (keyboard->lastLength < len)
                keyboard->lastLength *= 2;
            keyboard->tempBuffer = realloc(keyboard->tempBuffer,
                                           keyboard->lastLength);
        }

        memcpy(keyboard->tempBuffer, &surrounding[cursor], len);
        result = keyboard->tempBuffer;
    } while(0);

    if (surrounding)
        free(surrounding);
#endif
    return result;
}
#endif


void FcitxKeyboardLayoutCreate(FcitxKeyboard* keyboard,
                               const char* name,
                               const char* langCode,
                               const char* layoutString,
                               const char* variantString)
{
    FcitxKeyboardLayout* layout = fcitx_utils_malloc0(sizeof(FcitxKeyboardLayout));

    layout->layoutString = strdup(layoutString);
    if (variantString)
        layout->variantString = strdup(variantString);
    layout->owner = keyboard;

    if (fcitx_utils_strcmp0(langCode, "en") == 0
        && fcitx_utils_strcmp0(layoutString, "us") == 0
        && fcitx_utils_strcmp0(variantString, NULL) == 0)
        keyboard->enUSRegistered = true;

    int iPriority = 100;
    if (strcmp(keyboard->initialLayout, layoutString) == 0 &&
        fcitx_utils_strcmp0(keyboard->initialVariant, variantString) == 0) {
        iPriority = PRIORITY_MAGIC_FIRST;
    }
    else {
        boolean result = false;
        FcitxXkbLayoutExists(keyboard->owner, layoutString,
                             variantString, &result);
        if (result)
            iPriority = 50;
    }

    char *uniqueName;
    if (variantString) {
        fcitx_utils_alloc_cat_str(uniqueName, "fcitx-keyboard-", layoutString,
                                  "-", variantString);
    } else {
        fcitx_utils_alloc_cat_str(uniqueName, "fcitx-keyboard-", layoutString);
    }

    FcitxIMIFace iface;
    memset(&iface, 0, sizeof(FcitxIMIFace));
    iface.Init = FcitxKeyboardInit;
    iface.ResetIM = FcitxKeyboardResetIM;
    iface.DoInput = FcitxKeyboardDoInput;
    iface.GetCandWords = FcitxKeyboardGetCandWords;
    iface.Save = FcitxKeyboardSave;
    iface.ReloadConfig = NULL;
    iface.OnClose = FcitxKeyboardOnClose;

    FcitxInstanceRegisterIMv2(
        keyboard->owner,
        layout,
        uniqueName,
        name,
        "kbd",
        iface,
        iPriority,
        langCode);
    free(uniqueName);
}

void* SimpleCopy(void* arg, void* dest, void* src)
{
    FCITX_UNUSED(arg);
    FCITX_UNUSED(dest);
    return src;
}

void* FcitxKeyboardCreate(FcitxInstance* instance)
{
    FcitxKeyboard* keyboard = fcitx_utils_malloc0(sizeof(FcitxKeyboard));
    keyboard->owner = instance;
    if (!LoadKeyboardConfig(keyboard, &keyboard->config))
    {
        free(keyboard);
        return NULL;
    }
    char* localepath = fcitx_utils_get_fcitx_path("localedir");
    bindtextdomain("xkeyboard-config", localepath);
    free(localepath);
    union {
        short s;
        unsigned char b[2];
    } endian;

    endian.s = 0x1234;
    if (endian.b[0] == 0x12)
        keyboard->iconv = iconv_open("utf-8", "ucs-4be");
    else
        keyboard->iconv = iconv_open("utf-8", "ucs-4le");

    FcitxHotkeyHook hk;
    hk.arg = keyboard;
    hk.hotkey = keyboard->config.hkToggleWordHint;
    hk.hotkeyhandle = FcitxKeyboardHotkeyToggleWordHint;

    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    FcitxXkbRules* rules = FcitxXkbGetRules(instance);
    keyboard->rules = rules;

    keyboard->initialLayout = NULL;
    keyboard->initialVariant = NULL;

    FcitxXkbGetCurrentLayout(instance, &keyboard->initialLayout,
                             &keyboard->initialVariant);
    if (!keyboard->initialLayout)
        keyboard->initialLayout = strdup("us");

#if defined(ENABLE_LIBXML2)
    if (rules && utarray_len(rules->layoutInfos)) {
        char* tempfile = NULL;
        FcitxXDGGetFileUserWithPrefix("", "", "w", NULL);
        FcitxXDGGetFileUserWithPrefix("", "cached_layout_XXXXXX", NULL, &tempfile);
        int fd = mkstemp(tempfile);

        FILE* fp = NULL;
        if (fd > 0)
            fp = fdopen(fd, "w");
        else {
            free(tempfile);
        }

        FcitxIsoCodes* isocodes = FcitxXkbReadIsoCodes(ISOCODES_ISO639_XML, ISOCODES_ISO3166_XML);
        FcitxXkbLayoutInfo* layoutInfo;
        for (layoutInfo = (FcitxXkbLayoutInfo*) utarray_front(rules->layoutInfos);
            layoutInfo != NULL;
            layoutInfo = (FcitxXkbLayoutInfo*) utarray_next(rules->layoutInfos, layoutInfo))
        {
            {
                char** plang = NULL;
                plang = (char**) utarray_front(layoutInfo->languages);
                char* lang = NULL;
                if (plang) {
                    FcitxIsoCodes639Entry* entry = FcitxIsoCodesGetEntry(isocodes, *plang);
                    if (entry) {
                        lang = entry->iso_639_1_code;
                    }
                }
                char *description;
                fcitx_utils_alloc_cat_str(description, _("Keyboard"), " - ",
                                          dgettext("xkeyboard-config",
                                                   layoutInfo->description));
                FcitxKeyboardLayoutCreate(keyboard, description,
                                          lang, layoutInfo->name, NULL);
                free(description);
                if (fp)
                    fprintf(fp, "%s:%s:%s\n", layoutInfo->description, layoutInfo->name, lang ? lang : "");
            }
            FcitxXkbVariantInfo* variantInfo;
            for (variantInfo = (FcitxXkbVariantInfo*) utarray_front(layoutInfo->variantInfos);
                variantInfo != NULL;
                variantInfo = (FcitxXkbVariantInfo*) utarray_next(layoutInfo->variantInfos, variantInfo))
            {
                char** plang = NULL;
                plang = (char**) utarray_front(variantInfo->languages);
                if (!plang)
                    plang = (char**) utarray_front(layoutInfo->languages);
                char* lang = NULL;
                if (plang) {
                    FcitxIsoCodes639Entry* entry = FcitxIsoCodesGetEntry(isocodes, *plang);
                    if (entry) {
                        lang = entry->iso_639_1_code;
                    }
                }
                char *description;
                fcitx_utils_alloc_cat_str(description, _("Keyboard"), " - ",
                                          dgettext("xkeyboard-config",
                                                   layoutInfo->description),
                                          " - ",
                                          dgettext("xkeyboard-config",
                                                   variantInfo->description));
                FcitxKeyboardLayoutCreate(keyboard, description, lang,
                                          layoutInfo->name, variantInfo->name);
                if (fp)
                    fprintf(fp, "%s:%s:%s:%s:%s\n",
                            layoutInfo->description, layoutInfo->name,
                            variantInfo->description, variantInfo->name,
                            lang ? lang : "");
                free(description);
            }
        }
        FcitxIsoCodesFree(isocodes);

        if (fp) {
            fclose(fp);
            char* layoutFileName = 0;
            FcitxXDGGetFileUserWithPrefix("", "cached_layout", NULL, &layoutFileName);
            if (access(layoutFileName, 0))
                unlink(layoutFileName);
            rename(tempfile, layoutFileName);

            free(tempfile);
            free(layoutFileName);
        }
    }
    else
#endif
    {
        /* though this is unrelated to libxml2, but can only generated with libxml2 enabled */
#if defined(ENABLE_LIBXML2)
        FILE* fp = FcitxXDGGetFileUserWithPrefix("", "cached_layout", "r", NULL);
        if (fp) {
            char *buf = NULL, *buf1 = NULL;
            size_t len = 0;
            while (getline(&buf, &len, fp) != -1) {
                fcitx_utils_free(buf1);
                buf1 = fcitx_utils_trim(buf);
                UT_array* list = fcitx_utils_split_string(buf1, ':');
                if (utarray_len(list) == 3) {
                    char** layoutDescription = (char**)utarray_eltptr(list, 0);
                    char** layoutName = (char**)utarray_eltptr(list, 1);
                    char** layoutLang = (char**)utarray_eltptr(list, 2);
                    char *description;
                    fcitx_utils_alloc_cat_str(description, _("Keyboard"), " - ",
                                              dgettext("xkeyboard-config",
                                                     *layoutDescription), " ", _("(Unavailable)"));
                    FcitxKeyboardLayoutCreate(keyboard, description,
                                            *layoutLang, *layoutName, NULL);
                }
                else if (utarray_len(list) == 5) {
                    char** layoutDescription = (char**)utarray_eltptr(list, 0);
                    char** layoutName = (char**)utarray_eltptr(list, 1);
                    char** variantDescription = (char**)utarray_eltptr(list, 2);
                    char** variantName = (char**)utarray_eltptr(list, 3);
                    char** variantLang = (char**)utarray_eltptr(list, 4);
                    char *description;
                    fcitx_utils_alloc_cat_str(description, _("Keyboard"), " - ",
                                              dgettext("xkeyboard-config",
                                                       *layoutDescription),
                                              " - ",
                                              dgettext("xkeyboard-config",
                                                       *variantDescription),
                                              " ", _("(Unavailable)"));
                    FcitxKeyboardLayoutCreate(keyboard, description, *variantLang,
                                              *layoutName, *variantName);
                }
                fcitx_utils_free_string_list(list);
            }

            fcitx_utils_free(buf);
            fcitx_utils_free(buf1);

            fclose(fp);
        }
        else
#endif
        {
            fcitx_utils_free(keyboard->initialLayout);
            keyboard->initialLayout = strdup("us");
            fcitx_utils_free(keyboard->initialVariant);
            keyboard->initialVariant = NULL;
            FcitxKeyboardLayoutCreate(keyboard, _("Keyboard"), "en", "us", NULL);
        }
    }

    /* always have en us is better */
    if (!keyboard->enUSRegistered)
        FcitxKeyboardLayoutCreate(keyboard, _("Keyboard"), "en", "us", NULL);

    keyboard->lastLength = 10;
    keyboard->tempBuffer = fcitx_utils_malloc0(keyboard->lastLength);

    keyboard->dataSlot = FcitxInstanceAllocDataForIC(instance, NULL, SimpleCopy, NULL, keyboard);

    return keyboard;
}

boolean FcitxKeyboardInit(void *arg)
{
    FcitxKeyboardLayout* layout = (FcitxKeyboardLayout*) arg;
    boolean flag = true;
    FcitxInstanceSetContext(layout->owner->owner,
                            CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(layout->owner->owner,
                            CONTEXT_DISABLE_QUICKPHRASE, &flag);
    FcitxInstanceSetContext(layout->owner->owner,
                            CONTEXT_DISABLE_FULLWIDTH, &flag);
    FcitxInstanceSetContext(layout->owner->owner,
                            CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT, &flag);
    if (layout->variantString) {
        char *string;
        fcitx_utils_alloc_cat_str(string, layout->layoutString, ",",
                                  layout->variantString);
        FcitxInstanceSetContext(layout->owner->owner,
                                CONTEXT_IM_KEYBOARD_LAYOUT, string);
        free(string);
    } else {
        FcitxInstanceSetContext(layout->owner->owner,
                                CONTEXT_IM_KEYBOARD_LAYOUT,
                                layout->layoutString);
    }
    return true;
}

void FcitxKeyboardCommitBuffer(FcitxKeyboard* keyboard)
{
    if (keyboard->buffer[0][0]) {
        FcitxInstanceCommitString(keyboard->owner,
                                  FcitxInstanceGetCurrentIC(keyboard->owner),
                                  keyboard->buffer[0]);
        FcitxKeyboardResetIM(keyboard);
    }
}

void  FcitxKeyboardResetIM(void *arg)
{
    FcitxKeyboardLayout *layout = (FcitxKeyboardLayout*) arg;
    FcitxKeyboard *keyboard = layout->owner;
    keyboard->cursor_moved = false;
    keyboard->buffer[0][0] = '\0';
    keyboard->cursorPos = 0;
    keyboard->composeBuffer[0] = 0;
    keyboard->n_compose = 0;
}

void FcitxKeyboardOnClose(void* arg, FcitxIMCloseEventType event)
{
    FcitxKeyboardLayout *layout = (FcitxKeyboardLayout*) arg;
    FcitxKeyboard *keyboard = layout->owner;
    if (event == CET_LostFocus) {
    } else if (event == CET_ChangeByInactivate) {
        FcitxKeyboardCommitBuffer(keyboard);
    } else if (event == CET_ChangeByUser) {
        FcitxKeyboardCommitBuffer(keyboard);
    }
}

static boolean IsDictAvailable(FcitxKeyboard* keyboard)
{
    return FcitxSpellDictAvailable(keyboard->owner, keyboard->dictLang, NULL);
}

/* a little bit funny here LOL.... */
static const FcitxHotkey FCITX_HYPHEN_APOS[2] = {
    {NULL, FcitxKey_minus, FcitxKeyState_None},
    {NULL, FcitxKey_apostrophe, FcitxKeyState_None}
};

static void
FcitxKeyboardSetBuff(FcitxKeyboard *keyboard, const char *str)
{
    int len = strlen(str);
    if (len > FCITX_KEYBOARD_MAX_BUFFER)
        len = FCITX_KEYBOARD_MAX_BUFFER;
    memcpy(keyboard->buffer[0], str, len);
    keyboard->cursorPos = len;
    keyboard->buffer[0][len] = '\0';
    keyboard->cursor_moved = false;
}

static INPUT_RETURN_VALUE
FcitxKeyboardHandleFocus(FcitxKeyboard *keyboard, FcitxKeySym sym,
                         unsigned int state)
{
    FcitxInstance *instance = keyboard->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWord *cand_word;
    if (!FcitxCandidateWordGetListSize(cand_list))
        return IRV_TO_PROCESS;
    FcitxGlobalConfig *fc = FcitxInstanceGetGlobalConfig(instance);
    if (FcitxHotkeyIsHotKey(sym, state, fc->nextWord)) {
        if (!keyboard->cursor_moved) {
            cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
        } else {
            cand_word = FcitxCandidateWordGetFocus(cand_list, true);
            cand_word = FcitxCandidateWordGetNext(cand_list, cand_word);
            if (!cand_word) {
                FcitxCandidateWordSetPage(cand_list, 0);
            } else {
                FcitxCandidateWordSetFocus(
                    cand_list, FcitxCandidateWordGetIndex(cand_list,
                                                          cand_word));
            }
        }
    } else if (FcitxHotkeyIsHotKey(sym, state, fc->prevWord)) {
        if (!keyboard->cursor_moved) {
            cand_word = FcitxCandidateWordGetByIndex(
                cand_list,
                FcitxCandidateWordGetCurrentWindowSize(cand_list) - 1);
        } else {
            cand_word = FcitxCandidateWordGetFocus(cand_list, true);
            cand_word = FcitxCandidateWordGetPrev(cand_list, cand_word);
            if (cand_word) {
                FcitxCandidateWordSetFocus(
                    cand_list, FcitxCandidateWordGetIndex(cand_list,
                                                          cand_word));
            }
        }
    } else {
        return IRV_TO_PROCESS;
    }
    if (cand_word) {
        FcitxCandidateWordSetType(cand_word, MSG_CANDIATE_CURSOR);
        if (!keyboard->cursor_moved)
            memcpy(keyboard->buffer[1], keyboard->buffer[0],
                   sizeof(keyboard->buffer[0]));
        FcitxKeyboardSetBuff(keyboard, cand_word->strWord);
        keyboard->cursor_moved = true;
    } else if (keyboard->cursor_moved) {
        FcitxKeyboardSetBuff(keyboard, keyboard->buffer[1]);
    } else {
        return IRV_FLAG_UPDATE_INPUT_WINDOW;
    }
    FcitxInputStateSetShowCursor(input, true);
    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessages *client_preedit = FcitxInputStateGetClientPreedit(input);
    FcitxMessagesAddMessageStringsAtLast(client_preedit, MSG_INPUT,
                                         keyboard->buffer[0]);
    FcitxInputStateSetClientCursorPos(input, keyboard->cursorPos);

    if (!FcitxInstanceICSupportPreedit(instance, FcitxInstanceGetCurrentIC(instance))) {
        FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input),
                                             MSG_INPUT, keyboard->buffer[0]);
        FcitxInputStateSetCursorPos(input, keyboard->cursorPos);
    }
    return IRV_FLAG_UPDATE_INPUT_WINDOW;
}

INPUT_RETURN_VALUE FcitxKeyboardDoInput(void *arg, FcitxKeySym sym, unsigned int state)
{
    FcitxKeyboardLayout* layout = (FcitxKeyboardLayout*) arg;
    FcitxKeyboard* keyboard = layout->owner;
    FcitxInstance* instance = keyboard->owner;
    INPUT_RETURN_VALUE res = FcitxKeyboardHandleFocus(keyboard, sym, state);
    if (res != IRV_TO_PROCESS)
        return res;
    const char* currentLang = FcitxInstanceGetContextString(instance,
                                                            CONTEXT_IM_LANGUAGE);
    if (currentLang == NULL)
        currentLang = "";

    if (sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R ||
        sym == FcitxKey_Alt_L || sym == FcitxKey_Alt_R ||
        sym == FcitxKey_Control_L || sym == FcitxKey_Control_R ||
        sym == FcitxKey_Super_L || sym == FcitxKey_Super_R)
        return IRV_TO_PROCESS;

    uint32_t result = processCompose(layout, sym, state);
    if (result == INVALID_COMPOSE_RESULT)
        return IRV_DO_NOTHING;

    if (result) {
        sym = 0;
        state = 0;
    }

    FcitxInputContext* currentIC = FcitxInstanceGetCurrentIC(instance);
    void* enableWordHint = FcitxInstanceGetICData(instance, currentIC,
                                                  keyboard->dataSlot);
    if (enableWordHint && strcmp(keyboard->dictLang, currentLang) != 0)
        strncpy(keyboard->dictLang, currentLang, LANGCODE_LENGTH);

    /* dict is set and loaded as a side effect */
    if (enableWordHint && IsDictAvailable(keyboard)) {
        FcitxInputState *input = FcitxInstanceGetInputState(instance);

        if (FcitxHotkeyIsHotKey(sym, state,
                                keyboard->config.hkAddToUserDict)) {
            if (FcitxSpellAddPersonal(instance, keyboard->buffer[0],
                                      keyboard->dictLang)) {
                return IRV_DO_NOTHING;
            }
        }

        if (IsValidChar(result) || FcitxHotkeyIsHotKeySimple(sym, state) ||
            IsValidSym(sym, state)) {
            if (IsValidChar(result) || FcitxHotkeyIsHotKeyLAZ(sym, state) ||
                FcitxHotkeyIsHotKeyUAZ(sym, state) || IsValidSym(sym, state) ||
                (*keyboard->buffer[0] &&
                 FcitxHotkeyIsHotKey(sym, state, FCITX_HYPHEN_APOS))) {
                char buf[UTF8_MAX_LENGTH + 1];
                memset(buf, 0, sizeof(buf));
                if (result)
                    Ucs4ToUtf8(keyboard->iconv, result, buf);
                else
                    Ucs4ToUtf8(keyboard->iconv, FcitxKeySymToUnicode(sym), buf);
                size_t charlen = strlen(buf);

                size_t len = strlen(keyboard->buffer[0]);
                if (len >= FCITX_KEYBOARD_MAX_BUFFER) {
                    FcitxInstanceCommitString(instance, currentIC,
                                              keyboard->buffer[0]);
                    keyboard->cursorPos = 0;
                    keyboard->buffer[0][0] = '\0';
                    len = 0;
                }
                if (keyboard->buffer[0][keyboard->cursorPos] != 0) {
                    memmove(keyboard->buffer[0] + keyboard->cursorPos + charlen,
                            keyboard->buffer[0] + keyboard->cursorPos,
                            len - keyboard->cursorPos);
                }
                keyboard->buffer[0][len + charlen] = '\0';
                strncpy(&keyboard->buffer[0][keyboard->cursorPos],
                        buf, charlen);
                keyboard->cursorPos += charlen;

                return IRV_DISPLAY_CANDWORDS;
            }
        } else {
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                size_t slen = strlen(keyboard->buffer[0]);
                if (slen > 0) {
                    if (keyboard->cursorPos > 0) {
                        size_t len = fcitx_utf8_strlen(keyboard->buffer[0]);
                        char *pos = fcitx_utf8_get_nth_char(keyboard->buffer[0],
                                                            len - 1);
                        keyboard->cursorPos = pos - keyboard->buffer[0];
                        memset(keyboard->buffer[0] + keyboard->cursorPos,
                               0, sizeof(char) * (slen - keyboard->cursorPos));
                        return IRV_DISPLAY_CANDWORDS;
                    } else {
                        return IRV_DO_NOTHING;
                    }
                }
            }
        }

        FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
        if (FcitxCandidateWordGetListSize(candList)) {
            int index = FcitxCandidateWordCheckChooseKey(candList, sym, state);
            if (index >= 0)
                return IRV_TO_PROCESS;
        }

        if (strlen(keyboard->buffer[0]) > 0) {
            INPUT_RETURN_VALUE irv = IRV_FLAG_FORWARD_KEY;
            if (keyboard->config.bUseEnterToCommit &&
                FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
                irv = 0;
            }
            FcitxInstanceCommitString(instance, currentIC, keyboard->buffer[0]);

            if (result) {
                char buf[UTF8_MAX_LENGTH + 1];
                memset(buf, 0, sizeof(buf));
                Ucs4ToUtf8(keyboard->iconv, result, buf);
                FcitxInstanceCommitString(instance, currentIC, buf);
                irv = 0;
            }
            irv |= IRV_FLAG_UPDATE_INPUT_WINDOW | IRV_FLAG_RESET_INPUT;
            return irv;
        }
    } else {
        FcitxUICloseInputWindow(instance);
    }
    if (result) {
        char buf[UTF8_MAX_LENGTH + 1];
        memset(buf, 0, sizeof(buf));
        Ucs4ToUtf8(keyboard->iconv, result, buf);
        FcitxInstanceCommitString(instance, currentIC, buf);
        return IRV_FLAG_UPDATE_INPUT_WINDOW | IRV_FLAG_RESET_INPUT;
    }
    return IRV_TO_PROCESS;
}

static INPUT_RETURN_VALUE
FcitxKeyboardGetCandWordCb(void *arg, const char *commit)
{
    FcitxKeyboardLayout *layout = (FcitxKeyboardLayout*) arg;
    FcitxKeyboard *keyboard = layout->owner;
    FcitxInstance *instance = keyboard->owner;
    char str[strlen(commit) + 2];
    strcpy(str, commit);
    if (keyboard->config.bCommitWithExtraSpace)
        strcat(str, " ");
    FcitxInstanceCommitString(instance,
                              FcitxInstanceGetCurrentIC(instance), str);
    return IRV_FLAG_RESET_INPUT;
}

INPUT_RETURN_VALUE FcitxKeyboardGetCandWords(void* arg)
{
    FcitxKeyboardLayout* layout = (FcitxKeyboardLayout*) arg;
    FcitxKeyboard* keyboard = layout->owner;
    FcitxInstance* instance = keyboard->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    keyboard->cursor_moved = false;
    if (keyboard->buffer[0][0] == '\0')
        return IRV_CLEAN;

    static const unsigned int cmodtable[] = {
        FcitxKeyState_None, FcitxKeyState_Alt,
        FcitxKeyState_Ctrl, FcitxKeyState_Shift};
    if (fcitx_unlikely(keyboard->config.chooseModifier >= _CM_COUNT))
        keyboard->config.chooseModifier = _CM_COUNT - 1;
    FcitxCandidateWordList *mainList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(mainList, keyboard->config.maximumHintLength);
    FcitxCandidateWordSetChooseAndModifier(
        mainList, DIGIT_STR_CHOOSE, cmodtable[keyboard->config.chooseModifier]);
    size_t bufferlen = strlen(keyboard->buffer[0]);
    memcpy(FcitxInputStateGetRawInputBuffer(input),
           keyboard->buffer[0], bufferlen + 1);
    FcitxInputStateSetRawInputBufferSize(input, bufferlen);
    FcitxInputStateSetShowCursor(input, true);
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input),
                                         MSG_INPUT, keyboard->buffer[0]);
    FcitxInputStateSetClientCursorPos(input, keyboard->cursorPos);
    if (!FcitxInstanceICSupportPreedit(instance, FcitxInstanceGetCurrentIC(instance))) {
        FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input),
                                             MSG_INPUT, keyboard->buffer[0]);
        FcitxInputStateSetCursorPos(input, keyboard->cursorPos);
    }

    if (bufferlen < keyboard->config.minimumHintLength)
        return IRV_DISPLAY_CANDWORDS;

    FcitxCandidateWordList *newList;
    newList = FcitxSpellGetCandWords(instance, NULL, keyboard->buffer[0], NULL,
                                     keyboard->config.maximumHintLength,
                                     keyboard->dictLang, NULL,
                                     FcitxKeyboardGetCandWordCb, layout);
    if (!newList)
        return IRV_DISPLAY_CANDWORDS;
    FcitxCandidateWordMerge(mainList, newList, -1);
    FcitxCandidateWordFreeList(newList);
    return IRV_DISPLAY_CANDWORDS;
}

void  FcitxKeyboardSave(void *arg)
{
}

void  FcitxKeyboardReloadConfig(void *arg)
{
    FcitxKeyboard* keyboard = (FcitxKeyboard*) arg;
    LoadKeyboardConfig(keyboard, &keyboard->config);
}

boolean LoadKeyboardConfig(FcitxKeyboard* keyboard, FcitxKeyboardConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetKeyboardConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-keyboard.config", "r", NULL);

    if (!fp) {
        if (errno == ENOENT)
            SaveKeyboardConfig(fs);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
    FcitxKeyboardConfigConfigBind(fs, cfile, configDesc);
    FcitxConfigBindSync(&fs->gconfig);

    if (fp)
        fclose(fp);

    return true;
}

void SaveKeyboardConfig(FcitxKeyboardConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetKeyboardConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-keyboard.config",
                                             "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}

INPUT_RETURN_VALUE FcitxKeyboardHotkeyToggleWordHint(void* arg)
{
    FcitxKeyboard* keyboard = (FcitxKeyboard*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(keyboard->owner);
    FcitxInputContext* currentIC = FcitxInstanceGetCurrentIC(keyboard->owner);
    if (!currentIC)
        return IRV_TO_PROCESS;
    if (im && strncmp(im->uniqueName, "fcitx-keyboard",
                      strlen("fcitx-keyboard")) == 0) {
        void* enableWordHint = FcitxInstanceGetICData(keyboard->owner,
                                                      currentIC,
                                                      keyboard->dataSlot);
        if (!enableWordHint) {
            enableWordHint = PTR_TRUE;
            IsDictAvailable(keyboard);
        } else {
            enableWordHint = PTR_FALSE;
        }
        FcitxInstanceSetICData(keyboard->owner, currentIC,
                               keyboard->dataSlot, enableWordHint);
        return IRV_DO_NOTHING;
    } else {
        return IRV_TO_PROCESS;
    }
}

static const uint32_t validSym[] =
{
0x0041,
0x0042,
0x0043,
0x0044,
0x0045,
0x0046,
0x0047,
0x0048,
0x0049,
0x004a,
0x004b,
0x004c,
0x004d,
0x004e,
0x004f,
0x0050,
0x0051,
0x0052,
0x0053,
0x0054,
0x0055,
0x0056,
0x0057,
0x0058,
0x0059,
0x005a,
0x0061,
0x0062,
0x0063,
0x0064,
0x0065,
0x0066,
0x0067,
0x0068,
0x0069,
0x006a,
0x006b,
0x006c,
0x006d,
0x006e,
0x006f,
0x0070,
0x0071,
0x0072,
0x0073,
0x0074,
0x0075,
0x0076,
0x0077,
0x0078,
0x0079,
0x007a,
0x00c0,
0x00c1,
0x00c2,
0x00c3,
0x00c4,
0x00c5,
0x00c6,
0x00c7,
0x00c8,
0x00c9,
0x00ca,
0x00cb,
0x00cc,
0x00cd,
0x00ce,
0x00cf,
0x00d0,
0x00d1,
0x00d2,
0x00d3,
0x00d4,
0x00d5,
0x00d6,
0x00d8,
0x00d8,
0x00d9,
0x00da,
0x00db,
0x00dc,
0x00dd,
0x00de,
0x00df,
0x00e0,
0x00e1,
0x00e2,
0x00e3,
0x00e4,
0x00e5,
0x00e6,
0x00e7,
0x00e8,
0x00e9,
0x00ea,
0x00eb,
0x00ec,
0x00ed,
0x00ee,
0x00ef,
0x00f0,
0x00f1,
0x00f2,
0x00f3,
0x00f4,
0x00f5,
0x00f6,
0x00f8,
0x00f8,
0x00f9,
0x00fa,
0x00fb,
0x00fc,
0x00fd,
0x00fe,
0x00ff,
0x01a1,
0x01a3,
0x01a5,
0x01a6,
0x01a9,
0x01aa,
0x01ab,
0x01ac,
0x01ae,
0x01af,
0x01b1,
0x01b3,
0x01b5,
0x01b6,
0x01b9,
0x01ba,
0x01bb,
0x01bc,
0x01be,
0x01bf,
0x01c0,
0x01c3,
0x01c5,
0x01c6,
0x01c8,
0x01ca,
0x01cc,
0x01cf,
0x01d0,
0x01d1,
0x01d2,
0x01d5,
0x01d8,
0x01d9,
0x01db,
0x01de,
0x01e0,
0x01e3,
0x01e5,
0x01e6,
0x01e8,
0x01ea,
0x01ec,
0x01ef,
0x01f0,
0x01f1,
0x01f2,
0x01f5,
0x01f8,
0x01f9,
0x01fb,
0x01fe,
0x02a1,
0x02a6,
0x02a9,
0x02ab,
0x02ac,
0x02b1,
0x02b6,
0x02b9,
0x02bb,
0x02bc,
0x02c5,
0x02c6,
0x02d5,
0x02d8,
0x02dd,
0x02de,
0x02e5,
0x02e6,
0x02f5,
0x02f8,
0x02fd,
0x02fe,
0x03a2,
0x03a3,
0x03a5,
0x03a6,
0x03aa,
0x03ab,
0x03ac,
0x03b3,
0x03b5,
0x03b6,
0x03ba,
0x03bb,
0x03bc,
0x03bd,
0x03bf,
0x03c0,
0x03c7,
0x03cc,
0x03cf,
0x03d1,
0x03d2,
0x03d3,
0x03d9,
0x03dd,
0x03de,
0x03e0,
0x03e7,
0x03ec,
0x03ef,
0x03f1,
0x03f2,
0x03f3,
0x03f9,
0x03fd,
0x03fe,
0x04a6,
0x04a7,
0x04a8,
0x04a9,
0x04aa,
0x04ab,
0x04ac,
0x04ad,
0x04ae,
0x04af,
0x04b1,
0x04b2,
0x04b3,
0x04b4,
0x04b5,
0x04b6,
0x04b7,
0x04b8,
0x04b9,
0x04ba,
0x04bb,
0x04bc,
0x04bd,
0x04be,
0x04bf,
0x04c0,
0x04c1,
0x04c2,
0x04c3,
0x04c4,
0x04c5,
0x04c6,
0x04c7,
0x04c8,
0x04c9,
0x04ca,
0x04cb,
0x04cc,
0x04cd,
0x04ce,
0x04cf,
0x04d0,
0x04d1,
0x04d2,
0x04d3,
0x04d4,
0x04d5,
0x04d6,
0x04d7,
0x04d8,
0x04d9,
0x04da,
0x04db,
0x04dc,
0x04dd,
0x05c1,
0x05c2,
0x05c3,
0x05c4,
0x05c5,
0x05c6,
0x05c7,
0x05c8,
0x05c9,
0x05ca,
0x05cb,
0x05cc,
0x05cd,
0x05ce,
0x05cf,
0x05d0,
0x05d1,
0x05d2,
0x05d3,
0x05d4,
0x05d5,
0x05d6,
0x05d7,
0x05d8,
0x05d9,
0x05da,
0x05e1,
0x05e2,
0x05e3,
0x05e4,
0x05e5,
0x05e6,
0x05e7,
0x05e8,
0x05e9,
0x05ea,
0x06a1,
0x06a2,
0x06a3,
0x06a4,
0x06a5,
0x06a6,
0x06a7,
0x06a8,
0x06a9,
0x06aa,
0x06ab,
0x06ac,
0x06ad,
0x06ae,
0x06af,
0x06b1,
0x06b2,
0x06b3,
0x06b4,
0x06b5,
0x06b6,
0x06b7,
0x06b8,
0x06b9,
0x06ba,
0x06bb,
0x06bc,
0x06bd,
0x06be,
0x06bf,
0x06c0,
0x06c1,
0x06c2,
0x06c3,
0x06c4,
0x06c5,
0x06c6,
0x06c7,
0x06c8,
0x06c9,
0x06ca,
0x06cb,
0x06cc,
0x06cd,
0x06ce,
0x06cf,
0x06d0,
0x06d1,
0x06d2,
0x06d3,
0x06d4,
0x06d5,
0x06d6,
0x06d7,
0x06d8,
0x06d9,
0x06da,
0x06db,
0x06dc,
0x06dd,
0x06de,
0x06df,
0x06e0,
0x06e1,
0x06e2,
0x06e3,
0x06e4,
0x06e5,
0x06e6,
0x06e7,
0x06e8,
0x06e9,
0x06ea,
0x06eb,
0x06ec,
0x06ed,
0x06ee,
0x06ef,
0x06f0,
0x06f1,
0x06f2,
0x06f3,
0x06f4,
0x06f5,
0x06f6,
0x06f7,
0x06f8,
0x06f9,
0x06fa,
0x06fb,
0x06fc,
0x06fd,
0x06fe,
0x06ff,
0x07a1,
0x07a2,
0x07a3,
0x07a4,
0x07a5,
0x07a7,
0x07a8,
0x07a9,
0x07ab,
0x07b1,
0x07b2,
0x07b3,
0x07b4,
0x07b5,
0x07b6,
0x07b7,
0x07b8,
0x07b9,
0x07ba,
0x07bb,
0x07c1,
0x07c2,
0x07c3,
0x07c4,
0x07c5,
0x07c6,
0x07c7,
0x07c8,
0x07c9,
0x07ca,
0x07cb,
0x07cb,
0x07cc,
0x07cd,
0x07ce,
0x07cf,
0x07d0,
0x07d1,
0x07d2,
0x07d4,
0x07d5,
0x07d6,
0x07d7,
0x07d8,
0x07d9,
0x07e1,
0x07e2,
0x07e3,
0x07e4,
0x07e5,
0x07e6,
0x07e7,
0x07e8,
0x07e9,
0x07ea,
0x07eb,
0x07eb,
0x07ec,
0x07ed,
0x07ee,
0x07ef,
0x07f0,
0x07f1,
0x07f2,
0x07f3,
0x07f4,
0x07f5,
0x07f6,
0x07f7,
0x07f8,
0x07f9,
0x08f6,
0x0ce0,
0x0ce1,
0x0ce2,
0x0ce3,
0x0ce4,
0x0ce5,
0x0ce6,
0x0ce7,
0x0ce8,
0x0ce9,
0x0cea,
0x0ceb,
0x0cec,
0x0ced,
0x0cee,
0x0cef,
0x0cf0,
0x0cf1,
0x0cf2,
0x0cf3,
0x0cf4,
0x0cf5,
0x0cf6,
0x0cf7,
0x0cf8,
0x0cf9,
0x0cfa,
0x0da1,
0x0da2,
0x0da3,
0x0da4,
0x0da5,
0x0da6,
0x0da7,
0x0da8,
0x0da9,
0x0daa,
0x0dab,
0x0dac,
0x0dad,
0x0dae,
0x0daf,
0x0db0,
0x0db1,
0x0db2,
0x0db3,
0x0db4,
0x0db5,
0x0db6,
0x0db7,
0x0db8,
0x0db9,
0x0dba,
0x0dbb,
0x0dbc,
0x0dbd,
0x0dbe,
0x0dbf,
0x0dc0,
0x0dc1,
0x0dc2,
0x0dc3,
0x0dc4,
0x0dc5,
0x0dc6,
0x0dc7,
0x0dc8,
0x0dc9,
0x0dca,
0x0dcb,
0x0dcc,
0x0dcd,
0x0dce,
0x0dcf,
0x0dd0,
0x0dd1,
0x0dd2,
0x0dd3,
0x0dd4,
0x0dd5,
0x0dd6,
0x0dd7,
0x0dd8,
0x0dd9,
0x0dda,
0x0de0,
0x0de1,
0x0de2,
0x0de3,
0x0de4,
0x0de5,
0x0de6,
0x0de7,
0x0de8,
0x0de9,
0x0dea,
0x0deb,
0x0dec,
0x0ded,
0x13be,
0x100012c,
0x100012d,
0x1000174,
0x1000175,
0x1000176,
0x1000177,
0x100018f,
0x100019f,
0x10001a0,
0x10001a1,
0x10001af,
0x10001b0,
0x10001b5,
0x10001b6,
0x10001d1,
0x10001d2,
0x10001e6,
0x10001e7,
0x1000259,
0x1000275,
0x1000492,
0x1000493,
0x1000496,
0x1000497,
0x100049a,
0x100049b,
0x100049c,
0x100049d,
0x10004a2,
0x10004a3,
0x10004ae,
0x10004af,
0x10004b0,
0x10004b1,
0x10004b2,
0x10004b3,
0x10004b6,
0x10004b7,
0x10004b8,
0x10004b9,
0x10004ba,
0x10004bb,
0x10004d8,
0x10004d9,
0x10004e2,
0x10004e3,
0x10004e8,
0x10004e9,
0x10004ee,
0x10004ef,
0x1000531,
0x1000532,
0x1000533,
0x1000534,
0x1000535,
0x1000536,
0x1000537,
0x1000538,
0x1000539,
0x100053a,
0x100053b,
0x100053c,
0x100053d,
0x100053e,
0x100053f,
0x1000540,
0x1000541,
0x1000542,
0x1000543,
0x1000544,
0x1000545,
0x1000546,
0x1000547,
0x1000548,
0x1000549,
0x100054a,
0x100054b,
0x100054c,
0x100054d,
0x100054e,
0x100054f,
0x1000550,
0x1000551,
0x1000552,
0x1000553,
0x1000554,
0x1000555,
0x1000556,
0x1000561,
0x1000562,
0x1000563,
0x1000564,
0x1000565,
0x1000566,
0x1000567,
0x1000568,
0x1000569,
0x100056a,
0x100056b,
0x100056c,
0x100056d,
0x100056e,
0x100056f,
0x1000570,
0x1000571,
0x1000572,
0x1000573,
0x1000574,
0x1000575,
0x1000576,
0x1000577,
0x1000578,
0x1000579,
0x100057a,
0x100057b,
0x100057c,
0x100057d,
0x100057e,
0x100057f,
0x1000580,
0x1000581,
0x1000582,
0x1000583,
0x1000584,
0x1000585,
0x1000586,
0x1000670,
0x1000679,
0x100067e,
0x1000686,
0x1000688,
0x1000691,
0x1000698,
0x10006a4,
0x10006a9,
0x10006af,
0x10006ba,
0x10006be,
0x10006c1,
0x10006cc,
0x10006cc,
0x10006d2,
0x10010d0,
0x10010d1,
0x10010d2,
0x10010d3,
0x10010d4,
0x10010d5,
0x10010d6,
0x10010d7,
0x10010d8,
0x10010d9,
0x10010da,
0x10010db,
0x10010dc,
0x10010dd,
0x10010de,
0x10010df,
0x10010e0,
0x10010e1,
0x10010e2,
0x10010e3,
0x10010e4,
0x10010e5,
0x10010e6,
0x10010e7,
0x10010e8,
0x10010e9,
0x10010ea,
0x10010eb,
0x10010ec,
0x10010ed,
0x10010ee,
0x10010ef,
0x10010f0,
0x10010f1,
0x10010f2,
0x10010f3,
0x10010f4,
0x10010f5,
0x10010f6,
0x1001e02,
0x1001e03,
0x1001e0a,
0x1001e0b,
0x1001e1e,
0x1001e1f,
0x1001e36,
0x1001e37,
0x1001e40,
0x1001e41,
0x1001e56,
0x1001e57,
0x1001e60,
0x1001e61,
0x1001e6a,
0x1001e6b,
0x1001e80,
0x1001e81,
0x1001e82,
0x1001e83,
0x1001e84,
0x1001e85,
0x1001e8a,
0x1001e8b,
0x1001ea0,
0x1001ea1,
0x1001ea2,
0x1001ea3,
0x1001ea4,
0x1001ea5,
0x1001ea6,
0x1001ea7,
0x1001ea8,
0x1001ea9,
0x1001eaa,
0x1001eab,
0x1001eac,
0x1001ead,
0x1001eae,
0x1001eaf,
0x1001eb0,
0x1001eb1,
0x1001eb2,
0x1001eb3,
0x1001eb4,
0x1001eb5,
0x1001eb6,
0x1001eb7,
0x1001eb8,
0x1001eb9,
0x1001eba,
0x1001ebb,
0x1001ebc,
0x1001ebd,
0x1001ebe,
0x1001ebf,
0x1001ec0,
0x1001ec1,
0x1001ec2,
0x1001ec3,
0x1001ec4,
0x1001ec5,
0x1001ec6,
0x1001ec7,
0x1001ec8,
0x1001ec9,
0x1001eca,
0x1001ecb,
0x1001ecc,
0x1001ecd,
0x1001ece,
0x1001ecf,
0x1001ed0,
0x1001ed1,
0x1001ed2,
0x1001ed3,
0x1001ed4,
0x1001ed5,
0x1001ed6,
0x1001ed7,
0x1001ed8,
0x1001ed9,
0x1001eda,
0x1001edb,
0x1001edc,
0x1001edd,
0x1001ede,
0x1001edf,
0x1001ee0,
0x1001ee1,
0x1001ee2,
0x1001ee3,
0x1001ee4,
0x1001ee5,
0x1001ee6,
0x1001ee7,
0x1001ee8,
0x1001ee9,
0x1001eea,
0x1001eeb,
0x1001eec,
0x1001eed,
0x1001eee,
0x1001eef,
0x1001ef0,
0x1001ef1,
0x1001ef2,
0x1001ef3,
0x1001ef4,
0x1001ef5,
0x1001ef6,
0x1001ef7,
0x1001ef8,
0x1001ef9
};

static const uint16_t validChar[] =
{
0x0041,
0x0042,
0x0043,
0x0044,
0x0045,
0x0046,
0x0047,
0x0048,
0x0049,
0x004a,
0x004b,
0x004c,
0x004d,
0x004e,
0x004f,
0x0050,
0x0051,
0x0052,
0x0053,
0x0054,
0x0055,
0x0056,
0x0057,
0x0058,
0x0059,
0x005a,
0x0061,
0x0062,
0x0063,
0x0064,
0x0065,
0x0066,
0x0067,
0x0068,
0x0069,
0x006a,
0x006b,
0x006c,
0x006d,
0x006e,
0x006f,
0x0070,
0x0071,
0x0072,
0x0073,
0x0074,
0x0075,
0x0076,
0x0077,
0x0078,
0x0079,
0x007a,
0x00c0,
0x00c1,
0x00c2,
0x00c3,
0x00c4,
0x00c5,
0x00c6,
0x00c7,
0x00c8,
0x00c9,
0x00ca,
0x00cb,
0x00cc,
0x00cd,
0x00ce,
0x00cf,
0x00d0,
0x00d1,
0x00d2,
0x00d3,
0x00d4,
0x00d5,
0x00d6,
0x00d8,
0x00d8,
0x00d9,
0x00da,
0x00db,
0x00dc,
0x00dd,
0x00de,
0x00df,
0x00e0,
0x00e1,
0x00e2,
0x00e3,
0x00e4,
0x00e5,
0x00e6,
0x00e7,
0x00e8,
0x00e9,
0x00ea,
0x00eb,
0x00ec,
0x00ed,
0x00ee,
0x00ef,
0x00f0,
0x00f1,
0x00f2,
0x00f3,
0x00f4,
0x00f5,
0x00f6,
0x00f8,
0x00f8,
0x00f9,
0x00fa,
0x00fb,
0x00fc,
0x00fd,
0x00fe,
0x00ff,
0x0100,
0x0101,
0x0102,
0x0103,
0x0104,
0x0105,
0x0106,
0x0107,
0x0108,
0x0109,
0x010a,
0x010b,
0x010c,
0x010d,
0x010e,
0x010f,
0x0110,
0x0111,
0x0112,
0x0113,
0x0116,
0x0117,
0x0118,
0x0119,
0x011a,
0x011b,
0x011c,
0x011d,
0x011e,
0x011f,
0x0120,
0x0121,
0x0122,
0x0123,
0x0124,
0x0125,
0x0126,
0x0127,
0x0128,
0x0129,
0x012a,
0x012b,
0x012c,
0x012d,
0x012e,
0x012f,
0x0130,
0x0131,
0x0134,
0x0135,
0x0136,
0x0137,
0x0138,
0x0139,
0x013a,
0x013b,
0x013c,
0x013d,
0x013e,
0x0141,
0x0142,
0x0143,
0x0144,
0x0145,
0x0146,
0x0147,
0x0148,
0x014a,
0x014b,
0x014c,
0x014d,
0x0150,
0x0151,
0x0154,
0x0155,
0x0156,
0x0157,
0x0158,
0x0159,
0x015a,
0x015b,
0x015c,
0x015d,
0x015e,
0x015f,
0x0160,
0x0161,
0x0162,
0x0163,
0x0164,
0x0165,
0x0166,
0x0167,
0x0168,
0x0169,
0x016a,
0x016b,
0x016c,
0x016d,
0x016e,
0x016f,
0x0170,
0x0171,
0x0172,
0x0173,
0x0174,
0x0175,
0x0176,
0x0177,
0x0178,
0x0179,
0x017a,
0x017b,
0x017c,
0x017d,
0x017e,
0x018f,
0x0192,
0x019f,
0x01a0,
0x01a1,
0x01af,
0x01b0,
0x01b5,
0x01b6,
0x01d2,
0x01d2,
0x01e6,
0x01e7,
0x0259,
0x0275,
0x0386,
0x0388,
0x0389,
0x038a,
0x038c,
0x038e,
0x038f,
0x0390,
0x0391,
0x0392,
0x0393,
0x0394,
0x0395,
0x0396,
0x0397,
0x0398,
0x0399,
0x039a,
0x039b,
0x039b,
0x039c,
0x039d,
0x039e,
0x039f,
0x03a0,
0x03a1,
0x03a3,
0x03a4,
0x03a5,
0x03a6,
0x03a7,
0x03a8,
0x03a9,
0x03aa,
0x03ab,
0x03ac,
0x03ad,
0x03ae,
0x03af,
0x03b0,
0x03b1,
0x03b2,
0x03b3,
0x03b4,
0x03b5,
0x03b6,
0x03b7,
0x03b8,
0x03b9,
0x03ba,
0x03bb,
0x03bb,
0x03bc,
0x03bd,
0x03be,
0x03bf,
0x03c0,
0x03c1,
0x03c2,
0x03c3,
0x03c4,
0x03c5,
0x03c6,
0x03c7,
0x03c8,
0x03c9,
0x03ca,
0x03cb,
0x03cc,
0x03cd,
0x03ce,
0x0401,
0x0402,
0x0403,
0x0404,
0x0405,
0x0406,
0x0407,
0x0408,
0x0409,
0x040a,
0x040b,
0x040c,
0x040e,
0x040f,
0x0410,
0x0411,
0x0412,
0x0413,
0x0414,
0x0415,
0x0416,
0x0417,
0x0418,
0x0419,
0x041a,
0x041b,
0x041c,
0x041d,
0x041e,
0x041f,
0x0420,
0x0421,
0x0422,
0x0423,
0x0424,
0x0425,
0x0426,
0x0427,
0x0428,
0x0429,
0x042a,
0x042b,
0x042c,
0x042d,
0x042e,
0x042f,
0x0430,
0x0431,
0x0432,
0x0433,
0x0434,
0x0435,
0x0436,
0x0437,
0x0438,
0x0439,
0x043a,
0x043b,
0x043c,
0x043d,
0x043e,
0x043f,
0x0440,
0x0441,
0x0442,
0x0443,
0x0444,
0x0445,
0x0446,
0x0447,
0x0448,
0x0449,
0x044a,
0x044b,
0x044c,
0x044d,
0x044e,
0x044f,
0x0451,
0x0452,
0x0453,
0x0454,
0x0455,
0x0456,
0x0457,
0x0458,
0x0459,
0x045a,
0x045b,
0x045c,
0x045e,
0x045f,
0x0490,
0x0491,
0x0492,
0x0493,
0x0496,
0x0497,
0x049a,
0x049b,
0x049c,
0x049d,
0x04a2,
0x04a3,
0x04ae,
0x04af,
0x04b0,
0x04b1,
0x04b2,
0x04b3,
0x04b6,
0x04b7,
0x04b8,
0x04b9,
0x04ba,
0x04bb,
0x04d8,
0x04d9,
0x04e2,
0x04e3,
0x04e8,
0x04e9,
0x04ee,
0x04ef,
0x0531,
0x0532,
0x0533,
0x0534,
0x0535,
0x0536,
0x0537,
0x0538,
0x0539,
0x053a,
0x053b,
0x053c,
0x053d,
0x053e,
0x053f,
0x0540,
0x0541,
0x0542,
0x0543,
0x0544,
0x0545,
0x0546,
0x0547,
0x0548,
0x0549,
0x054a,
0x054b,
0x054c,
0x054d,
0x054e,
0x054f,
0x0550,
0x0551,
0x0552,
0x0553,
0x0554,
0x0555,
0x0556,
0x0561,
0x0562,
0x0563,
0x0564,
0x0565,
0x0566,
0x0567,
0x0568,
0x0569,
0x056a,
0x056b,
0x056c,
0x056d,
0x056e,
0x056f,
0x0570,
0x0571,
0x0572,
0x0573,
0x0574,
0x0575,
0x0576,
0x0577,
0x0578,
0x0579,
0x057a,
0x057b,
0x057c,
0x057d,
0x057e,
0x057f,
0x0580,
0x0581,
0x0582,
0x0583,
0x0584,
0x0585,
0x0586,
0x05d0,
0x05d1,
0x05d2,
0x05d3,
0x05d4,
0x05d5,
0x05d6,
0x05d7,
0x05d8,
0x05d9,
0x05da,
0x05db,
0x05dc,
0x05dd,
0x05de,
0x05df,
0x05e0,
0x05e1,
0x05e2,
0x05e3,
0x05e4,
0x05e5,
0x05e6,
0x05e7,
0x05e8,
0x05e9,
0x05ea,
0x0621,
0x0622,
0x0623,
0x0624,
0x0625,
0x0626,
0x0627,
0x0628,
0x0629,
0x062a,
0x062b,
0x062c,
0x062d,
0x062e,
0x062f,
0x0630,
0x0631,
0x0632,
0x0633,
0x0634,
0x0635,
0x0636,
0x0637,
0x0638,
0x0639,
0x063a,
0x0641,
0x0642,
0x0643,
0x0644,
0x0645,
0x0646,
0x0647,
0x0648,
0x0649,
0x064a,
0x0670,
0x0679,
0x067e,
0x0686,
0x0688,
0x0691,
0x0698,
0x06a4,
0x06a9,
0x06af,
0x06ba,
0x06be,
0x06c1,
0x06cc,
0x06cc,
0x06d2,
0x0e01,
0x0e02,
0x0e03,
0x0e04,
0x0e05,
0x0e06,
0x0e07,
0x0e08,
0x0e09,
0x0e0a,
0x0e0b,
0x0e0c,
0x0e0d,
0x0e0e,
0x0e0f,
0x0e10,
0x0e11,
0x0e12,
0x0e13,
0x0e14,
0x0e15,
0x0e16,
0x0e17,
0x0e18,
0x0e19,
0x0e1a,
0x0e1b,
0x0e1c,
0x0e1d,
0x0e1e,
0x0e1f,
0x0e20,
0x0e21,
0x0e22,
0x0e23,
0x0e24,
0x0e25,
0x0e26,
0x0e27,
0x0e28,
0x0e29,
0x0e2a,
0x0e2b,
0x0e2c,
0x0e2d,
0x0e2e,
0x0e2f,
0x0e30,
0x0e31,
0x0e32,
0x0e33,
0x0e34,
0x0e35,
0x0e36,
0x0e37,
0x0e38,
0x0e39,
0x0e3a,
0x0e40,
0x0e41,
0x0e42,
0x0e43,
0x0e44,
0x0e45,
0x0e46,
0x0e47,
0x0e48,
0x0e49,
0x0e4a,
0x0e4b,
0x0e4c,
0x0e4d,
0x10d0,
0x10d1,
0x10d2,
0x10d3,
0x10d4,
0x10d5,
0x10d6,
0x10d7,
0x10d8,
0x10d9,
0x10da,
0x10db,
0x10dc,
0x10dd,
0x10de,
0x10df,
0x10e0,
0x10e1,
0x10e2,
0x10e3,
0x10e4,
0x10e5,
0x10e6,
0x10e7,
0x10e8,
0x10e9,
0x10ea,
0x10eb,
0x10ec,
0x10ed,
0x10ee,
0x10ef,
0x10f0,
0x10f1,
0x10f2,
0x10f3,
0x10f4,
0x10f5,
0x10f6,
0x1e02,
0x1e03,
0x1e0a,
0x1e0b,
0x1e1e,
0x1e1f,
0x1e36,
0x1e37,
0x1e40,
0x1e41,
0x1e56,
0x1e57,
0x1e60,
0x1e61,
0x1e6a,
0x1e6b,
0x1e80,
0x1e81,
0x1e82,
0x1e83,
0x1e84,
0x1e85,
0x1e8a,
0x1e8b,
0x1ea0,
0x1ea1,
0x1ea2,
0x1ea3,
0x1ea4,
0x1ea5,
0x1ea6,
0x1ea7,
0x1ea8,
0x1ea9,
0x1eaa,
0x1eab,
0x1eac,
0x1ead,
0x1eae,
0x1eaf,
0x1eb0,
0x1eb1,
0x1eb2,
0x1eb3,
0x1eb4,
0x1eb5,
0x1eb6,
0x1eb7,
0x1eb8,
0x1eb9,
0x1eba,
0x1ebb,
0x1ebc,
0x1ebd,
0x1ebe,
0x1ebf,
0x1ec0,
0x1ec1,
0x1ec2,
0x1ec3,
0x1ec4,
0x1ec5,
0x1ec6,
0x1ec7,
0x1ec8,
0x1ec9,
0x1eca,
0x1ecb,
0x1ecc,
0x1ecd,
0x1ece,
0x1ecf,
0x1ed0,
0x1ed1,
0x1ed2,
0x1ed3,
0x1ed4,
0x1ed5,
0x1ed6,
0x1ed7,
0x1ed8,
0x1ed9,
0x1eda,
0x1edb,
0x1edc,
0x1edd,
0x1ede,
0x1edf,
0x1ee0,
0x1ee1,
0x1ee2,
0x1ee3,
0x1ee4,
0x1ee5,
0x1ee6,
0x1ee7,
0x1ee8,
0x1ee9,
0x1eea,
0x1eeb,
0x1eec,
0x1eed,
0x1eee,
0x1eef,
0x1ef0,
0x1ef1,
0x1ef2,
0x1ef3,
0x1ef4,
0x1ef5,
0x1ef6,
0x1ef7,
0x1ef8,
0x1ef9,
0x30a1,
0x30a2,
0x30a3,
0x30a4,
0x30a5,
0x30a6,
0x30a7,
0x30a8,
0x30a9,
0x30aa,
0x30ab,
0x30ad,
0x30af,
0x30b1,
0x30b3,
0x30b5,
0x30b7,
0x30b9,
0x30bb,
0x30bd,
0x30bf,
0x30c1,
0x30c3,
0x30c4,
0x30c6,
0x30c8,
0x30ca,
0x30cb,
0x30cc,
0x30cd,
0x30ce,
0x30cf,
0x30d2,
0x30d5,
0x30d8,
0x30db,
0x30de,
0x30df,
0x30e0,
0x30e1,
0x30e2,
0x30e3,
0x30e4,
0x30e5,
0x30e6,
0x30e7,
0x30e8,
0x30e9,
0x30ea,
0x30eb,
0x30ec,
0x30ed,
0x30ef,
0x30f2,
0x30f3
};

static inline
boolean IsValidSym(FcitxKeySym keysym, unsigned int state) {
    if (state != 0)
        return false;
    int min = 0;
    int max = sizeof (validSym) / sizeof(validSym[0]) - 1;
    int mid;
    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (validSym[mid] < keysym)
            min = mid + 1;
        else if (validSym[mid] > keysym)
            max = mid - 1;
        else {
            /* found it */
            return true;
        }
    }
    return false;
}

static inline
boolean IsValidChar(uint32_t c) {
    if (c == 0 || c == INVALID_COMPOSE_RESULT)
        return false;

    int min = 0;
    int max = sizeof (validChar) / sizeof(validChar[0]) - 1;
    int mid;
    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (validChar[mid] < c)
            min = mid + 1;
        else if (validChar[mid] > c)
            max = mid - 1;
        else {
            /* found it */
            return true;
        }
    }
    return false;
}

uint32_t
processCompose(FcitxKeyboardLayout* layout, uint32_t keyval, uint32_t state)
{
    int i;
    FcitxKeyboard* keyboard = layout->owner;

    for (i = 0; fcitx_compose_ignore[i] != FcitxKey_VoidSymbol; i++) {
        if (keyval == fcitx_compose_ignore[i])
            return 0;
    }

    keyboard->composeBuffer[keyboard->n_compose ++] = keyval;
    keyboard->composeBuffer[keyboard->n_compose] = 0;

    uint32_t result;
    if ((result = checkCompactTable(layout, &fcitx_compose_table_compact))) {
        // qDebug () << "checkCompactTable ->true";
        return result;
    }

    if ((result = checkAlgorithmically(layout))) {
        // qDebug () << "checkAlgorithmically ->true";
        return result;
    }

    if (keyboard->n_compose > 1) {
        keyboard->composeBuffer[0] = 0;
        keyboard->n_compose = 0;
        return INVALID_COMPOSE_RESULT;
    } else {
        keyboard->composeBuffer[0] = 0;
        keyboard->n_compose = 0;
        return 0;
    }
}

uint32_t
checkCompactTable(FcitxKeyboardLayout* layout, const FcitxComposeTableCompact *table)
{
    int row_stride;
    const uint32_t *seq_index;
    const uint32_t *seq;
    int i;
    FcitxKeyboard* keyboard = layout->owner;

    /* Will never match, if the sequence in the compose buffer is longer
    * than the sequences in the table. Further, compare_seq (key, val)
    * will overrun val if key is longer than val. */
    if (keyboard->n_compose > table->max_seq_len)
        return 0;

    seq_index = (const uint32_t *)bsearch(keyboard->composeBuffer,
                                         table->data, table->n_index_size,
                                         sizeof(uint32_t) * table->n_index_stride,
                                         compare_seq_index);

    if (!seq_index) {
        return 0;
    }

    if (seq_index && keyboard->n_compose == 1) {
        return INVALID_COMPOSE_RESULT;
    }

    seq = NULL;
    for (i = keyboard->n_compose - 1; i < table->max_seq_len; i++) {
        row_stride = i + 1;

        if (seq_index[i + 1] - seq_index[i] > 0) {
            seq = (const uint32_t *) bsearch(keyboard->composeBuffer + 1,
                                            table->data + seq_index[i], (seq_index[i + 1] - seq_index[i]) / row_stride,
                                            sizeof(uint32_t) * row_stride,
                                            compare_seq);
            if (seq) {
                if (i == keyboard->n_compose - 1)
                    break;
                else {
                    return INVALID_COMPOSE_RESULT;
                }
            }
        }
    }

    if (!seq) {
        return 0;
    } else {
        uint32_t value;
        value = seq[row_stride - 1];
        keyboard->composeBuffer[0] = 0;
        keyboard->n_compose = 0;
        return value;
    }
    return 0;
}

#define IS_DEAD_KEY(k) \
    ((k) >= FcitxKey_dead_grave && (k) <= (FcitxKey_dead_dasia+1))

uint32_t
checkAlgorithmically(FcitxKeyboardLayout* layout)
{
#if defined(ENABLE_ICU)
    int i;
    UChar combination_buffer[FCITX_MAX_COMPOSE_LEN];
    FcitxKeyboard* keyboard = layout->owner;

    if (keyboard->n_compose >= FCITX_MAX_COMPOSE_LEN)
        return 0;

    for (i = 0;i < keyboard->n_compose &&
             IS_DEAD_KEY(keyboard->composeBuffer[i]);i++) {
    }
    if (i == keyboard->n_compose)
        return INVALID_COMPOSE_RESULT;

    if (i > 0 && i == keyboard->n_compose - 1) {
        combination_buffer[0] = FcitxKeySymToUnicode((FcitxKeySym) keyboard->composeBuffer[i]);
        combination_buffer[keyboard->n_compose] = 0;
        i--;
        while (i >= 0) {
            switch (keyboard->composeBuffer[i]) {
#define CASE(keysym, unicode) \
case FcitxKey_dead_##keysym: combination_buffer[i + 1] = unicode; break
            CASE(grave, 0x0300);
            CASE(acute, 0x0301);
            CASE(circumflex, 0x0302);
            CASE(tilde, 0x0303);   /* Also used with perispomeni, 0x342. */
            CASE(macron, 0x0304);
            CASE(breve, 0x0306);
            CASE(abovedot, 0x0307);
            CASE(diaeresis, 0x0308);
            CASE(hook, 0x0309);
            CASE(abovering, 0x030A);
            CASE(doubleacute, 0x030B);
            CASE(caron, 0x030C);
            CASE(abovecomma, 0x0313);         /* Equivalent to psili */
            CASE(abovereversedcomma, 0x0314); /* Equivalent to dasia */
            CASE(horn, 0x031B);    /* Legacy use for psili, 0x313 (or 0x343). */
            CASE(belowdot, 0x0323);
            CASE(cedilla, 0x0327);
            CASE(ogonek, 0x0328);  /* Legacy use for dasia, 0x314.*/
            CASE(iota, 0x0345);
            CASE(voiced_sound, 0x3099);    /* Per Markus Kuhn keysyms.txt file. */
            CASE(semivoiced_sound, 0x309A);    /* Per Markus Kuhn keysyms.txt file. */

            /* The following cases are to be removed once xkeyboard-config,
            * xorg are fully updated.
            */
                /* Workaround for typo in 1.4.x xserver-xorg */
            case 0xfe66: combination_buffer[i+1] = 0x314; break;
            /* CASE(dasia, 0x314); */
            /* CASE(perispomeni, 0x342); */
            /* CASE(psili, 0x343); */
#undef CASE
            default:
                combination_buffer[i + 1] = FcitxKeySymToUnicode((FcitxKeySym) keyboard->composeBuffer[i]);
            }
            i--;
        }

        /* If the buffer normalizes to a single character,
        * then modify the order of combination_buffer accordingly, if necessary,
        * and return TRUE.
        **/
#if 0
        if (check_normalize_nfc(combination_buffer, keyboard->n_compose)) {
            gunichar value;
            combination_utf8 = g_ucs4_to_utf8(combination_buffer, -1, NULL, NULL, NULL);
            nfc = g_utf8_normalize(combination_utf8, -1, G_NORMALIZE_NFC);

            value = g_utf8_get_char(nfc);
            gtk_im_context_simple_commit_char(GTK_IM_CONTEXT(context_simple), value);
            context_simple->compose_buffer[0] = 0;

            g_free(combination_utf8);
            g_free(nfc);

            return TRUE;
        }
#endif
        UErrorCode state = U_ZERO_ERROR;
        UChar result[FCITX_MAX_COMPOSE_LEN + 1];
        i = unorm_normalize(combination_buffer, keyboard->n_compose, UNORM_NFC, 0, result, FCITX_MAX_COMPOSE_LEN + 1, &state);

        // qDebug () << "combination_buffer = " << QString::fromUtf16(combination_buffer) << "keyboard->n_compose" << keyboard->n_compose;
        // qDebug () << "result = " << QString::fromUtf16(result) << "i = " << i << state;

        if (i == 1) {
            keyboard->composeBuffer[0] = 0;
            keyboard->n_compose = 0;
            return result[0];
        }
    }
#endif
    return 0;
}

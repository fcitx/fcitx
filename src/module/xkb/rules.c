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

#include <libxml/parser.h>

#include "common.h"
#include "rules.h"

static void RulesHandlerStartElement(void *ctx,
                              const xmlChar *name,
                              const xmlChar **atts);
static void RulesHandlerEndElement(void *ctx,
                const xmlChar *name);
static void RulesHandlerCharacters(void *ctx,
                const xmlChar *ch,
                int len);
static void FcitxXkbLayoutInfoInit(void* arg);
static void FcitxXkbVariantInfoInit(void* arg);
static void FcitxXkbModelInfoInit(void* arg);
static void FcitxXkbOptionGroupInfoInit(void* arg);
static void FcitxXkbOptionInfoInit(void* arg);
static void FcitxXkbLayoutInfoCopy(void* dst, const void* src);
static void FcitxXkbVariantInfoCopy(void* dst, const void* src);
static void FcitxXkbModelInfoCopy(void* dst, const void* src);
static void FcitxXkbOptionGroupInfoCopy(void* dst, const void* src);
static void FcitxXkbOptionInfoCopy(void* dst, const void* src);
static void FcitxXkbLayoutInfoFree(void* arg);
static void FcitxXkbVariantInfoFree(void* arg);
static void FcitxXkbModelInfoFree(void* arg);
static void FcitxXkbOptionGroupInfoFree(void* arg);
static void FcitxXkbOptionInfoFree(void* arg);
static inline FcitxXkbLayoutInfo* FindByName(FcitxXkbRules* rules, const char* name);
static void MergeRules(FcitxXkbRules* rules, FcitxXkbRules* rulesextra);

static const UT_icd layout_icd = {sizeof(FcitxXkbLayoutInfo), FcitxXkbLayoutInfoInit, FcitxXkbLayoutInfoCopy, FcitxXkbLayoutInfoFree };
static const UT_icd variant_icd = {sizeof(FcitxXkbVariantInfo), FcitxXkbVariantInfoInit, FcitxXkbVariantInfoCopy, FcitxXkbVariantInfoFree };
static const UT_icd model_icd = {sizeof(FcitxXkbModelInfo), FcitxXkbModelInfoInit, FcitxXkbModelInfoCopy, FcitxXkbModelInfoFree };
static const UT_icd option_group_icd = {sizeof(FcitxXkbOptionGroupInfo), FcitxXkbOptionGroupInfoInit, FcitxXkbOptionGroupInfoCopy, FcitxXkbOptionGroupInfoFree };
static const UT_icd option_icd = {sizeof(FcitxXkbOptionInfo), FcitxXkbOptionInfoInit, FcitxXkbOptionInfoCopy, FcitxXkbOptionInfoFree };
#define COPY_IF_NOT_NULL(x) ((x)?(strdup(x)):NULL)
#define utarray_clone(t, f) do { \
        utarray_new((t), (f)->icd); \
        utarray_concat((t), (f)); \
    } while(0)

boolean StringEndsWith(const char* str, const char* suffix)
{
    size_t sl = strlen(str);
    size_t sufl = strlen(suffix);
    if (sl < sufl)
        return false;
    const char* start = str + (sl - sufl);
    if (strncmp(start, suffix, sufl) == 0)
        return true;
    return false;
}

FcitxXkbRules* FcitxXkbReadRules(const char* file)
{
    xmlSAXHandler handle;
    memset(&handle, 0, sizeof(xmlSAXHandler));
    handle.startElement = RulesHandlerStartElement;
    handle.endElement = RulesHandlerEndElement;
    handle.characters = RulesHandlerCharacters;
    
    xmlInitParser();
    
    FcitxXkbRules* rules = (FcitxXkbRules*) fcitx_utils_malloc0(sizeof(FcitxXkbRules));
    utarray_new(rules->layoutInfos, &layout_icd);
    utarray_new(rules->modelInfos, &model_icd);
    utarray_new(rules->optionGroupInfos, &option_group_icd);
    
    FcitxXkbRulesHandler ruleshandler;
    ruleshandler.rules = rules;
    ruleshandler.path = fcitx_utils_new_string_list();
    ruleshandler.fromExtra = false;
    
    xmlSAXUserParseFile(&handle, &ruleshandler, file);
    utarray_free(ruleshandler.path);
    
    if (strcmp (file + strlen(file) - 4, ".xml") == 0) {
        char* extra = strndup (file, strlen (file) - 4), *extrafile;
        asprintf(&extrafile, "%s.extras.xml", extra);
        FcitxXkbRules* rulesextra = (FcitxXkbRules*) fcitx_utils_malloc0(sizeof(FcitxXkbRules));
        utarray_new(rulesextra->layoutInfos, &layout_icd);
        utarray_new(rulesextra->modelInfos, &model_icd);
        utarray_new(rulesextra->optionGroupInfos, &option_group_icd);
        ruleshandler.rules = rulesextra;
        ruleshandler.path = fcitx_utils_new_string_list();
        xmlSAXUserParseFile(&handle, &ruleshandler, extrafile);
        utarray_free(ruleshandler.path);
        free(extra);
        free(extrafile);
        
        MergeRules(rules, rulesextra);
    }
    
    
    xmlCleanupParser();

    return rules;
}

void FcitxXkbRulesFree(FcitxXkbRules* rules)
{
    if (!rules)
        return;
    
    utarray_free(rules->layoutInfos);
    utarray_free(rules->modelInfos);
    utarray_free(rules->optionGroupInfos);
    
    FREE_IF_NOT_NULL(rules->version);
    free(rules);
}

void MergeRules(FcitxXkbRules* rules, FcitxXkbRules* rulesextra)
{
    utarray_concat(rules->modelInfos, rulesextra->modelInfos);
    utarray_concat(rules->optionGroupInfos, rulesextra->optionGroupInfos);
    
    FcitxXkbLayoutInfo* layoutInfo;
    UT_icd icd = {sizeof(FcitxXkbLayoutInfo*), 0, 0, 0};
    UT_array toAdd;
    utarray_init(&toAdd, &icd);
    for (layoutInfo = (FcitxXkbLayoutInfo*) utarray_front(rulesextra->layoutInfos);
         layoutInfo != NULL;
         layoutInfo = (FcitxXkbLayoutInfo*) utarray_next(rulesextra->layoutInfos, layoutInfo))
    {
        FcitxXkbLayoutInfo* l = FindByName(rules, layoutInfo->name);
        if (l) {
            utarray_concat(l->languages, layoutInfo->languages);
            utarray_concat(l->variantInfos, layoutInfo->variantInfos);
        }
        else
            utarray_push_back(&toAdd, &layoutInfo);
    }
    
    int i;
    for(i = 0; i < utarray_len(&toAdd); i ++) {
        FcitxXkbLayoutInfo* p = *(FcitxXkbLayoutInfo**) utarray_eltptr(&toAdd, i);
        utarray_push_back(rules->layoutInfos, p);
    }
    
    utarray_done(&toAdd);
    FcitxXkbRulesFree(rulesextra);
}


char* FcitxXkbRulesToReadableString(FcitxXkbRules* rules)
{
    FcitxXkbLayoutInfo* layoutInfo;
    FcitxXkbModelInfo* modelInfo;
    FcitxXkbVariantInfo* variantInfo;
    FcitxXkbOptionInfo* optionInfo;
    FcitxXkbOptionGroupInfo* optionGroupInfo;
    
    UT_array* list = fcitx_utils_new_string_list();
    
    fcitx_utils_string_list_printf_append(list, "Version: %s", rules->version);
    
    for (layoutInfo = (FcitxXkbLayoutInfo*) utarray_front(rules->layoutInfos);
         layoutInfo != NULL;
         layoutInfo = (FcitxXkbLayoutInfo*) utarray_next(rules->layoutInfos, layoutInfo))
    {
        fcitx_utils_string_list_printf_append(list, "\tLayout Name: %s", layoutInfo->name);
        fcitx_utils_string_list_printf_append(list, "\tLayout Description: %s", layoutInfo->description);
        char* languages = fcitx_utils_join_string_list(layoutInfo->languages, ',');
        fcitx_utils_string_list_printf_append(list, "\tLayout Languages: %s", languages);
        free(languages);
        for (variantInfo = (FcitxXkbVariantInfo*) utarray_front(layoutInfo->variantInfos);
             variantInfo != NULL;
             variantInfo = (FcitxXkbVariantInfo*) utarray_next(layoutInfo->variantInfos, variantInfo))
        {
            fcitx_utils_string_list_printf_append(list, "\t\tVariant Name: %s", variantInfo->name);
            fcitx_utils_string_list_printf_append(list, "\t\tVariant Description: %s", variantInfo->description);
            char* languages = fcitx_utils_join_string_list(variantInfo->languages, ',');
            fcitx_utils_string_list_printf_append(list, "\t\tVariant Languages: %s", languages);
            free(languages);
        }
    }

    for (modelInfo = (FcitxXkbModelInfo*) utarray_front(rules->modelInfos);
         modelInfo != NULL;
         modelInfo = (FcitxXkbModelInfo*) utarray_next(rules->modelInfos, modelInfo))
    {
        fcitx_utils_string_list_printf_append(list, "\tModel Name: %s", modelInfo->name);
        fcitx_utils_string_list_printf_append(list, "\tModel Description: %s", modelInfo->description);
        fcitx_utils_string_list_printf_append(list, "\tModel Vendor: %s", modelInfo->vendor);
    }
    
    for (optionGroupInfo = (FcitxXkbOptionGroupInfo*) utarray_front(rules->optionGroupInfos);
         optionGroupInfo != NULL;
         optionGroupInfo = (FcitxXkbOptionGroupInfo*) utarray_next(rules->optionGroupInfos, optionGroupInfo))
    {
        fcitx_utils_string_list_printf_append(list, "\tOption Group Name: %s", optionGroupInfo->name);
        fcitx_utils_string_list_printf_append(list, "\tOption Group Description: %s", optionGroupInfo->description);
        fcitx_utils_string_list_printf_append(list, "\tOption Group Exclusive: %d", optionGroupInfo->exclusive);
        for (optionInfo = (FcitxXkbOptionInfo*) utarray_front(optionGroupInfo->optionInfos);
             optionInfo != NULL;
             optionInfo = (FcitxXkbOptionInfo*) utarray_next(optionGroupInfo->optionInfos, optionInfo))
        {
            fcitx_utils_string_list_printf_append(list, "\t\tOption Name: %s", optionInfo->name);
            fcitx_utils_string_list_printf_append(list, "\t\tOption Description: %s", optionInfo->description);
        }
    }
    
    char* result = fcitx_utils_join_string_list(list, '\n');
    utarray_free(list);
    return result;
}

static inline
FcitxXkbLayoutInfo* FindByName(FcitxXkbRules* rules, const char* name) {
    FcitxXkbLayoutInfo* layoutInfo;
    for (layoutInfo = (FcitxXkbLayoutInfo*) utarray_front(rules->layoutInfos);
         layoutInfo != NULL;
         layoutInfo = (FcitxXkbLayoutInfo*) utarray_next(rules->layoutInfos, layoutInfo))
    {
        if (strcmp(layoutInfo->name, name) == 0)
            break;
    }
    return layoutInfo;
}

/* code borrow from kde-workspace/kcontrol/keyboard/xkb_rules.cpp */
void RulesHandlerStartElement(void *ctx,
                              const xmlChar *name,
                              const xmlChar **atts)
{
    FcitxXkbRulesHandler* ruleshandler = (FcitxXkbRulesHandler*) ctx;
    FcitxXkbRules* rules = ruleshandler->rules;
    utarray_push_back(ruleshandler->path, &name);

    char* strPath = fcitx_utils_join_string_list(ruleshandler->path, '/');
    if ( StringEndsWith(strPath, "layoutList/layout/configItem") ) {
        utarray_extend_back(rules->layoutInfos);
    }
    else if ( StringEndsWith(strPath, "layoutList/layout/variantList/variant") ) {
        FcitxXkbLayoutInfo* layoutInfo = (FcitxXkbLayoutInfo*) utarray_back(rules->layoutInfos);
        utarray_extend_back(layoutInfo->variantInfos);
    }
    else if ( StringEndsWith(strPath, "modelList/model") ) {
        utarray_extend_back(rules->modelInfos);
    }
    else if ( StringEndsWith(strPath, "optionList/group") ) {
        utarray_extend_back(rules->optionGroupInfos);
        FcitxXkbOptionGroupInfo* optionGroupInfo = (FcitxXkbOptionGroupInfo*) utarray_back(rules->optionGroupInfos);
        int i = 0;
        while(atts && atts[i*2] != 0) {
            if (strcmp(XMLCHAR_CAST atts[i*2], "allowMultipleSelection") == 0) {
                optionGroupInfo->exclusive = (strcmp(XMLCHAR_CAST atts[i*2 + 1], "true") != 0);
            }
            i++;
        }
    }
    else if ( StringEndsWith(strPath, "optionList/group/option") ) {
        FcitxXkbOptionGroupInfo* optionGroupInfo = (FcitxXkbOptionGroupInfo*) utarray_back(rules->optionGroupInfos);
        utarray_extend_back(optionGroupInfo->optionInfos);
    }
    else if ( strcmp(strPath, "xkbConfigRegistry") == 0 ) {
        int i = 0;
        while(atts && atts[i*2] != 0) {
            if (strcmp(XMLCHAR_CAST atts[i*2], "version") == 0 && strlen(XMLCHAR_CAST atts[i*2 + 1]) != 0) {
                rules->version = strdup(XMLCHAR_CAST atts[i*2 + 1]);
            }
            i++;
        }
    }
    free(strPath);
}

void RulesHandlerEndElement(void *ctx,
                const xmlChar *name)
{
    FcitxXkbRulesHandler* ruleshandler = (FcitxXkbRulesHandler*) ctx;
    utarray_pop_back(ruleshandler->path);
}

void RulesHandlerCharacters(void *ctx,
                const xmlChar *ch,
                int len)
{
    FcitxXkbRulesHandler* ruleshandler = (FcitxXkbRulesHandler*) ctx;
    FcitxXkbRules* rules = ruleshandler->rules;
    char* temp = strndup(XMLCHAR_CAST ch, len);
    char* trimmed = fcitx_utils_trim(temp);
    free(temp);
    if ( strlen(trimmed) != 0 ) {
        char* strPath = fcitx_utils_join_string_list(ruleshandler->path, '/');
        FcitxXkbLayoutInfo* layoutInfo = (FcitxXkbLayoutInfo*) utarray_back(rules->layoutInfos);
        FcitxXkbModelInfo* modelInfo = (FcitxXkbModelInfo*) utarray_back(rules->modelInfos);
        FcitxXkbOptionGroupInfo* optionGroupInfo = (FcitxXkbOptionGroupInfo*) utarray_back(rules->optionGroupInfos);
        if ( StringEndsWith(strPath, "layoutList/layout/configItem/name") ) {
            if ( layoutInfo != NULL )
                layoutInfo->name = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "layoutList/layout/configItem/description") ) {
            layoutInfo->description = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "layoutList/layout/configItem/languageList/iso639Id") ) {
            utarray_push_back(layoutInfo->languages, &trimmed);
        }
        else if ( StringEndsWith(strPath, "layoutList/layout/variantList/variant/configItem/name") ) {
            FcitxXkbVariantInfo* variantInfo = (FcitxXkbVariantInfo*) utarray_back(layoutInfo->variantInfos);
            variantInfo->name = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "layoutList/layout/variantList/variant/configItem/description") ) {
            FcitxXkbVariantInfo* variantInfo = (FcitxXkbVariantInfo*) utarray_back(layoutInfo->variantInfos);
            FREE_IF_NOT_NULL(variantInfo->description);
            variantInfo->description = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "layoutList/layout/variantList/variant/configItem/languageList/iso639Id") ) {
            FcitxXkbVariantInfo* variantInfo = (FcitxXkbVariantInfo*) utarray_back(layoutInfo->variantInfos);
            utarray_push_back(variantInfo->languages, &trimmed);
        }
        else if ( StringEndsWith(strPath, "modelList/model/configItem/name") ) {
            modelInfo->name = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "modelList/model/configItem/description") ) {
            modelInfo->description = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "modelList/model/configItem/vendor") ) {
            modelInfo->vendor = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "optionList/group/configItem/name") ) {
            optionGroupInfo->name = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "optionList/group/configItem/description") ) {
            optionGroupInfo->description = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "optionList/group/option/configItem/name") ) {
            FcitxXkbOptionInfo* optionInfo = (FcitxXkbOptionInfo*) utarray_back(optionGroupInfo->optionInfos);
            optionInfo->name = strdup(trimmed);
        }
        else if ( StringEndsWith(strPath, "optionList/group/option/configItem/description") ) {
            FcitxXkbOptionInfo* optionInfo = (FcitxXkbOptionInfo*) utarray_back(optionGroupInfo->optionInfos);
            FREE_IF_NOT_NULL(optionInfo->description);
            optionInfo->description = strdup(trimmed);
        }
        free(strPath);
    }
    free(trimmed);
}


void FcitxXkbLayoutInfoInit(void* arg)
{
    FcitxXkbLayoutInfo* layoutInfo = (FcitxXkbLayoutInfo*) arg;
    memset(layoutInfo, 0, sizeof(FcitxXkbLayoutInfo));
    layoutInfo->languages = fcitx_utils_new_string_list();
    utarray_new(layoutInfo->variantInfos, &variant_icd);
}


void FcitxXkbVariantInfoInit(void* arg)
{
    FcitxXkbVariantInfo* variantInfo = (FcitxXkbVariantInfo*) arg;
    memset(variantInfo, 0, sizeof(FcitxXkbVariantInfo));
    variantInfo->languages = fcitx_utils_new_string_list();
}


void FcitxXkbModelInfoInit(void* arg)
{
    FcitxXkbModelInfo* modelInfo = (FcitxXkbModelInfo*) arg;
    memset(modelInfo, 0, sizeof(FcitxXkbModelInfo));
}


void FcitxXkbOptionGroupInfoInit(void* arg)
{
    FcitxXkbOptionGroupInfo* optionGroupInfo = (FcitxXkbOptionGroupInfo*) arg;
    memset(optionGroupInfo, 0, sizeof(FcitxXkbOptionGroupInfo));
    utarray_new(optionGroupInfo->optionInfos, &option_icd);
}


void FcitxXkbOptionInfoInit(void* arg)
{
    FcitxXkbOptionInfo* optionInfo = (FcitxXkbOptionInfo*) arg;
    memset(optionInfo, 0, sizeof(FcitxXkbOptionInfo));
}


void FcitxXkbLayoutInfoFree(void* arg)
{
    FcitxXkbLayoutInfo* layoutInfo = (FcitxXkbLayoutInfo*) arg;
    FREE_IF_NOT_NULL(layoutInfo->name);
    FREE_IF_NOT_NULL(layoutInfo->description);
    utarray_free(layoutInfo->languages);
    utarray_free(layoutInfo->variantInfos);
}


void FcitxXkbVariantInfoFree(void* arg)
{
    FcitxXkbVariantInfo* variantInfo = (FcitxXkbVariantInfo*) arg;
    FREE_IF_NOT_NULL(variantInfo->name);
    FREE_IF_NOT_NULL(variantInfo->description);
    utarray_free(variantInfo->languages);
}


void FcitxXkbModelInfoFree(void* arg)
{
    FcitxXkbModelInfo* modelInfo = (FcitxXkbModelInfo*) arg;
    FREE_IF_NOT_NULL(modelInfo->name);
    FREE_IF_NOT_NULL(modelInfo->description);
    FREE_IF_NOT_NULL(modelInfo->vendor);
}


void FcitxXkbOptionGroupInfoFree(void* arg)
{
    FcitxXkbOptionGroupInfo* optionGroupInfo = (FcitxXkbOptionGroupInfo*) arg;
    FREE_IF_NOT_NULL(optionGroupInfo->name);
    FREE_IF_NOT_NULL(optionGroupInfo->description);
    utarray_free(optionGroupInfo->optionInfos);
}


void FcitxXkbOptionInfoFree(void* arg)
{
    FcitxXkbOptionInfo* optionInfo = (FcitxXkbOptionInfo*) arg;
    FREE_IF_NOT_NULL(optionInfo->name);
    FREE_IF_NOT_NULL(optionInfo->description);
}

void FcitxXkbLayoutInfoCopy(void* dst, const void* src)
{
    FcitxXkbLayoutInfo* layoutInfoDst = (FcitxXkbLayoutInfo*) dst;
    FcitxXkbLayoutInfo* layoutInfoSrc = (FcitxXkbLayoutInfo*) src;
    layoutInfoDst->name = COPY_IF_NOT_NULL(layoutInfoSrc->name);
    layoutInfoDst->description = COPY_IF_NOT_NULL(layoutInfoSrc->description);
    utarray_clone(layoutInfoDst->languages, layoutInfoSrc->languages);
    utarray_clone(layoutInfoDst->variantInfos, layoutInfoSrc->variantInfos);
}

void FcitxXkbModelInfoCopy(void* dst, const void* src)
{
    FcitxXkbModelInfo* modeInfoDst = (FcitxXkbModelInfo*) dst;
    FcitxXkbModelInfo* modeInfoSrc = (FcitxXkbModelInfo*) src;
    modeInfoDst->name = COPY_IF_NOT_NULL(modeInfoSrc->name);
    modeInfoDst->description = COPY_IF_NOT_NULL(modeInfoSrc->description);
    modeInfoDst->vendor = COPY_IF_NOT_NULL(modeInfoSrc->vendor);
}

void FcitxXkbOptionGroupInfoCopy(void* dst, const void* src)
{
    FcitxXkbOptionGroupInfo* optionGroupInfoDst = (FcitxXkbOptionGroupInfo*) dst;
    FcitxXkbOptionGroupInfo* optionGroupInfoSrc = (FcitxXkbOptionGroupInfo*) src;
    optionGroupInfoDst->name = COPY_IF_NOT_NULL(optionGroupInfoSrc->name);
    optionGroupInfoDst->description = COPY_IF_NOT_NULL(optionGroupInfoSrc->description);
    optionGroupInfoDst->exclusive = optionGroupInfoSrc->exclusive;
    utarray_clone(optionGroupInfoDst->optionInfos, optionGroupInfoSrc->optionInfos);
}

void FcitxXkbOptionInfoCopy(void* dst, const void* src)
{
    FcitxXkbOptionInfo* optionInfoDst = (FcitxXkbOptionInfo*) dst;
    FcitxXkbOptionInfo* optionInfoSrc = (FcitxXkbOptionInfo*) src;
    optionInfoDst->name = COPY_IF_NOT_NULL(optionInfoSrc->name);
    optionInfoDst->description = COPY_IF_NOT_NULL(optionInfoSrc->description);
}

void FcitxXkbVariantInfoCopy(void* dst, const void* src)
{
    FcitxXkbVariantInfo* variantInfoDst = (FcitxXkbVariantInfo*) dst;
    FcitxXkbVariantInfo* variantInfoSrc = (FcitxXkbVariantInfo*) src;
    variantInfoDst->name = COPY_IF_NOT_NULL(variantInfoSrc->name);
    variantInfoDst->description = COPY_IF_NOT_NULL(variantInfoSrc->description);
    utarray_clone(variantInfoDst->languages, variantInfoSrc->languages);
}


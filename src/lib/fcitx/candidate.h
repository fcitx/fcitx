#ifndef FCITX_CANDIDATE_H
#define FCITX_CANDIDATE_H
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/ime.h>

struct _CandidateWord;
struct _CandidateWordList;

typedef enum _INPUT_RETURN_VALUE (*CandidateWordCommitCallback)(void* arg, struct _CandidateWord* candWord);

typedef struct _CandidateWord
{
    char* strWord;
    char* strExtra;
    CandidateWordCommitCallback callback;
    void* owner;
    void* priv;
} CandidateWord;

struct _CandidateWordList* CandidateWordInit();
void CandidateWordInsert(struct _CandidateWordList* candList, CandidateWord* candWord, int position);
void CandidateWordAppend(struct _CandidateWordList* candList, CandidateWord* candWord);
CandidateWord* CandidateWordGetCurrentWindow(struct _CandidateWordList* candList);
CandidateWord* CandidateWordGetCurrentWindowNext(struct _CandidateWordList* candList, CandidateWord* candWord);
CandidateWord* CandidateWordGetByIndex(struct _CandidateWordList* candList, int index);
enum _INPUT_RETURN_VALUE CandidateWordChooseByIndex(struct _CandidateWordList* candList, int index);
void CandidateWordFree(void* arg);
boolean CandidateWordHasNext(struct _CandidateWordList* candList);
boolean CandidateWordHasPrev(struct _CandidateWordList* candList);
int CandidateWordPageCount(struct _CandidateWordList* candList);
void CandidateWordReset(struct _CandidateWordList* candList);
boolean CandidateWordGoPrevPage(struct _CandidateWordList* candList);
boolean CandidateWordGoNextPage(struct _CandidateWordList* candList);
void CandidateWordSetChoose(struct _CandidateWordList* candList, const char* strChoose);
void CandidateWordResize(struct _CandidateWordList* candList, int length);
int CandidateWordGetPageSize(struct _CandidateWordList* candList);
void CandidateWordSetPageSize(struct _CandidateWordList* candList, int size);
const char* CandidateWordGetChoose(struct _CandidateWordList* candList);
int CandidateWordGetCurrentPage(struct _CandidateWordList* candList);
int CandidateWordGetCurrentWindowSize(struct _CandidateWordList* candList);
int CandidateWordGetListSize(struct _CandidateWordList* candList);
CandidateWord* CandidateWordGetFirst(struct _CandidateWordList* candList);
CandidateWord* CandidateWordGetNext(struct _CandidateWordList* candList, CandidateWord* candWord);

#define DIGIT_STR_CHOOSE "1234567890"

#endif
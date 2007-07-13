#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#include "ui.h"
#include "version.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "PYFA.h"
#include "py.h"
#include "sp.h"
#include "ime.h"

extern Display *dpy;
extern int      iScreen;
extern int      MAINWND_WIDTH;
extern int      iMainWindowX;
extern int      iMainWindowY;
extern int      iInputWindowX;
extern int      iInputWindowY;
extern int      iInputWindowWidth;
extern int      iInputWindowHeight;

extern int      iMaxCandWord;
extern Bool     _3DEffectMainWindow;
extern _3D_EFFECT _3DEffectInputWindow;
extern WINDOW_COLOR inputWindowColor;
extern WINDOW_COLOR mainWindowColor;
extern MESSAGE_COLOR IMNameColor[];
extern MESSAGE_COLOR messageColor[];
extern MESSAGE_COLOR inputWindowLineColor;
extern MESSAGE_COLOR mainWindowLineColor;
extern MESSAGE_COLOR cursorColor;
extern ENTER_TO_DO enterToDo;

extern HOTKEYS  hkTrigger[];
extern HOTKEYS  hkGBK[];
extern HOTKEYS  hkCorner[];
extern HOTKEYS  hkPunc[];
extern HOTKEYS  hkPrevPage[];
extern HOTKEYS  hkNextPage[];
extern HOTKEYS  hkWBAddPhrase[];
extern HOTKEYS  hkWBDelPhrase[];
extern HOTKEYS  hkWBAdjustOrder[];
extern HOTKEYS  hkPYAddFreq[];
extern HOTKEYS  hkPYDelFreq[];
extern HOTKEYS  hkPYDelUserPhr[];
extern HOTKEYS  hkLegend[];
extern HOTKEYS	hkTrack[];

extern KEYCODE  switchKey;
extern XIMTriggerKey *Trigger_Keys;
extern INT8     iTriggerKeyCount;

extern Bool     bUseGBK;
extern Bool     bEngPuncAfterNumber;
extern Bool     bAutoHideInputWindow;
extern XColor   colorArrow;
extern Bool	bTrackCursor;
extern HIDE_MAINWINDOW hideMainWindow;
extern Bool     bCompactMainWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern int      iFontSize;
extern int      iMainWindowFontSize;

extern Bool     bChnPunc;
extern Bool     bCorner;
extern Bool     bUseLegend;

extern Bool     bPYCreateAuto;
extern Bool     bPYSaveAutoAsPhrase;
extern Bool     bPhraseTips;
extern Bool     bEngAfterSemicolon;
extern Bool     bEngAfterCap;

extern Bool     bFullPY;
extern Bool     bDisablePagingInLegend;

extern int      i2ndSelectKey;
extern int      i3rdSelectKey;

extern char     strFontName[];

extern ADJUSTORDER baseOrder;
extern ADJUSTORDER phraseOrder;
extern ADJUSTORDER freqOrder;

extern INT8     iIMIndex;
extern Bool     bLocked;

extern MHPY     MHPY_C[];
extern MHPY     MHPY_S[];

extern Bool     bUsePinyin;
extern Bool     bUseSP;
extern Bool     bUseQW;
extern Bool     bUseTable;

extern Bool	bLumaQQ;

#ifdef _USE_XFT
extern Bool     bUseAA;
#else
extern char     strUserLocale[];
#endif

/*
#if defined(DARWIN)*/
/* function to reverse byte order for integer
this is required for Mac machine*/
/*int ReverseInt (unsigned int pc_int)
{
    int             mac_int;
    unsigned char  *p;

    mac_int = pc_int;
    p = (unsigned char *) &pc_int;
    mac_int = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
    return mac_int;
}
#endif
*/
/*
 * 读取用户的设置文件
 */
void LoadConfig (Bool bMode)
{
    FILE           *fp;
    char            str[PATH_MAX], *pstr;
    char            strPath[PATH_MAX];
    int             i;
    int             r, g, b;	//代表红绿蓝

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/config");

    fp = fopen (strPath, "rt");

    if (!fp) {
	SaveConfig ();
	LoadConfig (True);	//读入默认值
	return;
    }

    for (;;) {
	if (!fgets (str, PATH_MAX, fp))
	    break;

	i = strlen (str) - 1;
	while (str[i] == ' ' || str[i] == '\n')
	    str[i--] = '\0';

	pstr = str;
	if (*pstr == ' ')
	    pstr++;
	if (pstr[0] == '#')
	    continue;

	if (strstr (pstr, "显示字体=") && bMode) {
	    pstr += 9;
	    strcpy (strFontName, pstr);
	}
	else if (strstr (pstr, "显示字体大小=") && bMode) {
	    pstr += 13;
	    iFontSize = atoi (pstr);
	}
	else if (strstr (pstr, "主窗口字体大小=") && bMode) {
	    pstr += 15;
	    iMainWindowFontSize = atoi (pstr);
	}
#ifdef _USE_XFT
	else if (strstr (pstr, "是否使用AA字体=") && bMode) {
	    pstr += 15;
	    bUseAA = atoi (pstr);
	}
#else
	else if (strstr (pstr, "字体区域=") && bMode) {
	    pstr += 9;
	    strcpy (strUserLocale, pstr);
	}
#endif
	else if (strstr (pstr, "候选词个数=")) {
	    pstr += 11;
	    iMaxCandWord = atoi (pstr);
	    if (iMaxCandWord > 10)
		iMaxCandWord = MAX_CAND_WORD;
	}
	else if (strstr (pstr, "数字后跟半角符号=")) {
	    pstr += 17;
	    bEngPuncAfterNumber = atoi (pstr);
	}
	else if (strstr (pstr, "LumaQQ支持=") ) {
	    pstr += 11;
	    bLumaQQ = atoi(pstr);
	}
	else if (strstr (pstr, "主窗口是否使用3D界面=")) {
	    pstr += 21;
	    _3DEffectMainWindow = atoi (pstr);
	}
	else if (strstr (pstr, "输入条使用3D界面=")) {
	    pstr += 17;
	    _3DEffectInputWindow = (_3D_EFFECT)atoi (pstr);
	}
	else if (strstr (pstr, "是否自动隐藏输入条=")) {
	    pstr += 19;
	    bAutoHideInputWindow = atoi (pstr);
	}
	else if (strstr (pstr, "主窗口隐藏模式=")) {
	    pstr += 15;
	    hideMainWindow = (HIDE_MAINWINDOW) atoi (pstr);
	}
	else if (strstr (pstr, "光标色=") && bMode) {
	    pstr += 7;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    cursorColor.color.red = (r << 8);
	    cursorColor.color.green = (g << 8);
	    cursorColor.color.blue = (b << 8);
	}
	else if (strstr (pstr, "主窗口背景色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    mainWindowColor.backColor.red = (r << 8);
	    mainWindowColor.backColor.green = (g << 8);
	    mainWindowColor.backColor.blue = (b << 8);
	}
	else if (strstr (pstr, "主窗口线条色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    mainWindowLineColor.color.red = (r << 8);
	    mainWindowLineColor.color.green = (g << 8);
	    mainWindowLineColor.color.blue = (b << 8);
	}
	else if (strstr (pstr, "主窗口输入法名称色=") && bMode) {
	    int             r1, r2, g1, g2, b1, b2;

	    pstr += 19;
	    sscanf (pstr, "%d %d %d %d %d %d %d %d %d", &r, &g, &b, &r1, &g1, &b1, &r2, &g2, &b2);
	    IMNameColor[0].color.red = (r << 8);
	    IMNameColor[0].color.green = (g << 8);
	    IMNameColor[0].color.blue = (b << 8);
	    IMNameColor[1].color.red = (r1 << 8);
	    IMNameColor[1].color.green = (g1 << 8);
	    IMNameColor[1].color.blue = (b1 << 8);
	    IMNameColor[2].color.red = (r2 << 8);
	    IMNameColor[2].color.green = (g2 << 8);
	    IMNameColor[2].color.blue = (b2 << 8);
	}
	else if (strstr (pstr, "输入窗背景色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    inputWindowColor.backColor.red = (r << 8);
	    inputWindowColor.backColor.green = (g << 8);
	    inputWindowColor.backColor.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗提示色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[0].color.red = (r << 8);
	    messageColor[0].color.green = (g << 8);
	    messageColor[0].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗用户输入色=") && bMode) {
	    pstr += 17;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[1].color.red = (r << 8);
	    messageColor[1].color.green = (g << 8);
	    messageColor[1].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗序号色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[2].color.red = (r << 8);
	    messageColor[2].color.green = (g << 8);
	    messageColor[2].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗第一个候选字色=") && bMode) {
	    pstr += 21;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[3].color.red = (r << 8);
	    messageColor[3].color.green = (g << 8);
	    messageColor[3].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗用户词组色=") && bMode) {
	    pstr += 17;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[4].color.red = (r << 8);
	    messageColor[4].color.green = (g << 8);
	    messageColor[4].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗提示编码色=") && bMode) {
	    pstr += 17;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[5].color.red = (r << 8);
	    messageColor[5].color.green = (g << 8);
	    messageColor[5].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗其它文本色=") && bMode) {
	    pstr += 17;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    messageColor[6].color.red = (r << 8);
	    messageColor[6].color.green = (g << 8);
	    messageColor[6].color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗线条色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    inputWindowLineColor.color.red = (r << 8);
	    inputWindowLineColor.color.green = (g << 8);
	    inputWindowLineColor.color.blue = (b << 8);
	}
	else if (strstr (pstr, "输入窗箭头色=") && bMode) {
	    pstr += 13;
	    sscanf (pstr, "%d %d %d\n", &r, &g, &b);
	    colorArrow.red = (r << 8);
	    colorArrow.green = (g << 8);
	    colorArrow.blue = (b << 8);
	}

	else if (strstr (pstr, "打开/关闭输入法=") && bMode) {
	    pstr += 16;
	    SetTriggerKeys (pstr);
	    SetHotKey (pstr, hkTrigger);
	}
	else if (strstr (pstr, "中英文快速切换键=")) {
	    pstr += 17;
	    SetSwitchKey (pstr);
	}
	else if (strstr (pstr, "GBK支持=")) {
	    pstr += 8;
	    SetHotKey (pstr, hkGBK);
	}
	else if (strstr (pstr, "分号输入英文=")) {
	    pstr += 13;
	    bEngAfterSemicolon = atoi (pstr);
	}
	else if (strstr (pstr, "大写字母输入英文=")) {
	    pstr += 17;
	    bEngAfterCap = atoi (pstr);
	}
	else if (strstr (pstr, "联想方式禁止翻页=")) {
	    pstr += 17;
	    bDisablePagingInLegend = atoi (pstr);
	}
	else if (strstr (pstr, "联想支持=")) {
	    pstr += 9;
	    SetHotKey (pstr, hkLegend);
	}
	else if (strstr (pstr, "Enter键行为=")) {
	    pstr += 12;
	    enterToDo = (ENTER_TO_DO) atoi (pstr);
	}
	else if (strstr (pstr, "全半角=")) {
	    pstr += 7;
	    SetHotKey (pstr, hkCorner);
	}
	else if (strstr (pstr, "中文标点=")) {
	    pstr += 9;
	    SetHotKey (pstr, hkPunc);
	}
	else if (strstr (pstr, "上一页=")) {
	    pstr += 7;
	    SetHotKey (pstr, hkPrevPage);
	}
	else if (strstr (pstr, "下一页=")) {
	    pstr += 7;
	    SetHotKey (pstr, hkNextPage);
	}
	else if (strstr (pstr, "第二三候选词选择键=")) {
	    pstr += 19;
	    if (!strcasecmp (pstr, "SHIFT")) {
		i2ndSelectKey = 50;	//左SHIFT的扫描码
		i3rdSelectKey = 62;	//右SHIFT的扫描码
	    }
	    else if (!strcasecmp (pstr, "CTRL")) {
		i2ndSelectKey = 37;	//左CTRL的扫描码
		i3rdSelectKey = 109;	//右CTRL的扫描码
	    }
	}

	else if (strstr (pstr, "使用拼音=")) {
	    pstr += 9;
	    bUsePinyin = atoi (pstr);
	}
	else if (strstr (pstr, "使用双拼=")) {
	    pstr += 9;
	    bUseSP = atoi (pstr);
	}
	else if (strstr (pstr, "使用区位=")) {
	    pstr += 9;
	    bUseQW = atoi (pstr);
	}
	else if (strstr (pstr, "使用码表=")) {
	    pstr += 9;
	    bUseTable = atoi (pstr);
	}
	else if (strstr (pstr, "提示词库中的词组=")) {
	    pstr += 17;
	    bPhraseTips = atoi (pstr);
	}

	else if (strstr (str, "使用全拼=")) {
	    pstr += 9;
	    bFullPY = atoi (pstr);
	}
	else if (strstr (str, "拼音自动组词=")) {
	    pstr += 13;
	    bPYCreateAuto = atoi (pstr);
	}
	else if (strstr (str, "保存自动组词=")) {
	    pstr += 13;
	    bPYSaveAutoAsPhrase = atoi (pstr);
	}
	else if (strstr (str, "增加拼音常用字=")) {
	    pstr += 15;
	    SetHotKey (pstr, hkPYAddFreq);
	}
	else if (strstr (str, "删除拼音常用字=")) {
	    pstr += 15;
	    SetHotKey (pstr, hkPYDelFreq);
	}
	else if (strstr (str, "删除拼音用户词组=")) {
	    pstr += 17;
	    SetHotKey (pstr, hkPYDelUserPhr);
	}
	else if (strstr (str, "拼音单字重码调整方式=")) {
	    pstr += 21;
	    baseOrder = (ADJUSTORDER) atoi (pstr);
	}
	else if (strstr (str, "拼音词组重码调整方式=")) {
	    pstr += 21;
	    phraseOrder = (ADJUSTORDER) atoi (pstr);
	}
	else if (strstr (str, "拼音常用词重码调整方式=")) {
	    pstr += 23;
	    freqOrder = (ADJUSTORDER) atoi (pstr);
	}
	else if (strstr (str, "是否模糊an和ang=")) {
	    pstr += 16;
	    MHPY_C[0].bMode = atoi (pstr);
	    MHPY_S[5].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊en和eng=")) {
	    pstr += 16;
	    MHPY_C[1].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊ian和iang=")) {
	    pstr += 18;
	    MHPY_C[2].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊in和ing=")) {
	    pstr += 16;
	    MHPY_C[3].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊ou和u=")) {
	    pstr += 14;
	    MHPY_C[4].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊uan和uang=")) {
	    pstr += 18;
	    MHPY_C[5].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊c和ch=")) {
	    pstr += 14;
	    MHPY_S[0].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊f和h=")) {
	    pstr += 13;
	    MHPY_S[1].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊l和n=")) {
	    pstr += 13;
	    MHPY_S[2].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊s和sh=")) {
	    pstr += 14;
	    MHPY_S[3].bMode = atoi (pstr);
	}
	else if (strstr (str, "是否模糊z和zh=")) {
	    pstr += 14;
	    MHPY_S[4].bMode = atoi (pstr);
	}
    }

    fclose (fp);

    if (!Trigger_Keys) {
	iTriggerKeyCount = 0;
	Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
	Trigger_Keys[0].keysym = XK_space;
	Trigger_Keys[0].modifier = ControlMask;
	Trigger_Keys[0].modifier_mask = ControlMask;
	Trigger_Keys[1].keysym = 0;
	Trigger_Keys[1].modifier = 0;
	Trigger_Keys[1].modifier_mask = 0;
    }
}

void SaveConfig (void)
{
    FILE           *fp;
    char            strPath[PATH_MAX];

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");

    if (access (strPath, 0))
	mkdir (strPath, S_IRWXU);

    strcat (strPath, "config");
    fp = fopen (strPath, "wt");
    if (!fp) {
	fprintf (stderr, "\n无法创建文件 config！\n");
	return;
    }

    fprintf (fp, "[程序]\n");
    fprintf (fp, "显示字体=%s\n", strFontName);
    fprintf (fp, "显示字体大小=%d\n", iFontSize);
    fprintf (fp, "主窗口字体大小=%d\n", iMainWindowFontSize);
#ifdef _USE_XFT
    fprintf (fp, "是否使用AA字体=%d\n", bUseAA);
#else
    if ( strUserLocale[0] )
    	fprintf (fp, "字体区域=%s\n", strUserLocale);
    else
    	fprintf (fp, "#字体区域=zh_CN.gb2312\n");
#endif

    fprintf (fp, "\n[输出]\n");
    fprintf (fp, "数字后跟半角符号=%d\n", bEngPuncAfterNumber);
    fprintf (fp, "Enter键行为=%d\n", enterToDo);
    fprintf (fp, "分号输入英文=%d\n", bEngAfterSemicolon);
    fprintf (fp, "大写字母输入英文=%d\n", bEngAfterCap);
    fprintf (fp, "联想方式禁止翻页=%d\n", bDisablePagingInLegend);
    fprintf (fp, "LumaQQ支持=%d\n", bLumaQQ);

    fprintf (fp, "\n[界面]\n");
    fprintf (fp, "候选词个数=%d\n", iMaxCandWord);
    fprintf (fp, "主窗口是否使用3D界面=%d\n", _3DEffectMainWindow);
    fprintf (fp, "输入条使用3D界面=%d\n", _3DEffectInputWindow);
    fprintf (fp, "主窗口隐藏模式=%d\n", (int) hideMainWindow);
    fprintf (fp, "是否自动隐藏输入条=%d\n", bAutoHideInputWindow);
    
    fprintf (fp, "光标色=%d %d %d\n", cursorColor.color.red >> 8, cursorColor.color.green >> 8, cursorColor.color.blue >> 8);
    fprintf (fp, "主窗口背景色=%d %d %d\n", mainWindowColor.backColor.red >> 8, mainWindowColor.backColor.green >> 8, mainWindowColor.backColor.blue >> 8);
    fprintf (fp, "主窗口线条色=%d %d %d\n", mainWindowLineColor.color.red >> 8, mainWindowLineColor.color.green >> 8, mainWindowLineColor.color.blue >> 8);
    fprintf (fp, "主窗口输入法名称色=%d %d %d %d %d %d %d %d %d\n", IMNameColor[0].color.red >> 8, IMNameColor[0].color.green >> 8, IMNameColor[0].color.blue >> 8, IMNameColor[1].color.red >> 8, IMNameColor[1].color.green >> 8,
	     IMNameColor[1].color.blue >> 8, IMNameColor[2].color.red >> 8, IMNameColor[2].color.green >> 8, IMNameColor[2].color.blue >> 8);
    fprintf (fp, "输入窗背景色=%d %d %d\n", inputWindowColor.backColor.red >> 8, inputWindowColor.backColor.green >> 8, inputWindowColor.backColor.blue >> 8);
    fprintf (fp, "输入窗提示色=%d %d %d\n", messageColor[0].color.red >> 8, messageColor[0].color.green >> 8, messageColor[0].color.blue >> 8);
    fprintf (fp, "输入窗用户输入色=%d %d %d\n", messageColor[1].color.red >> 8, messageColor[1].color.green >> 8, messageColor[1].color.blue >> 8);
    fprintf (fp, "输入窗序号色=%d %d %d\n", messageColor[2].color.red >> 8, messageColor[2].color.green >> 8, messageColor[2].color.blue >> 8);
    fprintf (fp, "输入窗第一个候选字色=%d %d %d\n", messageColor[3].color.red >> 8, messageColor[3].color.green >> 8, messageColor[3].color.blue >> 8);
    fprintf (fp, "#该颜色值只用于拼音中的用户自造词\n");
    fprintf (fp, "输入窗用户词组色=%d %d %d\n", messageColor[4].color.red >> 8, messageColor[4].color.green >> 8, messageColor[4].color.blue >> 8);
    fprintf (fp, "输入窗提示编码色=%d %d %d\n", messageColor[5].color.red >> 8, messageColor[5].color.green >> 8, messageColor[5].color.blue >> 8);
    fprintf (fp, "#五笔、拼音的单字/系统词组均使用该颜色\n");
    fprintf (fp, "输入窗其它文本色=%d %d %d\n", messageColor[6].color.red >> 8, messageColor[6].color.green >> 8, messageColor[6].color.blue >> 8);
    fprintf (fp, "输入窗线条色=%d %d %d\n", inputWindowLineColor.color.red >> 8, inputWindowLineColor.color.green >> 8, inputWindowLineColor.color.blue >> 8);
    fprintf (fp, "输入窗箭头色=%d %d %d\n", colorArrow.red >> 8, colorArrow.green >> 8, colorArrow.blue >> 8);

    fprintf (fp, "\n#除了“中英文快速切换键”外，其它的热键均可设置为两个，中间用空格分隔\n");
    fprintf (fp, "[热键]\n");
    fprintf (fp, "打开/关闭输入法=CTRL_SPACE\n");
    fprintf (fp, "#中英文快速切换键 可以设置为L_CTRL R_CTRL L_SHIFT R_SHIFT\n");
    fprintf (fp, "中英文快速切换键=L_CTRL\n");
    fprintf (fp, "光标跟随=CTRL_K\n");
    fprintf (fp, "GBK支持=CTRL_M\n");
    fprintf (fp, "联想支持=CTRL_L\n");
    fprintf (fp, "全半角=SHIFT_SPACE\n");
    fprintf (fp, "中文标点=ALT_SPACE\n");
    fprintf (fp, "上一页=-\n");
    fprintf (fp, "下一页==\n");
    fprintf (fp, "第二三候选词选择键=SHIFT\n");

    fprintf (fp, "\n[输入法]\n");
    fprintf (fp, "使用拼音=%d\n", bUsePinyin);
    fprintf (fp, "使用双拼=%d\n", bUseSP);
    fprintf (fp, "使用区位=%d\n", bUseQW);
    fprintf (fp, "使用码表=%d\n", bUseTable);
    fprintf (fp, "提示词库中的词组=%d\n", bPhraseTips);

    fprintf (fp, "\n[拼音]\n");
    fprintf (fp, "使用全拼=%d\n", bFullPY);
    fprintf (fp, "拼音自动组词=%d\n", bPYCreateAuto);
    fprintf (fp, "保存自动组词=%d\n", bPYSaveAutoAsPhrase);
    fprintf (fp, "增加拼音常用字=CTRL_8\n");
    fprintf (fp, "删除拼音常用字=CTRL_7\n");
    fprintf (fp, "删除拼音用户词组=CTRL_DELETE\n");
    fprintf (fp, "#重码调整方式说明：0-->不调整  1-->快速调整  2-->按频率调整\n");
    fprintf (fp, "拼音单字重码调整方式=%d\n", baseOrder);
    fprintf (fp, "拼音词组重码调整方式=%d\n", phraseOrder);
    fprintf (fp, "拼音常用词重码调整方式=%d\n", freqOrder);
    fprintf (fp, "是否模糊an和ang=%d\n", MHPY_C[0].bMode);
    fprintf (fp, "是否模糊en和eng=%d\n", MHPY_C[1].bMode);
    fprintf (fp, "是否模糊ian和iang=%d\n", MHPY_C[2].bMode);
    fprintf (fp, "是否模糊in和ing=%d\n", MHPY_C[3].bMode);
    fprintf (fp, "是否模糊ou和u=%d\n", MHPY_C[4].bMode);
    fprintf (fp, "是否模糊uan和uang=%d\n", MHPY_C[5].bMode);
    fprintf (fp, "是否模糊c和ch=%d\n", MHPY_S[0].bMode);
    fprintf (fp, "是否模糊f和h=%d\n", MHPY_S[1].bMode);
    fprintf (fp, "是否模糊l和n=%d\n", MHPY_S[2].bMode);
    fprintf (fp, "是否模糊s和sh=%d\n", MHPY_S[3].bMode);
    fprintf (fp, "是否模糊z和zh=%d\n", MHPY_S[4].bMode);

    fclose (fp);
}

void LoadProfile (void)
{
    FILE           *fp;
    char            str[PATH_MAX], *pstr;
    char            strPath[PATH_MAX];
    int             i;
    Bool            bRetVal;

    iMainWindowX = MAINWND_STARTX;
    iMainWindowY = MAINWND_STARTY;
    iInputWindowX = INPUTWND_STARTX;
    iInputWindowY = INPUTWND_STARTY;

    bRetVal = False;
    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/profile");

    fp = fopen (strPath, "rt");

    if (fp) {
	for (;;) {
	    if (!fgets (str, PATH_MAX, fp))
		break;

	    i = strlen (str) - 1;
	    while (str[i] == ' ' || str[i] == '\n')
		str[i--] = '\0';

	    pstr = str;

	    if (strstr (str, "版本=")) {
		pstr += 5;

		if (!strcasecmp (FCITX_VERSION, pstr))
		    bRetVal = True;
	    }
	    else if (strstr (str, "主窗口位置X=")) {
		pstr += 12;
		iMainWindowX = atoi (pstr);
		if (iMainWindowX < 0)
		    iMainWindowX = 0;
		else if ((iMainWindowX + MAINWND_WIDTH) > DisplayWidth (dpy, iScreen))
		    iMainWindowX = DisplayWidth (dpy, iScreen) - MAINWND_WIDTH;
	    }
	    else if (strstr (str, "主窗口位置Y=")) {
		pstr += 12;
		iMainWindowY = atoi (pstr);
		if (iMainWindowY < 0)
		    iMainWindowY = 0;
		else if ((iMainWindowY + MAINWND_HEIGHT) > DisplayHeight (dpy, iScreen))
		    iMainWindowY = DisplayHeight (dpy, iScreen) - MAINWND_HEIGHT;
	    }
	    else if (strstr (str, "输入窗口位置X=")) {
		pstr += 14;
		iInputWindowX = atoi (pstr);
		if (iInputWindowX < 0)
		    iInputWindowX = 0;
		else if ((iInputWindowX + iInputWindowWidth) > DisplayWidth (dpy, iScreen))
		    iInputWindowX = DisplayWidth (dpy, iScreen) - iInputWindowWidth - 3;
	    }
	    else if (strstr (str, "输入窗口位置Y=")) {
		pstr += 14;
		iInputWindowY = atoi (pstr);
		if (iInputWindowY < 0)
		    iInputWindowY = 0;
		else if ((iInputWindowY + INPUTWND_HEIGHT) > DisplayHeight (dpy, iScreen))
		    iInputWindowY = DisplayHeight (dpy, iScreen) - iInputWindowHeight;
	    }
	    else if (strstr (str, "是否全角=")) {
		pstr += 9;
		bCorner = atoi (pstr);
	    }
	    else if (strstr (str, "是否中文标点=")) {
		pstr += 13;
		bChnPunc = atoi (pstr);
	    }
	    else if (strstr (str, "是否GBK=")) {
		pstr += 8;
		bUseGBK = atoi (pstr);
	    }
	    else if (strstr (str, "是否光标跟随=")) {
		pstr += 13;
		bTrackCursor = atoi (pstr);
	    }
	    else if (strstr (str, "是否联想=")) {
		pstr += 9;
		bUseLegend = atoi (pstr);
	    }
	    else if (strstr (str, "当前输入法=")) {
		pstr += 11;
		iIMIndex = atoi (pstr);
	    }
	    else if (strstr (str, "禁止用键盘切换=")) {
		pstr += 15;
		bLocked = atoi (pstr);
	    }
	    else if (strstr (str, "主窗口简洁模式=")) {
		pstr += 15;
		bCompactMainWindow = atoi (pstr);
	    }
	}

	fclose (fp);
    }

    if (!bRetVal) {
	SaveConfig ();
	SaveProfile ();
    }
}

void SaveProfile (void)
{
    FILE           *fp;
    char            strPath[PATH_MAX];

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");

    if (access (strPath, 0))
	mkdir (strPath, S_IRWXU);

    strcat (strPath, "profile");
    fp = fopen (strPath, "wt");

    if (!fp) {
	fprintf (stderr, "\n无法创建文件 profile!\n");
	return;
    }

    fprintf (fp, "版本=%s\n", FCITX_VERSION);
    fprintf (fp, "主窗口位置X=%d\n", iMainWindowX);
    fprintf (fp, "主窗口位置Y=%d\n", iMainWindowY);
    fprintf (fp, "输入窗口位置X=%d\n", iInputWindowX);
    fprintf (fp, "输入窗口位置Y=%d\n", iInputWindowY);
    fprintf (fp, "是否全角=%d\n", bCorner);
    fprintf (fp, "是否中文标点=%d\n", bChnPunc);
    fprintf (fp, "是否GBK=%d\n", bUseGBK);
    fprintf (fp, "是否光标跟随=%d\n", bTrackCursor);
    fprintf (fp, "是否联想=%d\n", bUseLegend);
    fprintf (fp, "当前输入法=%d\n", iIMIndex);
    fprintf (fp, "禁止用键盘切换=%d\n", bLocked);
    fprintf (fp, "主窗口简洁模式=%d\n", bCompactMainWindow);

    fclose (fp);
}

void SetHotKey (char *strKeys, HOTKEYS * hotkey)
{
    char           *p;
    char            strKey[30];
    int             i, j;

    p = strKeys;

    while (*p == ' ')
	p++;
    i = 0;
    while (p[i] != ' ' && p[i] != '\0')
	i++;
    strncpy (strKey, p, i);
    strKey[i] = '\0';
    p += i + 1l;
    j = ParseKey (strKey);
    if (j != -1)
	hotkey[0] = j;
    if (j == -1)
	j = 0;
    else
	j = 1;

    i = 0;
    while (p[i] != ' ' && p[i] != '\0')
	i++;
    if (p[0]) {
	strncpy (strKey, p, i);
	strKey[i] = '\0';

	i = ParseKey (strKey);
	if (i == -1)
	    i = 0;
    }
    else
	i = 0;

    hotkey[j] = i;
}

/*
 * 计算文件中有多少行
 * 注意:文件中的空行也做为一行处理
 */
int CalculateRecordNumber (FILE * fpDict)
{
    char            strText[101];
    int             nNumber = 0;

    for (;;) {
	if (!fgets (strText, 100, fpDict))
	    break;

	nNumber++;
    }
    rewind (fpDict);

    return nNumber;
}

void SetSwitchKey (char *str)
{
    if (!strcasecmp (str, "R_CTRL"))
	switchKey = R_CTRL;
    else if (!strcasecmp (str, "R_SHIFT"))
	switchKey = R_SHIFT;
    else if (!strcasecmp (str, "L_SHIFT"))
	switchKey = L_SHIFT;
    else
	switchKey = L_CTRL;
}

void SetTriggerKeys (char *str)
{
    int             i;
    char            strKey[2][30];
    char           *p;

    //首先来判断用户设置了几个热键，最多为2    
    p = str;

    i = 0;
    iTriggerKeyCount = 0;
    while (*p) {
	if (*p == ' ') {
	    iTriggerKeyCount++;
	    while (*p == ' ')
		p++;
	    strcpy (strKey[1], p);
	    break;
	}
	else
	    strKey[0][i++] = *p++;
    }
    strKey[0][i] = '\0';

    Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
    for (i = 0; i <= (iTriggerKeyCount + 1); i++) {
	Trigger_Keys[i].keysym = 0;
	Trigger_Keys[i].modifier = 0;
	Trigger_Keys[i].modifier_mask = 0;
    }

    for (i = 0; i <= iTriggerKeyCount; i++) {
	if (strstr (strKey[i], "CTRL_")) {
	    Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ControlMask;
	    Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ControlMask;
	}
	else if (strstr (strKey[i], "SHIFT_")) {
	    Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ShiftMask;
	    Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ShiftMask;
	}
	else if (strstr (strKey[i], "ALT_")) {
	    Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | Mod1Mask;
	    Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | Mod1Mask;
	}

	if (Trigger_Keys[i].modifier == 0) {
	    Trigger_Keys[i].modifier = ControlMask;
	    Trigger_Keys[i].modifier_mask = ControlMask;
	}

	p = strKey[i] + strlen (strKey[i]) - 1;
	while (*p != '_') {
	    p--;
	    if (p == strKey[i] || (p == strKey[i] + strlen (strKey[i]) - 1)) {
		Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
		Trigger_Keys[i].keysym = XK_space;
		return;
	    }
	}
	p++;

	if (strlen (p) == 1)
	    Trigger_Keys[i].keysym = tolower (*p);
	else if (!strcasecmp (p, "LCTRL"))
	    Trigger_Keys[i].keysym = XK_Control_L;
	else if (!strcasecmp (p, "RCTRL"))
	    Trigger_Keys[i].keysym = XK_Control_R;
	else if (!strcasecmp (p, "LSHIFT"))
	    Trigger_Keys[i].keysym = XK_Shift_L;
	else if (!strcasecmp (p, "RSHIFT"))
	    Trigger_Keys[i].keysym = XK_Shift_R;
	else if (!strcasecmp (p, "LALT"))
	    Trigger_Keys[i].keysym = XK_Alt_L;
	else if (!strcasecmp (p, "RALT"))
	    Trigger_Keys[i].keysym = XK_Alt_R;
	else
	    Trigger_Keys[i].keysym = XK_space;
    }
}

Bool CheckHZCharset (char *strHZ)
{
    if (!bUseGBK) {
	//GB2312的汉字编码规则为：第一个字节的值在0xA1到0xFE之间(实际为0xF7)，第二个字节的值在0xA1到0xFE之间
	//由于查到的资料说法不一，懒得核实，就这样吧
	int             i;

	for (i = 0; i < strlen (strHZ); i++) {
	    if ((unsigned char) strHZ[i] < (unsigned char) 0xA1 || (unsigned char) strHZ[i] > (unsigned char) 0xF7 || (unsigned char) strHZ[i + 1] < (unsigned char) 0xA1 || (unsigned char) strHZ[i + 1] > (unsigned char) 0xFE)
		return False;
	    i++;
	}
    }

    return True;
}

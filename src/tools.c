#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>

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
extern int      i3DEffect;
extern WINDOW_COLOR inputWindowColor;
extern WINDOW_COLOR mainWindowColor;
extern char     strUserLocale[];
extern MESSAGE_COLOR messageColor[];
extern MESSAGE_COLOR inputWindowLineColor;
extern MESSAGE_COLOR mainWindowLineColor;
extern MESSAGE_COLOR cursorColor;
extern ENTER_TO_DO enterToDo;

extern Bool     bSP;
extern HOTKEYS  hkGBK[];
extern HOTKEYS  hkCorner[];
extern HOTKEYS  hkPunc[];
extern HOTKEYS  hkPrevPage[];
extern HOTKEYS  hkNextPage[];
extern HOTKEYS  hkERBIPrevPage[];
extern HOTKEYS  hkERBINextPage[];
extern HOTKEYS  hkWBAddPhrase[];
extern HOTKEYS  hkWBDelPhrase[];
extern HOTKEYS  hkWBAdjustOrder[];
extern HOTKEYS  hkPYAddFreq[];
extern HOTKEYS  hkPYDelFreq[];
extern HOTKEYS  hkPYDelUserPhr[];
extern HOTKEYS  hkLegend[];
extern KEYCODE  switchKey;
extern Bool     bUseGBK;
extern Bool     bEngPuncAfterNumber;
extern Bool     bAutoHideInputWindow;
extern Bool     bTrackCursor;
extern Bool     bUseCtrlShift;
extern Bool     bWBAutoSendToClient;
extern Bool     bWBExactMatch;
extern Bool     bUseZPY;
extern Bool     bWBUseZ;
extern XColor   colorArrow;
extern HIDE_MAINWINDOW hideMainWindow;
extern Bool     bWBAutoAdjustOrder;
extern Bool     bPromptWBCode;
extern HIDE_MAINWINDOW hideMainWindow;
extern int      iFontSize;

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

extern IME      imeIndex;

extern MHPY     MHPY_C[];
extern MHPY     MHPY_S[];

#ifdef _USE_XFT
extern Bool     bUseAA;
#endif

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
	LoadConfig (True);		//读入默认值
	return;
    }

    strUserLocale[0] = '\0';

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

	if (strstr (pstr, "区域设置=") && bMode) {
	    pstr += 9;
	    strcpy (strUserLocale, pstr);
	}
	else if (strstr (pstr, "显示字体=") && bMode) {
	    pstr += 9;
	    strcpy (strFontName, pstr);
	}
	else if (strstr (pstr, "显示字体大小=") && bMode) {
	    pstr += 13;
	    iFontSize = atoi (pstr);
	}
#ifdef _USE_XFT
	else if (strstr (pstr, "是否使用AA字体=") && bMode) {
	    pstr += 15;
	    bUseAA = atoi (pstr);
	}
#endif
	else if (strstr (pstr, "是否使用Ctrl_Shift打开输入法=") && bMode) {
	    pstr += 29;
	    bUseCtrlShift = atoi (pstr);
	}
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

	else if (strstr (pstr, "是否使用3D界面=")) {
	    pstr += 15;
	    i3DEffect = atoi (pstr);
	}
	else if (strstr (pstr, "是否自动隐藏输入条=")) {
	    pstr += 19;
	    bAutoHideInputWindow = atoi (pstr);
	}
	else if (strstr (pstr, "主窗口隐藏模式=")) {
	    pstr += 15;
	    hideMainWindow = (HIDE_MAINWINDOW) atoi (pstr);
	}
	else if (strstr (pstr, "是否光标跟随=")) {
	    pstr += 13;
	    bTrackCursor = atoi (pstr);
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

	else if (strstr (pstr, "五笔四键自动上屏=")) {
	    pstr += 17;
	    bWBAutoSendToClient = atoi (pstr);
	}
	else if (strstr (pstr, "自动调整五笔顺序=")) {
	    pstr += 17;
	    bWBAutoAdjustOrder = atoi (pstr);
	}
	else if (strstr (pstr, "提示词库中已有的词组=")) {
	    pstr += 21;
	    bPhraseTips = atoi (pstr);
	}
	else if (strstr (pstr, "使用Z输入拼音=")) {
	    pstr += 14;
	    bUseZPY = atoi (pstr);
	}
	else if (strstr (pstr, "Z模糊匹配=")) {
	    pstr += 10;
	    bWBUseZ = atoi (pstr);
	}
	else if (strstr (pstr, "五笔精确匹配=")) {
	    pstr += 13;
	    bWBExactMatch = atoi (pstr);
	}
	else if (strstr (pstr, "提示五笔编码=")) {
	    pstr += 13;
	    bPromptWBCode = atoi (pstr);
	}
	else if (strstr (pstr, "增加五笔词组=")) {
	    pstr += 13;
	    SetHotKey (pstr, hkWBAddPhrase);
	}
	else if (strstr (pstr, "调整五笔顺序=")) {
	    pstr += 13;
	    SetHotKey (pstr, hkWBAdjustOrder);
	}
	else if (strstr (pstr, "删除五笔词组=")) {
	    pstr += 13;
	    SetHotKey (pstr, hkWBDelPhrase);
	}
	
	else if (strstr (pstr, "向前翻页=")) {
	    pstr += 9;
	    SetHotKey (pstr, hkERBIPrevPage);
	}
	else if (strstr (pstr, "向后翻页=")) {
	    pstr += 9;
	    SetHotKey (pstr, hkERBINextPage);
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
    fprintf (fp, "#区域设置=zh_CN\n");
    fprintf (fp, "显示字体=*\n");
    fprintf (fp, "显示字体大小=18\n");
#ifdef _USE_XFT
    fprintf (fp, "是否使用AA字体=0\n");
#endif
    fprintf (fp, "是否使用Ctrl_Shift打开输入法=1\n");

    fprintf (fp, "\n[输出]\n");
    fprintf (fp, "数字后跟半角符号=1\n");
    fprintf (fp, "Enter键行为=2\n");
    fprintf (fp, "分号输入英文=1\n");
    fprintf (fp, "大写字母输入英文=1\n");
    fprintf (fp, "联想方式禁止翻页=1\n");

    fprintf (fp, "\n[界面]\n");
    fprintf (fp, "候选词个数=5\n");
    fprintf (fp, "是否使用3D界面=2\n");
    fprintf (fp, "是否自动隐藏输入条=1\n");
    fprintf (fp, "主窗口隐藏模式=1\n");

    fprintf (fp, "是否光标跟随=1\n");

    fprintf (fp, "光标色=92 210 131\n");
    fprintf (fp, "主窗口背景色=230 230 230\n");
    fprintf (fp, "主窗口线条色=255 0 0\n");
    fprintf (fp, "输入窗背景色=240 240 240\n");
    fprintf (fp, "输入窗线条色=100 200 255\n");
    fprintf (fp, "输入窗箭头色=255 150 255\n");

    fprintf (fp, "输入窗用户输入色=0 0 255\n");
    fprintf (fp, "输入窗提示色=255 0 0\n");
    fprintf (fp, "输入窗序号色=200 0 0\n");
    fprintf (fp, "输入窗第一个候选字色=0 150 100\n");
    fprintf (fp, "#该颜色值只用于拼音中的用户自造词\n");
    fprintf (fp, "输入窗用户词组色=0 0 255\n");
    fprintf (fp, "输入窗提示编码色=100 100 255\n");
    fprintf (fp, "#五笔、拼音的单字/系统词组均使用该颜色\n");
    fprintf (fp, "输入窗其它文本色=0 0 0\n");

    fprintf (fp, "\n[热键]\n");
    fprintf (fp, "#中英文快速切换键 可以设置为L_CTRL R_CTRL L_SHIFT R_SHIFT\n");
    fprintf (fp, "中英文快速切换键=L_CTRL\n");
    fprintf (fp, "GBK支持=CTRL_M\n");
    fprintf (fp, "联想支持=CTRL_L\n");
    fprintf (fp, "全半角=SHIFT_SPACE\n");
    fprintf (fp, "中文标点=ALT_SPACE\n");
    fprintf (fp, "上一页=- ,\n");
    fprintf (fp, "下一页== .\n");
    fprintf (fp, "第二三候选词选择键=SHIFT\n");

    fprintf (fp, "\n[五笔]\n");
    fprintf (fp, "五笔四键自动上屏=1\n");
    fprintf (fp, "自动调整五笔顺序=0\n");
    fprintf (fp, "提示词库中已有的词组=0\n");
    fprintf (fp, "五笔精确匹配=0\n");
    fprintf (fp, "提示五笔编码=1\n");
    fprintf (fp, "使用Z输入拼音=%d\n", bUseZPY);
    fprintf (fp, "Z模糊匹配=%d\n", bWBUseZ);
    fprintf (fp, "增加五笔词组=CTRL_8\n");
    fprintf (fp, "调整五笔顺序=CTRL_6\n");
    fprintf (fp, "删除五笔词组=CTRL_7\n");

	fprintf (fp, "\n[二笔]\n");
    fprintf (fp, "向前翻页=[\n");
    fprintf (fp, "向后翻页=]\n");

    fprintf (fp, "\n[拼音]\n");
    fprintf (fp, "使用全拼=0\n");
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
		    iInputWindowX = DisplayWidth (dpy, iScreen) - iInputWindowWidth;
	    }
	    else if (strstr (str, "输入窗口位置Y=")) {
		pstr += 14;
		iInputWindowY = atoi (pstr);
		if (iInputWindowY < 0)
		    iInputWindowY = 0;
		else if ((iInputWindowY + INPUTWND_HEIGHT) > DisplayHeight (dpy, iScreen))
		    iInputWindowY = DisplayHeight (dpy, iScreen) - INPUTWND_HEIGHT;
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
	    else if (strstr (str, "是否联想=")) {
		pstr += 9;
		bUseLegend = atoi (pstr);
	    }
	    else if (strstr (str, "当前输入法=")) {
		pstr += 11;
		imeIndex = atoi (pstr);
	    }
	    else if (strstr (str, "是否使用双拼=")) {
		pstr += 13;
		bSP = atoi (pstr);
		if (bSP)
		    LoadSPData ();
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
    fprintf (fp, "是否全角=%d\n", (bCorner) ? 1 : 0);
    fprintf (fp, "是否中文标点=%d\n", (bChnPunc) ? 1 : 0);
    fprintf (fp, "是否GBK=%d\n", (bUseGBK) ? 1 : 0);
    fprintf (fp, "是否联想=%d\n", (bUseLegend) ? 1 : 0);
    fprintf (fp, "当前输入法=%d\n", imeIndex);
    fprintf (fp, "是否使用双拼=%d\n", bSP);

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

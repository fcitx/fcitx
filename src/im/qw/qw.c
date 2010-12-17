/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
 * 区位的算法来自于rfinput-2.0
 */
#include <string.h>
#include <iconv.h>

#include "core/fcitx.h"
#include "core/keys.h"

#include "im/qw/qw.h"
#include "ui/InputWindow.h"
#include "tools/configfile.h"

extern char     strCodeInput[];
extern int      iCodeInputCount;
extern int      iCandWordCount;
extern int      iCandPageCount;
extern int	iCurrentCandPage;
extern char     strStringGet[];
    
char     strQWHZ[3];
char     strQWHZUTF8[UTF8_MAX_LENGTH + 1];

INPUT_RETURN_VALUE DoQWInput(unsigned int sym, unsigned int state, int keyCount)
{
    INPUT_RETURN_VALUE retVal;

    retVal = IRV_TO_PROCESS;
    if (IsHotKeyDigit(sym, state)) {
        if ( iCodeInputCount!= 4 ) {
            strCodeInput[iCodeInputCount++]=sym;
            strCodeInput[iCodeInputCount]='\0';
            if ( iCodeInputCount==4 ) {
            strcpy(strStringGet, QWGetCandWord(sym-'0'-1));
            retVal= IRV_GET_CANDWORDS;
         }
         else if (iCodeInputCount==3)
            retVal=QWGetCandWords(SM_FIRST);
         else
            retVal=IRV_DISPLAY_CANDWORDS;
      }
   }
   else if (IsHotKey(sym, state, FCITX_BACKSPACE)) {
      if (!iCodeInputCount)
          return IRV_DONOT_PROCESS_CLEAN;
      iCodeInputCount--;
      strCodeInput[iCodeInputCount] = '\0';

      if (!iCodeInputCount)
          retVal = IRV_CLEAN;
      else {
          iCandPageCount = 0;
          SetMessageCount(&messageDown, 0);
          retVal = IRV_DISPLAY_CANDWORDS;
      }
   }
   else if (IsHotKey(sym, state, FCITX_SPACE)) {
      if (!iCodeInputCount)
          return IRV_TO_PROCESS;
      if (iCodeInputCount!=3)
          return IRV_DO_NOTHING;
          
      strcpy(strStringGet, QWGetCandWord(0));
      retVal= IRV_GET_CANDWORDS;
   }
   else
      return IRV_TO_PROCESS;
   
    SetMessageCount(&messageUp, 0);
    AddMessageAtLast(&messageUp, MSG_INPUT, "%s", strCodeInput);
   if ( iCodeInputCount!=3 )
      SetMessageCount(&messageDown, 0);
      
   return retVal;
}

char *QWGetCandWord (int iIndex)
{
   if ( !iCandPageCount )
      return NULL;
      
   SetMessageCount(&messageDown, 0);
   if ( iIndex==-1 )
      iIndex=9;
   return GetQuWei((strCodeInput[0] - '0') * 10 + strCodeInput[1] - '0',iCurrentCandPage * 10+iIndex+1);
}

INPUT_RETURN_VALUE QWGetCandWords (SEARCH_MODE mode)
{
    int             iQu, iWei;
    int             i;
    char            strTemp[3];

    if ( fc.bPointAfterNumber ) {
       strTemp[1] = '.';
       strTemp[2] = '\0';
    }
    else
       strTemp[1]='\0';

    iQu = (strCodeInput[0] - '0') * 10 + strCodeInput[1] - '0';
    
    if (mode==SM_FIRST ) {
       iCandPageCount = 9;
   iCurrentCandPage = strCodeInput[2]-'0';
    }
    else {
       if ( !iCandPageCount )
      return IRV_TO_PROCESS;
       if (mode==SM_NEXT) {
          if ( iCurrentCandPage!=iCandPageCount )
         iCurrentCandPage++;
   }
   else {
          if ( iCurrentCandPage )
         iCurrentCandPage--;
   }
    }
   
    iWei = iCurrentCandPage * 10;

    SetMessageCount(&messageDown, 0);
    for (i = 0; i < 10; i++) {
   strTemp[0] = i + 1 + '0';
   if (i == 9)
      strTemp[0] = '0';
    AddMessageAtLast(&messageDown, MSG_INDEX, "%s", strTemp);
    AddMessageAtLast(&messageDown, (i)? MSG_OTHER:MSG_FIRSTCAND, "%s", GetQuWei (iQu, iWei + i + 1));
   if (i != 9)
        MessageConcatLast(&messageDown, " ");
    }    
    
    strCodeInput[2]=iCurrentCandPage+'0';
    SetMessageCount(&messageUp, 0);
    AddMessageAtLast(&messageUp, MSG_INPUT, "%s", strCodeInput);
    
    return IRV_DISPLAY_CANDWORDS;
}

char           *GetQuWei (int iQu, int iWei)
{

   char *inbuf, *outbuf;

   size_t insize = 2, avail = UTF8_MAX_LENGTH + 1;

   iconv_t convGBK = iconv_open("utf-8", "gb18030");

    if (iQu >= 95) {      /* Process extend Qu 95 and 96 */
   strQWHZ[0] = iQu - 95 + 0xA8;
   strQWHZ[1] = iWei + 0x40;

   /* skip 0xa87f and 0xa97f */
   if ((unsigned char) strQWHZ[1] >= 0x7f)
       strQWHZ[1]++;
    }
    else {
   strQWHZ[0] = iQu + 0xa0;
   strQWHZ[1] = iWei + 0xa0;
    }

    strQWHZ[2] = '\0';

   inbuf = strQWHZ;

   outbuf = strQWHZUTF8;

   iconv(convGBK, &inbuf, &insize, &outbuf, &avail);

   iconv_close(convGBK);

    return strQWHZUTF8;
}

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pyTools.h"

#define INT8 int8_t

void LoadPYMB(FILE *fi, struct _PYMB **pPYMB, int isUser)
{
  struct _PYMB *PYMB;
  int i, j, r, n, t;

  /* Is there a way to avoid reading the whole file twice? */

  /* First Pass: Determine the size of the PYMB array to be created */

  n = 0;
  while (1)
  {
      INT8 clen;
      r = fread(&t, sizeof (int), 1, fi);
      if (!r)
          break;
      ++n;
      
      fread(&clen, sizeof (INT8), 1, fi);
      fseek(fi, sizeof (char) * clen, SEEK_CUR);
      fread(&t, sizeof(int), 1, fi);
      
      for (i = 0; i < t; ++i)
      {
          int iLen;
          fread(&iLen, sizeof(int), 1, fi);
          fseek(fi , sizeof(char) * iLen, SEEK_CUR);
          fread(&iLen, sizeof(int), 1, fi);
          fseek(fi , sizeof(char) * iLen, SEEK_CUR);
          fread(&iLen, sizeof(int), 1, fi);
          if (isUser)
              fread(&iLen, sizeof(int), 1, fi);
      }
  }

  /* Second Pass: Actually read the data */

  fseek(fi, 0, SEEK_SET);

  *pPYMB = PYMB = malloc(sizeof (*PYMB) * (n + 1));

  for (i = 0; i < n; ++i)
  {
    r = fread(&(PYMB[i].PYFAIndex), sizeof (int), 1, fi);

    INT8 clen;
    fread(&clen, sizeof (INT8), 1, fi);
    fread(PYMB[i].HZ, sizeof (char) * clen, 1, fi);
    PYMB[i].HZ[clen] = '\0';

    fread(&(PYMB[i].UserPhraseCount), sizeof (int), 1, fi);
    PYMB[i].UserPhrase = malloc(sizeof(*(PYMB[i].UserPhrase)) * PYMB[i].UserPhraseCount);

#define PU(i,j) (PYMB[(i)].UserPhrase[(j)])
    for (j = 0; j < PYMB[i].UserPhraseCount; ++j)
    {
      fread(&(PU(i,j).Length), sizeof (int), 1, fi);

      PU(i,j).Map = malloc(sizeof (char) * PU(i,j).Length + 1);
      fread(PU(i,j).Map, sizeof (char) * PU(i,j).Length, 1, fi);
      PU(i,j).Map[PU(i,j).Length] = '\0';

      int iLen;
      fread(&iLen, sizeof (int), 1, fi);
      PU(i,j).Phrase = malloc(sizeof (char) * iLen + 1);
      fread(PU(i,j).Phrase, sizeof (char) * iLen, 1, fi);
      PU(i,j).Phrase[iLen] = '\0';

      fread(&(PU(i,j).Index), sizeof (int), 1, fi);

      if (isUser)
          fread(&(PU(i,j).Hit), sizeof (int), 1, fi);
      else
          PU(i,j).Hit = 0;
    }
#undef PU
  }
  PYMB[n].HZ[0] = '\0';

  return;
}

int LoadPYBase(FILE *fi, struct _HZMap **pHZMap)
{
  int i, j, r, PYFACount;
  struct _HZMap *HZMap;

  r = fread(&PYFACount, sizeof (int), 1, fi);
  if (!r)
    return 0;

  *pHZMap = HZMap = malloc(sizeof (*HZMap) * (PYFACount + 1));
  for (i = 0; i < PYFACount; ++i)
  {
    fread(HZMap[i].Map, sizeof(char) * 2, 1, fi);
    HZMap[i].Map[2] = '\0';

    fread(&(HZMap[i].BaseCount), sizeof (int), 1, fi);
    HZMap[i].HZ = malloc(sizeof(char *) * HZMap[i].BaseCount);
    HZMap[i].Index = malloc(sizeof (int) * HZMap[i].BaseCount);

    for (j = 0; j < HZMap[i].BaseCount; ++j)
    {
      INT8 clen;
      fread(&clen, sizeof(INT8), 1, fi);
      HZMap[i].HZ[j] = malloc(sizeof(char) *( clen + 1));
      fread(HZMap[i].HZ[j], sizeof(char) * clen, 1, fi);
      HZMap[i].HZ[j][clen] = '\0';
      fread(&HZMap[i].Index[j], sizeof (int), 1, fi);
    }
  }
  HZMap[i].Map[0] = '\0';

  return PYFACount;
}

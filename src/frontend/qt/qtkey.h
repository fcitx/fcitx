/*
 * Copyright (C) 2017~2020 by CSSlayer
 * wengxt@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above Copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above Copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */
#ifndef _PLATFORMINPUTCONTEXT_QTKEY_H_
#define _PLATFORMINPUTCONTEXT_QTKEY_H_

#include <QString>

int keysymToQtKey(uint32_t keysym, const QString &text);

#endif // _PLATFORMINPUTCONTEXT_QTKEY_H_

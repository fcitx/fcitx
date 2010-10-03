/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdlib.h>
#include "fcitx-config/sprintf.h"

#ifndef HAVE_VASPRINTF
int vasprintf(char **ptr, const char *format, va_list ap)
{
    int ret;
    va_list ap2;
    
    va_copy(ap2, ap);
    
    ret = vsnprintf(NULL, 0, format, ap2);
    if (ret <= 0) return ret;
    
    (*ptr) = (char *)malloc(ret+1);
    if (!*ptr) return -1;
    
    va_copy(ap2, ap);
    
    ret = vsnprintf(*ptr, ret+1, format, ap2);
    
    return ret;
}
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char **ptr, const char *format, ...)
{
    va_list ap;
    int ret;
    
    *ptr = NULL;
    va_start(ap, format);
    ret = vasprintf(ptr, format, ap);
    va_end(ap);
    
    return ret;
}
#endif


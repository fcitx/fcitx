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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

/**
 * @addtogroup FcitxUtils
 * @{
 */

/**
 * @file log.h
 *
 * Fcitx provides a simply log function in order to record usually message for developers.
 *
 * A common use case for log function will like this:
 *
 * @code
 * FcitxLog(INFO, "Log for %s", str);
 * @endcode
 *
 * FcitxLog uses printf style argument, please make sure you don't use format string with some
 * unsafe string, for example, contains percent symbol.
 *
 */
#ifndef _FCITX_LOG_H_
#define _FCITX_LOG_H_

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief ErrorLevel, DEBUG will not be printed if -DENABLE_DEBUG=On is not specified while compiling
     **/
    typedef enum _FcitxLogLevel
    {
        FCITX_DEBUG = 0,
        FCITX_ERROR = 1,
        FCITX_INFO = 2,
        FCITX_FATAL = 3,
        FCITX_WARNING = 4,
        FCITX_NONE = 5
    } FcitxLogLevel, ErrorLevel;

/**
 * a convenient macro for log, it will automatically printed current file and code line number.
 */
#define FcitxLog(e, fmt...) FcitxLogFunc(FCITX_##e, __FILE__, __LINE__, fmt)

    /**
     * print a log string to stderr, should use FcitxLog instead of this function
     *
     * @param level log level
     * @param filename current filename
     * @param line line number
     * @param fmt printf fmt
     * @param  ...
     * @return void
     **/
    void FcitxLogFunc(FcitxLogLevel level, const char* filename,
                      const int line, const char* fmt, ...);

    /**
     * print a log string to stderr, may used to hook into other log system.
     *
     * @param level log level
     * @param filename current filename
     * @param line line number
     * @param fmt printf fmt
     * @param ap va_list
     * @return void
     *
     * @since 4.2.7
     **/
    void FcitxLogFuncV(FcitxLogLevel e, const char* filename, const int line,
                       const char* fmt, va_list ap);

    /**
     * set log level
     *
     * @param e log level
     * @return void
     *
     * @since 4.2.7
     **/
    void FcitxLogSetLevel(FcitxLogLevel e);

    /**
     * get log level
     *
     * @return FcitxLogLevel
     **/
    FcitxLogLevel FcitxLogGetLevel();

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */

// kate: indent-mode cstyle; space-indent on; indent-width 0;

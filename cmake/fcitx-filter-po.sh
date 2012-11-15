#!/bin/sh
#   Copyright (C) 2012~2012 by Yichao Yu
#   yyc1992@gmail.com
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

cur_msgid="${MSGEXEC_MSGID}"
cur_msgstr="$(cat)"
var_prefix="${1}"

if [ -z "$cur_msgid" ] || [ -z "$cur_msgstr" ]; then
    exit 0
fi

. "$(dirname "$(realpath -es $0)")/fcitx-parse-po.sh"

varname="$(fcitx_msgid_to_varname "${var_prefix}" "${cur_msgid}")"
varbase64="$(echo -n "${cur_msgstr}" | base64 -w0)"
echo "${varname}=${varbase64}"

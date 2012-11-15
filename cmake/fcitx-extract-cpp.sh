#!/bin/bash
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

action="$1"

shift 1 || exit 1

. "$(dirname "${BASH_SOURCE}")/fcitx-write-po.sh"

case "${action}" in
    -c)
        in_file="${1}"
        if fcitx_str_match "*.c" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.h" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.cpp" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.cxx" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.C" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.cc" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.hh" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.H" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.hxx" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.hpp" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.sh" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.bash" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.csh" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.zsh" "${in_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        [[ -z "$*" ]] && exit 1
        echo "Extracting po string from c sources."
        xgettext -o "${out_file}" --from-code=utf-8 \
            --force-po --kde -k_ -kN_ -kD_:2 \
            -kgettext -kngettext:1,2 -kdgettext:2 \
            -kdcgettext:2 -kdcngettext:2,3  -kdngettext:2,3 \
            -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 \
            -ktr2i18n:1 -kI18N_NOOP:1 -kI18N_NOOP2:1c,2 -kaliasLocale \
            -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 "$@"
        fcitx_fix_po_charset_utf8 "${out_file}"
        exit 0
        ;;
esac

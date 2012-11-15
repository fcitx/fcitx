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

filter_src="$(dirname "$(realpath -es $0)")/fcitx-filter-po.sh"

fcitx_quote () {
    local quoted=${1//\'/\'\\\'\'};
    printf "'%s'" "$quoted"
}

fcitx_str_match() {
    local pattern="$1"
    local string="$2"
    case "${string}" in
        $pattern)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

fcitx_exts_match() {
    local in_file="$1"
    local ext
    shift
    for ext in "${@}"; do
        fcitx_str_match "*.${ext}" "${in_file}" && return 0
    done
    return 1
}

fcitx_msgid_to_varname() {
    local prefix="${1}"
    local msgid="${2}"
    echo -n "${prefix}___$(echo -n "${msgid}" | base64 -w0 | sed -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g')"
}

fcitx_parse_po_file() {
    local prefix="${1}"
    local pofile="${2}"
    local parse_res="${3}"
    msgattrib -o- --translated --no-fuzzy --no-obsolete --force-po "${pofile}" |
    msgconv -tutf-8 --force-po |
    msgexec -i- "${filter_src}" "${prefix}" > "${parse_res}"
}

fcitx_find_str() {
    local prefix="${1}"
    local msgid="${2}"
    local varname="$(fcitx_msgid_to_varname "${prefix}" "${msgid}")"
    eval "echo -n \"\${${varname}}\"" | base64 -d
}

all_po_langs=''

fcitx_lang_to_prefix() {
    local lang="${1}"
    echo -n "${lang}" | base64 -w0 | sed -e 's:_:_0:g' \
        -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g' -e 's:\([A-Z]\):_\L\1:g'
}

fcitx_load_all_pos() {
    local po_cache="$1"
    local po_parse_cache_dir="$2"
    mkdir -p "${po_parse_cache_dir}"
    local po_lang
    local po_file
    local prefix
    local po_parse_cache_file
    local line
    while read line; do
        po_lang="${line%% *}"
        po_file="${line#* }"
        prefix="$(fcitx_lang_to_prefix "${po_lang}")"
        po_parse_cache_file="${po_parse_cache_dir}/${prefix}.fxpo"
        if [ "${po_parse_cache_file}" -ot "${po_file}" ] ||
            ! [ -f "${po_parse_cache_file}" ]; then
            echo "Parsing po file: ${po_file}"
            local unique_fname="${po_parse_cache_file}.$$"
            fcitx_parse_po_file "${prefix}" "${po_file}" "${unique_fname}"
            . "${unique_fname}"
            mv "${unique_fname}" "${po_parse_cache_file}"
            echo "Finished parsing po file: ${po_file}"
        else
            . "${po_parse_cache_file}"
        fi
        all_po_langs="${all_po_langs} ${po_lang}"
    done <<EOF
$(cat "${po_cache}")
EOF
}

fcitx_find_str_for_lang() {
    local lang="${1}"
    local msgid="${2}"
    local prefix="$(fcitx_lang_to_prefix "${lang}")"
    fcitx_find_str "${prefix}" "${msgid}"
}

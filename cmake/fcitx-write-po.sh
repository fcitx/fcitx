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

. "${_FCITX_MACRO_CMAKE_DIR}/fcitx-parse-po.sh"

__fcitx_pot_header_format='Project-Id-Version: PACKAGE VERSION
Report-Msgid-Bugs-To: %s
POT-Creation-Date: %s
PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE
Last-Translator: FULL NAME <EMAIL@ADDRESS>
Language-Team: LANGUAGE <LL@li.org>
Language: LANG
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
'
__fcitx_pot_header_quote_format='# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'\''S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\\n"
"Report-Msgid-Bugs-To: %s\\n"
"POT-Creation-Date: %s\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"Language: LANG\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=utf-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
'

fcitx_write_po_header() {
    local out_file="$1"
    local bug_addr="$2"
    if [ -z "${bug_addr}" ]; then
        bug_addr='EMAIL@ADDRESS'
    fi
    printf "${__fcitx_pot_header_quote_format}" \
        "${bug_addr}" "$(date +'%Y-%m-%d %H:%M%z')" > "${out_file}"
}

all_msg_id=()

fcitx_write_all_po() {
    local out_file="$1"
    local var_name
    local places
    for msgid in "${all_msg_id[@]}"; do
        var_name="$(fcitx_msgid_to_varname MSGID "${msgid}")"
        eval "places=(\"\${${var_name}[@]}\")"
        echo
        for place in "${places[@]}"; do
            echo "#: ${place}"
        done
        msgid=${msgid//\"/\\\"}
        echo 'msgid "'"${msgid}"'"'
        echo 'msgstr ""'
    done >> "${out_file}"
}

fcitx_record_po_msg() {
    local in_file="$1"
    local msgid="$2"
    local line_num="$3"
    if [[ -z "${msgid}" ]]; then
        continue
    fi
    local var_name="$(fcitx_msgid_to_varname MSGID "${msgid}")"
    local tmp
    eval "tmp=\"\${${var_name}[*]}\""
    if [[ -z "${tmp}" ]]; then
        all_msg_id=("${all_msg_id[@]}" "${msgid}")
    fi
    eval "${var_name}=(\"\${${var_name}[@]}\" \"\${in_file}:\${line_num}\")"
}

fcitx_po_set_header() {
    local header="$1"
    local po_file="$2"
    local content
    content=$("${_FCITX_PO_PARSER_EXECUTABLE}" \
        --fix-header "${header}" "${po_file}")
    printf '%s\n' "${content}" > "${po_file}"
}

fcitx_merge_all_pos() {
    local bug_addr="$1"
    local pot_file="$2"
    shift 2
    local po_list=("$@")
    local header
    local p_file
    if [[ -z "${po_list[*]}" ]]; then
        fcitx_write_po_header "${pot_file}" "${bug_addr}"
    else
        header=$(printf "${__fcitx_pot_header_format}" \
            "${bug_addr}" "$(date +'%Y-%m-%d %H:%M%z')")
        header="${header}"$'\n'
        for p_file in "${po_list[@]}"; do
            fcitx_po_set_header "${header}" "${p_file}"
        done
        msgcat -o- --to-code=utf-8 "${po_list[@]}" | \
            "${_FCITX_PO_PARSER_EXECUTABLE}" \
            --fix-header "${header}" | {
            cat <<EOF
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
EOF
            sed -e '/^# /d' -e '/^#$/d'
        } > "${pot_file}"
    fi
}

fcitx_filter_array() {
    local IFS=$'\n'
    eval "$1"'=($(for ele in "${'"$1"'[@]}"; do echo "${ele}"; done | "${@:2}"))'
}

fcitx_generate_pot() {
    local bug_addr="${1}"
    local type_list=()
    while read line; do
        [ -z "${line}" ] && continue
        type="${line%% *}"
        file="${line#* }"
        type_list=("${type_list[@]}" "${type}")
        eval '__ex_file_'"${type}"'=("${__ex_file_'"${type}"'[@]}" "${file}")'
    done <<EOF
$(cat "${src_cache}")
EOF
    po_dir="${cache_base}/sub_pos"
    mkdir -p "${po_dir}"
    fcitx_filter_array type_list sort -u
    for type in "${type_list[@]}"; do
        fcitx_filter_array "__ex_file_${type}" sort -u
    done
    local handler_list=()
    while read line; do
        [ -z "${line}" ] && continue
        type="${line%% *}"
        handler="${line#* }"
        handler_list=("${handler_list[@]}" "${handler}")
        eval __ex_handler_"${type}"='${handler}'
    done <<EOF
$(cat "${handler_cache}")
EOF

    # Do not sort handler. The order these handlers are added should
    # be more stable than the path name of them.
    fcitx_filter_array handler_list uniq
    po_list=()
    po_num=0
    for type in "${type_list[@]}"; do
        eval 'file_list=("${__ex_file_'"${type}"'[@]}")'
        if [ "${type}" = "generic" ]; then
            echo "Classifying Files to be translated..."
            for handler in "${handler_list[@]}"; do
                file_left=()
                handled_list=()
                for file in "${file_list[@]}"; do
                    if "${handler}" -c "${file}" &> /dev/null; then
                        handled_list=("${handled_list[@]}" "${file}")
                    else
                        file_left=("${file_left[@]}" "${file}")
                    fi
                done
                file_list=("${file_left[@]}")
                ((po_num++))
                po_filename="${po_dir}/subpo_${po_num}.po"
                "${handler}" -w "${po_filename}" "${handled_list[@]}" && {
                    po_list=("${po_list[@]}" "${po_filename}")
                }
            done
            if ! [[ -z "${file_list[*]}" ]]; then
                echo "Warning: Following files are added but not handled."
                for extra_file in "${file_list[@]}"; do
                    echo $'\t'"${extra_file}"
                done
            fi
        else
            echo "Extracting strings from ${type} files."
            eval handler='"${__ex_handler_'"${type}"'}"'
            [ -z "${handler}" ] && {
                echo "Cannot find handler for type ${type}." >&2
                continue
            }
            ((po_num++))
            po_filename="${po_dir}/subpo_${po_num}.po"
            "${handler}" -w "${po_filename}" "${file_list[@]}" && {
                po_list=("${po_list[@]}" "${po_filename}")
            }
        fi
    done
    echo "Merging sub po files..."
    fcitx_merge_all_pos "${bug_addr}" "${pot_file}" "${po_list[@]}"
    while read line; do
        [ -z "${line}" ] && continue
        po_lang="${line%% *}"
        po_file="${line#* }"
        echo "Updating ${po_file} ..."
        msgmerge --quiet --update --backup=none -s --lang="${po_lang}" \
            "${po_file}" "${pot_file}"
    done <<EOF
$(cat "${po_cache}")
EOF
}

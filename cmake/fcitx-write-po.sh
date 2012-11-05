#!/bin/bash

. "$(dirname ${BASH_SOURCE})/fcitx-parse-po.sh"

write_po_header() {
    local out_file="$1"
    cat > "${out_file}"  <<EOF
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"Report-Msgid-Bugs-To: fcitx-dev@googlegroups.com\n"
"POT-Creation-Date: 2010-11-17 11:48+0800\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
EOF
}

all_msg_id=()

write_all_po() {
    local out_file="$1"
    local var_name
    local places
    for msgid in "${all_msg_id[@]}"; do
        var_name="$(msgid_to_varname MSGID "${msgid}")"
        eval "places=(\"\${${var_name}[@]}\")"
        echo
        for place in "${places[@]}"; do
            echo "#: ${place}"
        done
        echo -n 'msgid "'
        echo -n "${msgid}" | sed -e 's/"/\\"/g'
        echo '"'
        echo 'msgstr ""'
    done >> "${out_file}"
}

record_po_msg() {
    local in_file="$1"
    local msgid="$2"
    local line_num="$3"
    if [[ -z "${msgid}" ]]; then
        continue
    fi
    local var_name="$(msgid_to_varname MSGID "${msgid}")"
    local tmp
    eval "tmp=\"\${${var_name}[*]}\""
    if [[ -z "${tmp}" ]]; then
        all_msg_id=("${all_msg_id[@]}" "${msgid}")
    fi
    eval "${var_name}=(\"\${${var_name}[@]}\" \"\${in_file}:\${line_num}\")"
}

fix_po_charset_utf8() {
    local po_file="$1"
    local regex='^[[:blank:]]*"Content-Type:.*; charset=.*\\n"[[:blank:]]*$'
    local fix_res='"Content-Type: text/plain; charset=utf-8\\n"'
    sed -i "${po_file}" -e "0,/${regex}/s|${regex}|${fix_res}|"
}

merge_all_pos() {
    local pot_file="$1"
    shift
    local po_list=("$@")
    if [[ -z "${po_list[*]}" ]]; then
        write_po_header "${pot_file}"
    else
        msgcat -o "${pot_file}" --use-first --to-code=utf-8 "${po_list[@]}"
    fi
}

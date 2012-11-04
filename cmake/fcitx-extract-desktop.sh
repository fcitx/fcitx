#!/bin/bash

action="$1"

shift 1 || exit 1

str_match() {
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

write_header() {
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

msgid_to_varname() {
    local prefix="${1}"
    local msgid="${2}"
    echo -n "${prefix}_$(echo -n "${msgid}" | base64 -w0 | sed -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g')"
}

all_msg_id=()

write_all() {
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

record_msg() {
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

extract_subconfig() {
    local IFS=','
    local sub_config="$1"
    local in_file="$2"
    local line_num="$3"
    for section in ${sub_config}; do
        [[ "${section}" =~ ^([^:]+):(.*)$ ]] || continue
        [[ "${BASH_REMATCH[2]}" == "domain" ]] && continue
        msgid="${BASH_REMATCH[1]}"
        record_msg "${in_file}" "${msgid}" "${line_num}"
    done
}

extract_desktop() {
    local in_file="$1"
    local out_file="$2"
    local line_num=0
    while read line; do
        ((line_num++))
        case "${line}" in
            _*=*)
                line="${line#_}"
                key="${line%%=*}"
                msgid="${line#*=}"
                record_msg "${in_file}" "${msgid}" "${line_num}"
                ;;
            SubConfig=*)
                key="${line%%=*}"
                sub_config="${line#*=}"
                extract_subconfig "${sub_config}" "${in_file}" "${line_num}"
                ;;
        esac
    done < "${in_file}" >> "${out_file}"
}

case "${action}" in
    -c)
        in_file="${1}"
        if str_match "*.conf.in" "${in_file}"; then
            exit 0
        elif str_match "*.desktop.in" "${in_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        write_header "${out_file}"
        for rel_path in "$@"; do
            echo "Extracting po string from ${rel_path}"
            extract_desktop "${rel_path}" "${out_file}"
        done
        write_all "${out_file}"
        exit 0
        ;;
esac

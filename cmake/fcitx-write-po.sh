. "$(dirname "${BASH_SOURCE}")/fcitx-parse-po.sh"

fcitx_write_po_header() {
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
        echo -n 'msgid "'
        echo -n "${msgid}" | sed -e 's/"/\\"/g'
        echo '"'
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

fcitx_fix_po_charset_utf8() {
    local po_file="$1"
    local regex='^[[:blank:]]*"Content-Type:.*; charset=.*\\n"[[:blank:]]*$'
    local fix_res='"Content-Type: text/plain; charset=utf-8\\n"'
    sed -i "${po_file}" -e "0,/${regex}/s|${regex}|${fix_res}|"
}

fcitx_merge_all_pos() {
    local pot_file="$1"
    shift
    local po_list=("$@")
    if [[ -z "${po_list[*]}" ]]; then
        fcitx_write_po_header "${pot_file}"
    else
        msgcat -o "${pot_file}" --use-first --to-code=utf-8 "${po_list[@]}"
    fi
}

fcitx_generate_pot() {
    file_list=()
    while read file; do
        file_list=("${file_list[@]}"
            "$(realpath -es --relative-to="${PWD}" "${file}")")
    done <<EOF
$(sort -u "${src_cache}")
EOF
    po_dir="${cache_base}/sub_pos"
    mkdir -p "${po_dir}"
    po_list=()
    po_num=0
    echo "Classifying Files to be translated..."
    while read handler; do
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
        if [[ -z "${handled_list[*]}" ]]; then
            continue
        fi
        let "po_num++"
        po_filename="${po_dir}/subpo_${po_num}.po"
        "${handler}" -w "${po_filename}" "${handled_list[@]}"
        po_list=("${po_list[@]}" "${po_filename}")
    done <<EOF
$(cat "${handler_cache}")
EOF

    if ! [[ -z "${file_list[*]}" ]]; then
        echo "Warning: Following files are added but not handled."
        for extra_file in "${file_list[@]}"; do
            echo $'\t'"${extra_file}"
        done
    fi
    echo "Merging sub po files..."
    fcitx_merge_all_pos "${pot_file}" "${po_list[@]}"
    while read line; do
        po_lang="${line%% *}"
        po_file="${line#* }"
        echo "Updating ${po_file} ..."
        msgmerge --quiet --update --backup=none -s --lang="${po_lang}" \
            "${po_file}" "${pot_file}"
    done <<EOF
$(cat "${po_cache}")
EOF
}

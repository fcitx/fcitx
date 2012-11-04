#!/bin/bash

cache_base="$1"
pot_file="$2"
action="$3"

shift 3 || exit 1

src_cache="${cache_base}/fcitx-translation-src-cache.txt"
po_cache="${cache_base}/fcitx-translation-po-cache.txt"
handler_cache="${cache_base}/fcitx-translation-handler-cache.txt"

add_sources() {
    realpath -es "$@" >> "${src_cache}" || exit 1
}

case "${action}" in
    --add-sources)
        add_sources "$@" || {
            rm -f "${src_cache}"
            exit 1
        }
        exit 0
        ;;
    --add-po)
        echo "$1 $2" >> "${po_cache}"
        exit 0
        ;;
    --add-handler)
        echo "${1}" >> "${handler_cache}"
        exit 0
        ;;
    --pot)
        file_list=()
        while read file; do
            file_list=("${file_list[@]}"
                "$(realpath -es --relative-to="${PWD}" "${file}")")
        done < <(sort -u "${src_cache}")
        po_dir="${cache_base}/sub_pos"
        mkdir -p "${po_dir}"
        po_list=()
        po_num=0
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
            let "po_num++"
            po_filename="${po_dir}/subpo_${po_num}.po"
            "${handler}" -w "${po_filename}" "${handled_list[@]}"
            po_list=("${po_list[@]}" "${po_filename}")
        done < "${handler_cache}"
        if ! [[ -z "${file_list[*]}" ]]; then
            echo "Warning: Following files are added but not handled."
            for extra_file in "${file_list[@]}"; do
                echo $'\t'"${extra_file}"
            done
        fi
        msgcat -o "${pot_file}" --use-first --to-code=utf-8 "${po_list[@]}"
        while read line; do
            po_lang="${line%% *}"
            po_file="${line#* }"
            msgmerge --quiet --update --backup=none -s --lang="${po_lang}" \
                "${po_file}" "${pot_file}"
        done < "${po_cache}"
        exit 0
        ;;
    --clean)
        rm -f "${src_cache}" "${po_cache}" "${handler_cache}"
        exit 0
        ;;
    *)
        exit 1
        ;;
esac

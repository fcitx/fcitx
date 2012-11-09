#!/bin/bash

cache_base="$1"
pot_file="$2"
action="$3"

shift 3 || exit 1

. "$(dirname ${BASH_SOURCE})/fcitx-write-po.sh"

src_cache="${cache_base}/sources-cache.txt"
po_cache="${cache_base}/pos-cache.txt"
handler_cache="${cache_base}/handlers-cache.txt"
parse_cache="${cache_base}/parse_po_cache"

add_sources() {
    realpath -es "$@" >> "${src_cache}" || exit 1
}

generate_pot() {
    file_list=()
    mapfile -t lines <<EOF
$(sort -u "${src_cache}")
EOF
    for file in "${lines[@]}"; do
        file_list=("${file_list[@]}"
            "$(realpath -es --relative-to="${PWD}" "${file}")")
    done
    po_dir="${cache_base}/sub_pos"
    mkdir -p "${po_dir}"
    po_list=()
    po_num=0
    mapfile -t lines < "${handler_cache}"
    echo "Classifying Files to be translated..."
    for handler in "${lines[@]}"; do
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
    done

    if ! [[ -z "${file_list[*]}" ]]; then
        echo "Warning: Following files are added but not handled."
        for extra_file in "${file_list[@]}"; do
            echo $'\t'"${extra_file}"
        done
    fi
    echo "Merging sub po files..."
    fcitx_merge_all_pos "${pot_file}" "${po_list[@]}"
    mapfile -t lines < "${po_cache}"
    for line in "${lines[@]}"; do
        po_lang="${line%% *}"
        po_file="${line#* }"
        echo "Updating ${po_file} ..."
        msgmerge --quiet --update --backup=none -s --lang="${po_lang}" \
            "${po_file}" "${pot_file}"
    done
}

case "${action}" in
    --check-apply-handler)
        # full_name may be invalid
        script="$1"
        in_file="$2"
        out_file="$3"
        "${script}" "${parse_cache}" "${po_cache}" -c "${in_file}" "${out_file}"
        exit $?
        ;;
    --apply-po-merge)
        # full_name may be invalid
        script="$1"
        in_file="$2"
        out_file="$3"
        "${script}" "${parse_cache}" "${po_cache}" -w "${in_file}" "${out_file}"
        exit $?
        ;;
    --parse-pos)
        # full_name may be invalid
        fcitx_load_all_pos "${po_cache}" "${parse_cache}"
        exit 0
        ;;
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
        generate_pot
        exit 0
        ;;
    --clean)
        rm -f "${src_cache}" "${po_cache}" "${handler_cache}"
        exit 0
        ;;
    --uninstall)
        mapfile -t installed_files < install_manifest.txt || exit 1
        for file in "${installed_files[@]}"; do
            file="${DESTDIR}/${file}"
            [[ -f "${file}" ]] || [[ -L "${file}" ]] || {
                echo "File: ${file}, doesn't exist or is not a regular file."
                continue
            }
            rm -v "${file}" || exit 1
        done
        exit 0
        ;;
    *)
        exit 1
        ;;
esac

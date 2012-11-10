#!/bin/sh

cache_base="$1"
pot_file="$2"
action="$3"

shift 3 || exit 1

. "$(dirname "$(realpath -es $0)")/fcitx-parse-po.sh"

src_cache="${cache_base}/sources-cache.txt"
po_cache="${cache_base}/pos-cache.txt"
handler_cache="${cache_base}/handlers-cache.txt"
parse_cache="${cache_base}/parse_po_cache"

add_sources() {
    realpath -es "$@" >> "${src_cache}" || exit 1
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
        . "$(dirname "${BASH_SOURCE}")/fcitx-write-po.sh"
        fcitx_generate_pot
        exit 0
        ;;
    --clean)
        rm -f "${src_cache}" "${po_cache}" "${handler_cache}"
        exit 0
        ;;
    --uninstall)
        while read file; do
            file="${DESTDIR}/${file}"
            [ -f "${file}" ] || [ -L "${file}" ] || {
                echo "File: ${file}, doesn't exist or is not a regular file."
                continue
            }
            rm -v "${file}" || exit 1
        done <<EOF
$(cat install_manifest.txt)
EOF
        exit 0
        ;;
    *)
        exit 1
        ;;
esac

#!/bin/bash

action="$1"

shift 1 || exit 1

. "$(dirname "${BASH_SOURCE}")/fcitx-write-po.sh"

case "${action}" in
    -c)
        in_file="${1}"
        if fcitx_str_match "*.ui" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.qml" "${in_file}"; then
            exit 0
        elif fcitx_str_match "*.qs" "${in_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        echo "Extracting po string from qt sources."
        # need to touch source dir here since lupdate will otherwise include
        # absolute path (or wrong relative path) in po files afaik.
        tempfile="$(mktemp --tmpdir=. --suffix=_fcitx_qt_$$.ts)"
        rm "${tempfile}"
        lupdate -locations relative -no-obsolete "$@" -ts "${tempfile}"
        lconvert -of po -o "${out_file}" -if ts -i "${tempfile}" \
            --drop-translations
        rm "${tempfile}"
        exit 0
        ;;
esac

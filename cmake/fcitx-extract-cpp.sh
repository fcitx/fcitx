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

case "${action}" in
    -c)
        in_file="${1}"
        if str_match "*.c" "${in_file}"; then
            exit 0
        elif str_match "*.h" "${in_file}"; then
            exit 0
        elif str_match "*.cpp" "${in_file}"; then
            exit 0
        elif str_match "*.cxx" "${in_file}"; then
            exit 0
        elif str_match "*.C" "${in_file}"; then
            exit 0
        elif str_match "*.cc" "${in_file}"; then
            exit 0
        elif str_match "*.hh" "${in_file}"; then
            exit 0
        elif str_match "*.H" "${in_file}"; then
            exit 0
        elif str_match "*.hxx" "${in_file}"; then
            exit 0
        elif str_match "*.hpp" "${in_file}"; then
            exit 0
        elif str_match "*.sh" "${in_file}"; then
            exit 0
        elif str_match "*.bash" "${in_file}"; then
            exit 0
        elif str_match "*.csh" "${in_file}"; then
            exit 0
        elif str_match "*.zsh" "${in_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        xgettext -o "${out_file}" --from-code=utf-8 --force-po \
            --keyword=_ --keyword=N_ --keyword=D_:2 --keyword=i18n \
            --keyword=gettext --keyword=ngettext:1,2 --keyword=dgettext:2 \
            --keyword=dcgettext:2 --keyword=dcngettext:2,3 \
            --keyword=dngettext:2,3 "$@"
        sed -i "${out_file}" -e '0,/^"Content-Type:.*; charset=.*\\n"$/s|^"Content-Type:.*; charset=.*\\n"$|"Content-Type: text/plain; charset=utf-8\\n"|'
        exit 0
        ;;
esac

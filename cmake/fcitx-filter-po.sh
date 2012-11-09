#!/bin/bash

cur_msgid="${MSGEXEC_MSGID}"
cur_msgstr="$(cat)"
var_prefix="${1}"

if [[ -z $cur_msgid ]] || [[ -z $cur_msgstr ]]; then
    exit 0
fi

. "$(dirname ${BASH_SOURCE})/fcitx-parse-po.sh"

varname="$(fcitx_msgid_to_varname "${var_prefix}" "${cur_msgid}")"
varbase64="$(echo -n "${cur_msgstr}" | base64 -w0)"
echo "${varname}=${varbase64}"

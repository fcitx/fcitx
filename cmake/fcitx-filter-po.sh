#!/bin/bash

cur_msgid="${MSGEXEC_MSGID}"
cur_msgstr="$(cat)"
var_prefix="${1}"

if [[ -z $cur_msgid ]] || [[ -z $cur_msgstr ]]; then
    exit 0
fi

. "$(dirname ${BASH_SOURCE})/fcitx-parse-po.sh"

varname="$(msgid_to_varname "${var_prefix}" "${cur_msgid}")"
varquote="$(quote "${cur_msgstr}")"
echo "${varname}=${varquote}"

#!/bin/bash

cur_msgid="${MSGEXEC_MSGID}"
cur_msgstr="$(cat)"
var_prefix="${1}"

if [[ -z $cur_msgid ]] || [[ -z $cur_msgstr ]]; then
    exit 0
fi

quote ()
{
    local quoted=${1//\'/\'\\\'\'};
    printf "'%s'" "$quoted"
}
msgid_to_varname() {
    local prefix="${1}"
    local msgid="${2}"
    echo -n "${prefix}_$(echo -n "${msgid}" | base64 -w0 | sed -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g')"
}

varname="$(msgid_to_varname "${var_prefix}" "${cur_msgid}")"
varquote="$(quote "${cur_msgstr}")"
echo "${varname}=${varquote}"

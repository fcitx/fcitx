#!/usr/bin/env bash

shopt -s extglob nullglob globstar
export TEXTDOMAIN=fcitx

__test_bash_unicode() {
    local magic_str='${1}'$'\xe4'$'\xb8'$'\x80'
    local magic_replace=${magic_str//\$\{/$'\n'$\{}
    ! [ "${magic_str}" = "${magic_replace}" ]
}

if type gettext &> /dev/null && __test_bash_unicode; then
    _() {
        gettext "$@"
    }
else
    _() {
        echo "$@"
    }
fi

#############################
# utility
#############################

add_and_check_file() {
    local prefix="$1"
    local file="$2"
    local inode
    inode="$(stat -L --printf='%i' "${file}" 2> /dev/null)" || return 0
    local varname="___add_and_check_file_${prefix}_${inode}"
    [ ! -z "${!varname}" ] && return 1
    eval "${varname}=1"
    return 0
}

print_array() {
    for ele in "$@"; do
        echo "${ele}"
    done
}

repeat_str() {
    local i
    local n="$1"
    local str="$2"
    local res=""
    for ((i = 0;i < n;i++)); do
        res="${res}${str}"
    done
    echo "${res}"
}

find_in_path() {
    local w="$1"
    local IFS=':'
    local p
    local f
    local fs
    for p in ${PATH}; do
        eval 'fs=("${p}/"'"${w}"')'
        for f in "${fs[@]}"; do
            echo "$f"
        done
    done
}

run_grep_fcitx() {
    "$@" | grep fcitx
}

get_config_dir() {
    local conf_option="$1"
    local default_name="$2"
    for path in "$(fcitx4-config "--${conf_option}" 2> /dev/null)" \
        "/usr/share/fcitx/${default_name}" \
        "/usr/local/share/fcitx/${default_name}"; do
        [ ! -z "${path}" ] && [ -d "${path}" ] && {
            echo "${path}"
            return 0
        }
    done
    return 1
}

get_from_config_file() {
    local file="$1"
    local key="$2"
    local value
    value=$(sed -ne "s=^${key}\=\(.*\)=\1=gp" "$file" 2> /dev/null)
    [ -z "$value" ] && return 1
    echo "${value}"
    return 0
}


#############################
# print
#############################

# tty and color
__istty=0

check_istty() {
    [ -t 1 ] && {
        __istty=1
    } || {
        __istty=0
    }
}

print_tty_ctrl() {
    ((__istty)) || return
    echo -ne '\e['"${1}"'m'
}

replace_reset() {
    local line
    local IFS=$'\n'
    if [ ! -z "$1" ]; then
        while read line; do
            echo "${line//$'\e'[0m/$'\e'[${1}m}"
        done
        [ -z "${line}" ] || {
            echo -n "${line//$'\e'[0m/$'\e'[${1}m}"
        }
    else
        cat
    fi
}

__replace_line() {
    local IFS=$'\n'
    local __line=${1//\$\{/$'\n'$\{}
    shift
    local __varname
    echo "${__line}" | while read __line; do
        if [[ ${__line} =~ ^\$\{([_a-zA-Z0-9]+)\} ]]; then
            __varname="${BASH_REMATCH[1]}"
            echo -n "${__line/\$\{${__varname}\}/${!__varname}}"
        else
            echo -n "${__line}"
        fi
    done
    echo
}

__replace_vars() {
    local IFS=$'\n'
    local __line
    while read __line; do
        __replace_line "${__line}" "$@"
    done
    [ -z "${__line}" ] || {
        echo -n "$(__replace_line "${__line}" "$@")"
    }
}

print_eval() {
    echo "$1" | __replace_vars "${@:2}"
}

# print inline
code_inline() {
    print_tty_ctrl '01;36'
    echo -n '`'"$1"'`' | replace_reset '01;36'
    print_tty_ctrl '0'
}

print_link() {
    local text="$1"
    local url="$2"
    print_tty_ctrl '01;33'
    echo -n "[$text]($url)" | replace_reset '01;33'
    print_tty_ctrl '0'
}

print_not_found() {
    print_eval "$(_ '${1} not found.')" "$(code_inline $1)"
}

# indent levels and list index counters
__current_level=0
__list_indexes=(0)

set_cur_level() {
    local level="$1"
    local indexes=()
    local i
    if ((level >= 0)); then
        ((__current_level = level))
        for ((i = 0;i <= __current_level;i++)); do
            ((indexes[i] = __list_indexes[i]))
        done
        __list_indexes=("${indexes[@]}")
    else
        ((__current_level = 0))
        __list_indexes=()
    fi
}

increase_cur_level() {
    local level="$1"
    ((level = __current_level + level))
    set_cur_level "$level"
}

# print blocks
__need_blank_line=0

write_paragraph() {
    local str="$1"
    local p1="$2"
    local p2="$3"
    local code="$4"
    local prefix="$(repeat_str "${__current_level}" "    ")"
    local line
    local i=0
    local whole_prefix
    local IFS=$'\n'
    ((__need_blank_line)) && echo
    [ -z "${code}" ] || print_tty_ctrl "${code}"
    {
        while read line; do
            ((i == 0)) && {
                whole_prefix="${prefix}${p1}"
            } || {
                whole_prefix="${prefix}${p2}"
            }
            ((i++))
            [ -z "${line}" ] && {
                echo
            } || {
                echo "${whole_prefix}${line}"
            }
        done | replace_reset "${code}"
    } <<EOF
${str}
EOF
    [ -z "${code}" ] || print_tty_ctrl "0"
    __need_blank_line=1
}

write_eval() {
    write_paragraph "$(print_eval "$@")"
}

write_error() {
    write_paragraph "**${1}**" "${2}" "${3}" '01;31'
}

write_error_eval() {
    write_error "$(print_eval "$@")"
}

write_quote_str() {
    local str="$1"
    increase_cur_level 1
    __need_blank_line=0
    echo
    write_paragraph "${str}" '' '' '01;35'
    echo
    __need_blank_line=0
    increase_cur_level -1
}

write_quote_cmd() {
    write_quote_str "$("$@" 2>&1)"
}

write_title() {
    local level="$1"
    local title="$2"
    local prefix='######'
    prefix="${prefix::$level}"
    ((__need_blank_line)) && echo
    print_tty_ctrl '01;34'
    echo "${prefix} ${title}" | replace_reset '01;34'
    print_tty_ctrl '0'
    __need_blank_line=0
    set_cur_level -1
}

write_order_list() {
    local str="$1"
    local index
    increase_cur_level -1
    increase_cur_level 1
    ((index = ++__list_indexes[__current_level - 1]))
    ((${#index} > 2)) && index="${index: -2}"
    index="${index}.   "
    increase_cur_level -1
    write_paragraph "${str}" "${index::4}" '    ' '01;32'
    increase_cur_level 1
}

write_order_list_eval() {
    write_order_list "$(print_eval "$@")"
}

# write_list() {
#     local str="$1"
#     increase_cur_level -1
#     write_paragraph "${str}" '*   ' '    ' '01;32'
#     increase_cur_level 1
# }


#############################
# print tips and links
#############################

wiki_url="http://fcitx-im.org/wiki"

beginner_guide_link() {
    print_link "$(_ "Beginner's Guide")" \
        "${wiki_url}$(_ /Beginner%27s_Guide)"
}

set_env_link() {
    local env_name="$1"
    local value="$2"
    local fmt
    fmt=$(_ 'Please set environment variable ${env_name} to "${value}" using the tool your distribution provides or add ${1} to your ${2}. See ${link}.')
    local link
    link=$(print_link \
        "$(_ "Input Method Related Environment Variables: ")${env_name}" \
        "${wiki_url}$(_ "/Input_method_related_environment_variables")#${env_name}")
    write_error_eval "${fmt}" "$(code_inline "export ${env_name}=${value}")" \
        "$(code_inline '~/.xprofile')"
}

gnome_36_link() {
    local link
    link=$(print_link \
        "$(_ "Note for GNOME Later than 3.6")" \
        "${wiki_url}$(_ "/Note_for_GNOME_Later_than_3.6")")
    local fmt
    fmt=$(_ 'If you are using ${1}, you may want to uninstall ${2} or remove ${3} to be able to use any input method other than ${2}. See ${link} for more detail as well as alternative solutions.')
    write_error_eval "${fmt}" "$(code_inline 'gnome>=3.6')" \
        "$(code_inline 'ibus')" "$(code_inline 'ibus-daemon')"
}

no_xim_link() {
    local fmt
    fmt=$(_ 'To see some application specific problems you may have when using xim, check ${link1}. For other more general problems of using XIM including application freezing, see ${link2}.')
    local link1
    link1=$(print_link \
        "$(_ "Hall of Shame for Linux IME Support")" \
        "${wiki_url}$(_ "/Hall_of_Shame_for_Linux_IME_Support")")
    local link2
    link2=$(print_link \
        "$(_ "here")" \
        "${wiki_url}$(_ "/XIM")")
    write_error_eval "${fmt}"
}


#############################
# system info
#############################

ldpaths=()
init_ld_paths() {
    local IFS=$'\n'
    local _ldpaths=($(ldconfig -p 2> /dev/null | grep '=>' | \
        sed -e 's:.* => \(.*\)/[^/]*$:\1:g' | sort -u)
        /lib /lib32 /lib64 /usr/lib /usr/lib32 /usr/lib64
        /usr/local/lib /usr/local/lib32 /usr/local/lib64)
    local path
    ldpaths=()
    for path in "${_ldpaths[@]}"; do
        [ -d "${path}" ] && add_and_check_file ldpath "${path}" && {
            ldpaths=("${ldpaths[@]}" "${path}")
        }
    done
}
init_ld_paths

check_system() {
    write_title 1 "$(_ "System Info.")"
    write_order_list "$(code_inline 'uname -a'):"
    if type uname &> /dev/null; then
        write_quote_cmd uname -a
    else
        write_error "$(print_not_found 'uname')"
    fi
    if type lsb_release &> /dev/null; then
        write_order_list "$(code_inline 'lsb_release -a'):"
        write_quote_cmd lsb_release -a
        write_order_list "$(code_inline 'lsb_release -d'):"
        write_quote_cmd lsb_release -d
    else
        write_order_list "$(code_inline lsb_release):"
        write_paragraph "$(print_not_found 'lsb_release')"
    fi
    write_order_list "$(code_inline /etc/lsb-release):"
    if [ -f /etc/lsb-release ]; then
        write_quote_cmd cat /etc/lsb-release
    else
        write_paragraph "$(print_not_found '/etc/lsb-release')"
    fi
    write_order_list "$(code_inline /etc/os-release):"
    if [ -f /etc/os-release ]; then
        write_quote_cmd cat /etc/os-release
    else
        write_paragraph "$(print_not_found '/etc/os-release')"
    fi
}

check_env() {
    write_title 1 "$(_ "Environment.")"
    write_order_list "DISPLAY:"
    write_quote_str "DISPLAY='${DISPLAY}'"
    write_order_list "$(_ "Keyboard Layout:")"
    increase_cur_level 1
    write_order_list "$(code_inline setxkbmap)"
    if type setxkbmap &> /dev/null; then
        write_quote_cmd setxkbmap -print
    else
        write_paragraph "$(print_not_found 'setxkbmap')"
    fi
    write_order_list "$(code_inline xprop)"
    if type xprop &> /dev/null; then
        write_quote_cmd xprop -root _XKB_RULES_NAMES
    else
        write_paragraph "$(print_not_found 'xprop')"
    fi
    increase_cur_level -1
    write_order_list "$(_ "Locale:")"
    if type locale &> /dev/null; then
        increase_cur_level 1
        write_order_list "$(_ "Current locale:")"
        write_quote_cmd locale
        write_order_list "$(_ "All locale:")"
        write_quote_cmd locale -a
        increase_cur_level -1
    else
        write_paragraph "$(print_not_found 'locale')"
    fi
}

check_fcitx() {
    local IFS=$'\n'
    write_title 1 "$(_ "Fcitx State.")"
    write_order_list "$(_ 'executable:')"
    if ! fcitx_exe="$(which fcitx 2> /dev/null)"; then
        write_error "$(_ "Cannot find fcitx executable!")"
        __need_blank_line=0
        write_error_eval "$(_ 'Please check ${1} for how to install fcitx.')" \
            "$(beginner_guide_link)"
        exit 1
    else
        write_eval "$(_ 'Found fcitx at ${1}.')" "$(code_inline "${fcitx_exe}")"
    fi
    write_order_list "$(_ 'version:')"
    version=$(fcitx -v 2> /dev/null | \
        sed -e 's/.*fcitx version: \([0-9.]*\).*/\1/g')
    write_eval "$(_ 'Fcitx version: ${version}')"
    write_order_list "$(_ 'process:')"
    psoutput=$(ps -Ao pid,comm)
    process=()
    while read line; do
        if [[ $line =~ ^([0-9]*)\ .*fcitx.* ]]; then
            [ "${BASH_REMATCH[1]}" = "$$" ] && continue
            process=("${process[@]}" "${line}")
        fi
    done <<EOF
${psoutput}
EOF
    if ! ((${#process[@]})); then
        write_error "$(_ "Fcitx is not running.")"
        __need_blank_line=0
        write_error_eval "$(_ 'Please check the Configure link of your distribution in ${1} for how to setup fcitx autostart.')" "$(beginner_guide_link)"
        return 1
    fi
    local pcount="${#process[@]}"
    if ((pcount > 1)); then
        write_eval "$(_ 'Found ${1} fcitx processes:')" "${#process[@]}"
    else
        write_eval "$(_ 'Found ${1} fcitx process:')" "${#process[@]}"
    fi
    write_quote_cmd print_array "${process[@]}"
    write_order_list "$(code_inline 'fcitx-remote'):"
    if type fcitx-remote &> /dev/null; then
        if ! fcitx-remote &> /dev/null; then
            write_error "$(_ "Cannot connect to fcitx correctly.")"
        else
            write_eval "$(_ '${1} works properly.')" \
                "$(code_inline 'fcitx-remote')"
        fi
    else
        write_error "$(print_not_found "fcitx-remote")"
    fi
}


#############################
# front end
#############################

_env_correct() {
    write_eval \
        "$(_ 'Environment variable ${1} is set to "${2}" correctly.')" \
        "$1" "$2"
}

_env_incorrect() {
    write_error_eval \
        "$(_ 'Environment variable ${1} is "${2}" instead of "${3}". Please check if you have exported it incorrectly in any of your init files.')" \
        "$1" "$3" "$2"
}

check_xim() {
    write_title 2 "Xim."
    xim_name=fcitx
    write_order_list "$(code_inline '${XMODIFIERS}'):"
    if [ -z "${XMODIFIERS}" ]; then
        set_env_link XMODIFIERS '@im=fcitx'
        __need_blank_line=0
    elif [ "${XMODIFIERS}" = '@im=fcitx' ]; then
        _env_correct 'XMODIFIERS' '@im=fcitx'
        __need_blank_line=0
    else
        _env_incorrect 'XMODIFIERS' '@im=fcitx' "${XMODIFIERS}"
        if [[ ${XMODIFIERS} =~ @im=([-_0-9a-zA-Z]+) ]]; then
            xim_name="${BASH_REMATCH[1]}"
        else
            __need_blank_line=0
            write_error_eval "$(_ 'Cannot interpret XMODIFIERS: ${1}.')" \
                "${XMODIFIERS}"
        fi
        if [ "${xim_name}" = "ibus" ]; then
            __need_blank_line=0
            gnome_36_link
        fi
    fi
    write_eval "$(_ 'Xim Server Name from Environment variable is ${1}.')" \
        "${xim_name}"
    write_order_list "$(_ 'XIM_SERVERS on root window:')"
    local atom_name=XIM_SERVERS
    if ! type xprop &> /dev/null; then
        write_error "$(print_not_found 'xprop')"
    else
        xprop=$(xprop -root -notype -f "${atom_name}" \
            '32a' ' $0\n' "${atom_name}" 2> /dev/null)
        if [[ ${xprop} =~ ^${atom_name}\ @server=(.*)$ ]]; then
            xim_server_name="${BASH_REMATCH[1]}"
            if [ "${xim_server_name}" = "${xim_name}" ]; then
                write_paragraph "$(_ "Xim server name is the same with that set in the environment variable.")"
            else
                write_error_eval "$(_ 'Xim server name: "${1}" is different from that set in the environment variable: "${2}".')" \
                    "${xim_server_name}" "${xim_name}"
            fi
        else
            write_error "$(_ "Cannot find xim_server on root window.")"
        fi
    fi
}

_check_toolkit_env() {
    local env_name="$1"
    local name="$2"
    write_order_list "$(code_inline '${'"${env_name}"'}'):"
    if [ -z "${!env_name}" ]; then
        set_env_link "${env_name}" 'fcitx'
    elif [ "${!env_name}" = 'fcitx' ]; then
        _env_correct "${env_name}" 'fcitx'
    else
        _env_incorrect "${env_name}" 'fcitx' "${!env_name}"
        __need_blank_line=0
        if [ "${!env_name}" = 'xim' ]; then
            write_error_eval "$(_ 'You are using xim in ${1} programs.')" \
                "${name}"
            no_xim_link
        else
            write_error_eval \
                "$(_ 'You may have trouble using fcitx in ${1} programs.')" \
                 "${name}"
            if [ "${!env_name}" = "ibus" ] && [ "${name}" = 'qt' ]; then
                __need_blank_line=0
                gnome_36_link
            fi
        fi
        set_env_link "${env_name}" 'fcitx'
    fi
}

check_qt() {
    write_title 2 "Qt."
    _check_toolkit_env QT_IM_MODULE qt
    qtimmodule_found=''
    write_order_list "$(_ 'Qt IM module files:')"
    echo
    for path in "${ldpaths[@]}"; do
        # the {/*,} here is for lib/$ARCH/ when output of ldconfig
        # failed to include it
        for file in "${path}"{/*,}/qt{,4}/**/*fcitx*.so; do
            if [[ ${file} =~ (im-fcitx|inputmethods) ]]; then
                __need_blank_line=0
                add_and_check_file qt "${file}" && {
                    write_eval "$(_ 'Found fcitx qt im module: ${1}.')" \
                        "$(code_inline "${file}")"
                }
                qtimmodule_found=1
            else
                __need_blank_line=0
                add_and_check_file qt "${file}" && {
                    write_eval \
                        "$(_ 'Found unknown fcitx qt module: ${1}.')" \
                        "$(code_inline "${file}")"
                }
            fi
        done
    done
    if [ -z "${qtimmodule_found}" ]; then
        __need_blank_line=0
        write_error "$(_ "Cannot find fcitx input method module for qt.")"
    fi
}

check_gtk_immodule() {
    local version="$1"
    local IFS=$'\n'
    local query_immodule
    local _query_immodule=($(find_in_path "gtk-query-immodules-${version}*"))
    local module_found=0
    local fcitx_gtk
    write_order_list "gtk ${version}:"
    [ ${#_query_immodule[@]} = 0 ] && {
        write_error_eval \
            "$(_ "Cannot find gtk-query-immodules for gtk ${1}")" \
            "${version}"
        return 1
    }
    local f
    for f in "${_query_immodule[@]}"; do
        add_and_check_file "gtk_immodule_${version}" "${f}" && {
            write_eval \
                "$(_ 'Found gtk-query-immodules for gtk ${1} at ${2}.')" \
                "${version}" "$(code_inline "${f}")"
            if fcitx_gtk=$("$f" | grep fcitx); then
                module_found=1
                __need_blank_line=0
                write_eval "$(_ 'Found fcitx im modules for gtk ${1}.')" \
                    "${version}"
                write_quote_str "${fcitx_gtk}"
            fi
        }
    done
    ((module_found)) || {
        write_error_eval \
            "$(_ 'Cannot find fcitx im module for gtk ${1}.')" \
            "${version}"
    }
}

check_gtk_immodule_cache() {
    local version="$1"
    local IFS=$'\n'
    local cache_found=0
    local fcitx_gtk
    write_order_list "gtk ${version}:"
    for path in /etc "${ldpaths[@]}"; do
        # the {/*,} here is for lib/$ARCH/ when output of ldconfig
        # failed to include it
        for file in "${path}"{/*,}/gtk{-${version}{.0,},}{/**,}/*immodules*; do
            [ -f "${file}" ] || continue
            add_and_check_file "gtk_immodule_cache_${version}" "${file}" || {
                continue
            }
            write_eval "$(_ 'Found immodules cache for gtk ${1} ${2}.')" \
                "${version}" "$(code_inline "${file}")"
            if fcitx_gtk=$(grep fcitx "${file}"); then
                cache_found=1
                __need_blank_line=0
                write_eval "$(_ 'Found fcitx in cache file ${1}:')" \
                    "$(code_inline "${file}")"
                write_quote_str "${fcitx_gtk}"
            fi
        done
    done
    ((cache_found)) || {
        write_error_eval \
            "$(_ 'Cannot find fcitx im module for gtk ${1} in cache.')" \
            "${version}"
    }
}

check_gtk() {
    write_title 2 "Gtk."
    _check_toolkit_env GTK_IM_MODULE gtk
    write_order_list "$(_ 'Gtk IM module files:')"
    increase_cur_level 1
    check_gtk_immodule 2
    check_gtk_immodule 3
    increase_cur_level -1
    write_order_list "$(_ 'Gtk IM module cache:')"
    increase_cur_level 1
    check_gtk_immodule_cache 2
    check_gtk_immodule_cache 3
    increase_cur_level -1
}


#############################
# fcitx modules
#############################

check_modules() {
    local addon_conf_dir
    write_title 2 "$(_ "Fcitx Addons:")"
    write_order_list "$(_ 'Addon Config Dir:')"
    addon_conf_dir="$(get_config_dir addonconfigdir addon)" || {
        write_error "$(_ "Cannot find fcitx addon config directory.")"
        return
    }
    local enabled_addon=()
    local disabled_addon=()
    local name
    local enable
    write_eval "$(_ 'Found fcitx addon config directory: ${1}.')" \
        "$(code_inline "${addon_conf_dir}")"
    write_order_list "$(_ 'Addon List:')"
    for file in "${addon_conf_dir}"/*.conf; do
        if ! name=$(get_from_config_file "${file}" Name); then
            write_error_eval \
                "$(_ 'Invalid addon config file ${1}.')" \
                "$(code_inline "${file}")"
            continue
        fi
        enable=$(get_from_config_file "${file}" Enabled)
        if [ -f ~/.config/fcitx/addon/${name}.conf ]; then
            _enable=$(get_from_config_file \
                ~/.config/fcitx/addon/${name}.conf Enabled)
            [ -z "${_enable}" ] || enable="${_enable}"
        fi
        if [ $(echo "${enable}" | sed -e 's/.*/\L&/g') = false ]; then
            disabled_addon=("${disabled_addon[@]}" "${name}")
        else
            enabled_addon=("${enabled_addon[@]}" "${name}")
        fi
    done
    increase_cur_level 1
    write_order_list_eval "$(_ 'Found ${1} enabled addons:')" \
        "${#enabled_addon[@]}"
    [ "${#enabled_addon[@]}" = 0 ] || {
        write_quote_cmd print_array "${enabled_addon[@]}"
    }
    write_order_list_eval "$(_ 'Found ${1} disabled addons:')" \
        "${#disabled_addon[@]}"
    [ "${#disabled_addon[@]}" = 0 ] || {
        write_quote_cmd print_array "${disabled_addon[@]}"
    }
    increase_cur_level -1
}

check_input_methods() {
    write_title 2 "$(_ "Input Methods:")"
    local IFS=','
    local imlist=($(get_from_config_file \
        ~/.config/fcitx/profile EnabledIMList)) || {
        write_error "$(_ "Cannot read im list from fcitx profile.")"
        return 0
    }
    local enabled_im=()
    local disabled_im=()
    local im
    local name
    local enable
    for im in "${imlist[@]}"; do
        [[ $im =~ ^([^:]+):(True|False)$ ]] || {
            write_error_eval "$(_ 'Invalid item ${1} in im list.')" \
                "${im}"
            continue
        }
        name="${BASH_REMATCH[1]}"
        if [ "${BASH_REMATCH[2]}" = True ]; then
            enabled_im=("${enabled_im[@]}" "${name}")
        else
            disabled_im=("${disabled_im[@]}" "${name}")
        fi
    done
    write_order_list_eval "$(_ 'Found ${1} enabled input methods:')" \
        "${#enabled_im[@]}"
    [ "${#enabled_im[@]}" = 0 ] || {
        write_quote_cmd print_array "${enabled_im[@]}"
    }
    write_order_list "$(_ 'Default input methods:')"
    case "${#enabled_im[@]}" in
        0)
            write_error "$(_ "You don't have any input methods enabled.")"
            ;;
        1)
            write_error "$(_ "You only have one input method enabled, please add a keyboard input method as the first one and your main input method as the second one.")"
            ;;
        *)
            if [[ ${enabled_im[0]} =~ ^fcitx-keyboard- ]]; then
                write_eval \
                    "$(_ 'You have a keyboard input method "${1}" correctly added as your default input method.')" \
                    "${enabled_im[0]}"
            else
                write_error_eval \
                    "$(_ 'Your first (default) input method is ${1} instead of a keyboard input method. You may have trouble deactivate fcitx.')" \
                    "${enabled_im[0]}"
            fi
            ;;
    esac
}


#############################
# log
#############################

check_log() {
    write_order_list "$(code_inline 'date')."
    if type date &> /dev/null; then
        write_quote_cmd date
    else
        write_error "$(print_not_found 'date')"
    fi
    write_order_list "$(code_inline '~/.config/fcitx/log/')."
    [ -d ~/.config/fcitx/log/ ] || {
        write_paragraph "$(print_not_found '~/.config/fcitx/log/')"
        return
    }
    write_quote_cmd ls -AlF ~/.config/fcitx/log/
    write_order_list "$(code_inline '~/.config/fcitx/log/crash.log')."
    if [ -f ~/.config/fcitx/log/crash.log ]; then
        write_quote_cmd cat ~/.config/fcitx/log/crash.log
    else
        write_paragraph "$(print_not_found '~/.config/fcitx/log/crash.log')"
    fi
}


#############################
# cmd line
#############################

_check_frontend=1
_check_modules=1
_check_log=1
[ -z "$1" ] || exec > "$1"


#############################
# init output
#############################

check_istty


#############################
# run
#############################

check_system
check_env
check_fcitx

((_check_frontend)) && {
    write_title 1 "$(_ "Frontends setup.")"
    check_xim
    check_qt
    check_gtk
}

((_check_modules)) && {
    write_title 1 "$(_ "Configuration.")"
    check_modules
    check_input_methods
}

((_check_log)) && {
    write_title 1 "$(_ "Log.")"
    check_log
}

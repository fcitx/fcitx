#!/usr/bin/env bash

shopt -s extglob nullglob globstar


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
    local IFS='\n'
    if [ ! -z "$1" ]; then
        while read line; do
            echo "${line}" | sed -e 's/'$'\e''\[0m/'$'\e''\['"$1"'m/g'
        done
        [ -z "${line}" ] || {
            echo -n \
                "$(echo "${line}" | sed -e 's/'$'\e''\[0m/'$'\e''\['"$1"'m/g')"
        }
    else
        cat
    fi
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

write_error() {
    write_paragraph "**${1}**" "${2}" "${3}" '01;31'
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
    write_quote_str "$("$@")"
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

# write_list() {
#     local str="$1"
#     increase_cur_level -1
#     write_paragraph "${str}" '*   ' '    ' '01;32'
#     increase_cur_level 1
# }


#############################
# print tips and links
#############################

beginner_guide_link() {
    print_link "Beginner's Guide" "http://fcitx-im.org/wiki/Beginner%27s_Guide"
}

set_env_link() {
    local env_name="$1"
    local value="$2"
    write_error "Please set environment variable ${env_name} to '${value}' using the tool your distribution provides or add $(code_inline "export ${env_name}=${value}") to your $(code_inline '~/.xprofile'). See $(print_link "Input Method Related Environment Variables: ${env_name}" "http://fcitx-im.org/wiki/Input_method_related_environment_variables#${env_name}")."
}

gnome_36_link() {
    write_error "If you are using $(code_inline 'gnome>=3.6'), you may want to uninstall $(code_inline 'ibus') or remove $(code_inline 'ibus-daemon') to be able to use any input method other than $(code_inline 'ibus'). See $(print_link "Note for GNOME Later than 3.6" "http://fcitx-im.org/wiki/Note_for_GNOME_Later_than_3.6") for more detail as well as alternative solutions."
}

no_xim_link() {
    write_error "To see some application specific problems you may have when using xim, check $(print_link "Hall of Shame for Linux IME Support" "http://fcitx-im.org/wiki/Hall_of_Shame_for_Linux_IME_Support"). Other more general problems of using XIM including application freezing. See also $(print_link "here" "http://fcitx-im.org/wiki/XIM")."
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
    write_title 1 "System Info."
    write_order_list "$(code_inline 'uname -a'):"
    if type uname &> /dev/null; then
        write_quote_cmd uname -a
    else
        write_error "$(code_inline 'uname') not found."
    fi
    if type lsb_release &> /dev/null; then
        write_order_list "$(code_inline 'lsb_release -a'):"
        write_quote_cmd lsb_release -a
        write_order_list "$(code_inline 'lsb_release -d'):"
        write_quote_cmd lsb_release -d
    else
        write_order_list "$(code_inline lsb_release):"
        write_paragraph "$(code_inline 'lsb_release') not found."
    fi
    write_order_list "$(code_inline /etc/lsb-release):"
    if [ -f /etc/lsb-release ]; then
        write_quote_cmd cat /etc/lsb-release
    else
        write_paragraph "$(code_inline /etc/lsb-release) not found."
    fi
    write_order_list "$(code_inline /etc/os-release):"
    if [ -f /etc/os-release ]; then
        write_quote_cmd cat /etc/os-release
    else
        write_paragraph "$(code_inline /etc/os-release) not found."
    fi
}

check_fcitx() {
    write_title 1 "Fcitx State."
    write_order_list 'executable:'
    if ! fcitx_exe="$(which fcitx 2> /dev/null)"; then
        write_error "Cannot find fcitx executable!"
        __need_blank_line=0
        write_error "Please check $(beginner_guide_link) for how to install fcitx."
        exit 1
    else
        write_paragraph "Found fcitx at $(code_inline "${fcitx_exe}")."
    fi
    write_order_list 'version:'
    version=$(fcitx -v 2> /dev/null | \
        sed -e 's/.*fcitx version: \([0-9.]*\).*/\1/g')
    write_paragraph "Fcitx version ${version}."
    write_order_list 'process:'
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
        write_error "Fcitx is not running."
        __need_blank_line=0
        write_error "Please check the Configure link of your distribution in $(beginner_guide_link) for how to setup fcitx to start automatically."
        return 1
    fi
    local pcount="${#process[@]}"
    if ((pcount > 1)); then
        write_paragraph "Found ${#process[@]} fcitx processes:"
    else
        write_paragraph "Found ${#process[@]} fcitx process:"
    fi
    write_quote_cmd print_array "${process[@]}"
    write_order_list 'remote:'
    if type fcitx-remote &> /dev/null; then
        if ! fcitx-remote &> /dev/null; then
            write_error "Cannot connect to fcitx correctly."
        else
            write_paragraph "fcitx-remote works properly."
        fi
    else
        write_error "Cannot find fcitx-remote."
    fi
}


#############################
# front end
#############################

check_xim() {
    write_title 2 "Xim."
    xim_name=fcitx
    write_order_list "$(code_inline '${XMODIFIERS}'):"
    if [ -z "${XMODIFIERS}" ]; then
        set_env_link XMODIFIERS '@im=fcitx'
        __need_blank_line=0
    elif [ "${XMODIFIERS}" = '@im=fcitx' ]; then
        write_paragraph \
            "Environment variable XMODIFIERS is set to '@im=fcitx' correctly."
        __need_blank_line=0
    else
        write_error "Environment variable XMODIFIERS is '${XMODIFIERS}' instead of '@im=fcitx'. Please check if you have exported it incorrectly in any of your init files."
        if [[ ${XMODIFIERS} =~ @im=([-_0-9a-zA-Z]+) ]]; then
            xim_name="${BASH_REMATCH[1]}"
        else
            __need_blank_line=0
            write_error "Cannot interpret XMODIFIERS: ${XMODIFIERS}."
        fi
        if [ "${xim_name}" = "ibus" ]; then
            __need_blank_line=0
            gnome_36_link
        fi
    fi
    write_paragraph "Xim Server Name from Environment variable is ${xim_name}."
    write_order_list 'XIM_SERVERS on root window:'
    local atom_name=XIM_SERVERS
    if ! type xprop &> /dev/null; then
        write_error "Cannot find xprop."
    else
        xprop=$(xprop -root -notype -f "${atom_name}" \
            '32a' ' $0\n' "${atom_name}" 2> /dev/null)
        if [[ ${xprop} =~ ^${atom_name}\ @server=(.*)$ ]]; then
            xim_server_name="${BASH_REMATCH[1]}"
            if [ "${xim_server_name}" = "${xim_name}" ]; then
                write_paragraph "Xim server's name is the same with that set in the environment variable."
            else
                write_error "Xim server's name: '${xim_server_name}' is different from that set in the environment variable: '${xim_name}'."
            fi
        else
            write_error "Cannot find xim_server."
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
        write_paragraph \
            "Environment variable ${env_name} is set to 'fcitx' correctly."
    else
        write_error "Environment variable ${env_name} is '${!env_name}' instead of 'fcitx'."
        __need_blank_line=0
        if [ "${!env_name}" = 'xim' ]; then
            write_error "You are using xim in ${name} programs."
            no_xim_link
        else
            write_error "You may have trouble using fcitx in ${name} programs."
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
    write_order_list 'Qt IM module files:'
    echo
    for path in "${ldpaths[@]}"; do
        # the {/*,} here is for lib/$ARCH/ when output of ldconfig
        # failed to include it
        for file in "${path}"{/*,}/qt{,4}/**/*fcitx*.so; do
            if [[ ${file} =~ (im-fcitx|inputmethods) ]]; then
                __need_blank_line=0
                add_and_check_file qt "${file}" && {
                    write_paragraph \
                        "Found fcitx qt im module: $(code_inline "${file}")."
                }
                qtimmodule_found=1
            else
                __need_blank_line=0
                add_and_check_file qt "${file}" && {
                    write_paragraph \
                        "Found unknown fcitx qt module: $(code_inline "${file}")."
                }
            fi
        done
    done
    if [ -z "${qtimmodule_found}" ]; then
        __need_blank_line=0
        write_error "Cannot find fcitx input method module for qt."
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
        write_error "Cannot find gtk-query-immodules for gtk ${version}"
        return 1
    }
    local f
    for f in "${_query_immodule[@]}"; do
        add_and_check_file "gtk_immodule_${version}" "${f}" && {
            write_paragraph \
                "Found gtk-query-immodules for gtk ${version} at $(code_inline "${f}")."
            if fcitx_gtk=$("$f" | grep fcitx); then
                module_found=1
                __need_blank_line=0
                write_paragraph "Found fcitx im modules for gtk ${version}."
                write_quote_str "${fcitx_gtk}"
            fi
        }
    done
    ((module_found)) || {
        write_error "Cannot find fcitx im module for gtk ${version}."
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
            write_paragraph \
                "Found immodules cache for gtk ${version} $(code_inline "${file}")."
            if fcitx_gtk=$(grep fcitx "${file}"); then
                cache_found=1
                __need_blank_line=0
                write_paragraph "Found fcitx in cache file $(code_inline "${file}"):"
                write_quote_str "${fcitx_gtk}"
            fi
        done
    done
    ((cache_found)) || {
        write_error "Cannot find fcitx im module for gtk ${version} in cache."
    }
}

check_gtk() {
    write_title 2 "Gtk."
    _check_toolkit_env GTK_IM_MODULE gtk
    write_order_list 'Gtk IM module files:'
    increase_cur_level 1
    check_gtk_immodule 2
    check_gtk_immodule 3
    increase_cur_level -1
    write_order_list 'Gtk IM module cache:'
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
    write_title 2 "Fcitx Addons:"
    write_order_list 'Addon Config Dir:'
    addon_conf_dir="$(get_config_dir addonconfigdir addon)" || {
        write_error "Cannot find fcitx addon config directory."
        return
    }
    local enabled_addon=()
    local disabled_addon=()
    local name
    local enable
    write_paragraph \
        "Found fcitx addon config directory: $(code_inline "${addon_conf_dir}")."
    write_order_list 'Addon List:'
    for file in "${addon_conf_dir}"/*.conf; do
        if ! name=$(get_from_config_file "${file}" Name); then
            write_error "Invalid addon config file $(code_inline "${file}")."
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
    write_order_list "Found ${#enabled_addon[@]} enabled addons:"
    [ "${#enabled_addon[@]}" = 0 ] || {
        write_quote_cmd print_array "${enabled_addon[@]}"
    }
    write_order_list "Found ${#disabled_addon[@]} disabled addons:"
    [ "${#disabled_addon[@]}" = 0 ] || {
        write_quote_cmd print_array "${disabled_addon[@]}"
    }
    increase_cur_level -1
}

check_input_methods() {
    write_title 2 "Input Methods:"
    local IFS=','
    local imlist=($(get_from_config_file \
        ~/.config/fcitx/profile EnabledIMList)) || {
        write_error "Cannot read im list from fcitx profile."
        return 0
    }
    local enabled_im=()
    local disabled_im=()
    local im
    local name
    local enable
    for im in "${imlist[@]}"; do
        [[ $im =~ ^([^:]+):(True|False)$ ]] || {
            write_error "Invalid item ${im} in im list."
            continue
        }
        name="${BASH_REMATCH[1]}"
        if [ "${BASH_REMATCH[2]}" = True ]; then
            enabled_im=("${enabled_im[@]}" "${name}")
        else
            disabled_im=("${disabled_im[@]}" "${name}")
        fi
    done
    write_order_list "Found ${#enabled_im[@]} enabled input methods:"
    [ "${#enabled_im[@]}" = 0 ] || {
        write_quote_cmd print_array "${enabled_im[@]}"
    }
    write_order_list 'Default input methods:'
    case "${#enabled_im[@]}" in
        0)
            write_error "You don't have any input methods enabled."
            ;;
        1)
            write_error "You only have one input method enabled, please add a keyboard input method as the first one and your main input method as the second one."
            ;;
        *)
            if [[ ${enabled_im[0]} =~ ^fcitx-keyboard- ]]; then
                write_paragraph "You have a keyboard input method '${enabled_im[0]}' correctly added as your default input method."
            else
                write_error "Your first(default) input method is ${enabled_im[0]} instead of a keyboard input method. You may have trouble deactivate fcitx."
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
        write_error "$(code_inline 'date') not found."
    fi
    write_order_list "$(code_inline '~/.config/fcitx/log/')."
    [ -d ~/.config/fcitx/log/ ] || {
        write_paragraph "$(code_inline '~/.config/fcitx/log/') not found."
        return
    }
    write_quote_cmd ls -AlF ~/.config/fcitx/log/
    write_order_list "$(code_inline '~/.config/fcitx/log/crash.log')."
    if [ -f ~/.config/fcitx/log/crash.log ]; then
        write_quote_cmd cat ~/.config/fcitx/log/crash.log
    else
        write_paragraph "$(code_inline '~/.config/fcitx/log/crash.log') not found."
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
check_fcitx

((_check_frontend)) && {
    write_title 1 "Frontends setup."
    check_xim
    check_qt
    check_gtk
}

((_check_modules)) && {
    write_title 1 "Configuration."
    check_modules
    check_input_methods
}

((_check_log)) && {
    write_title 1 "Log."
    check_log
}

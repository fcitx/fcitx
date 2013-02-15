#!/bin/sh

export TEXTDOMAIN=fcitx

if which kdialog > /dev/null 2>&1; then
    message() {
        kdialog --msgbox "$1"
    }
    error() {
        kdialog --error "$1"
    }
elif which zenity > /dev/null 2>&1; then
    message() {
        zenity --info --text="$1"
    }
    error() {
        zenity --error --text="$1"
    }
else
    message() {
        echo "$1"
    }
    error() {
        echo "$1" >&2
    }
fi

if type gettext > /dev/null 2>&1; then
    _() {
        gettext "$@"
    }
else
    _() {
        echo "$@"
    }
fi

usage() {
    error "Usage: $0 [skin file]"
    exit 1
}

if [ $# != 1 ]; then
    usage
fi

_filelist=$(tar -tf "$1")

if [ "$(echo "${_filelist}" | grep -c '^[^/]*/$' 2> /dev/null)" != "1" ]; then
    error "$(_ "Error: skin file should only contain one directory.")"
    exit 1
fi

if ! echo "${_filelist}" | grep "^[^/]*/fcitx_skin.conf" > /dev/null 2>&1; then
    error "$(_ "Error: skin file doesn't contain skin config.")"
    exit 1
fi

if [ -n "$XDG_CONFIG_HOME" ]; then
    SKINPATH="$XDG_CONFIG_HOME/fcitx/skin"
elif [ -n "$HOME" ]; then
    SKINPATH="$HOME/.config/fcitx/skin"
else
    error "$(_ 'Error: $HOME or $XDG_CONFIG_HOME is not set, cannot determine the install path')"
    exit 1
fi

mkdir -p "$SKINPATH" || {
    echo "Error: cannot create skin dir"
    exit 1
}

if tar -C "$SKINPATH" -xf "$1"; then
    message "$(_ "Successfully Installed skin")"
else
    error "$(_ "Error: skin failed to install")"
    exit 1
fi

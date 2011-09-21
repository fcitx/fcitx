#!/bin/sh

export TEXTDOMAIN=fcitx

if which kdialog > /dev/null 2>&1; then
    MESSAGE_ERROR="kdialog --error"
    MESSAGE_COMMON="kdialog --msgbox"
elif which zenity > /dev/null 2>&1; then
    MESSAGE_ERROR="zenity_error"
    MESSAGE_COMMON="zenity_info"
else
    MESSAGE_ERROR="echo"
    MESSAGE_COMMON="echo"
fi

function zenity_error()
{
    zenity --error --text="$1"
}

function zenity_info()
{
    zenity --info --text="$1"
}

function usage()
{
    echo "Usage: fcitx-skin-installer [skin file]"
    exit 1
}

function message()
{
    $MESSAGE_COMMON "$1"
}

function error()
{
    $MESSAGE_ERROR "$1"
}

if [ $# != 1 ]; then
    usage
fi

DIRCOUNT=`tar -tf $1 | grep -c '/$'`
if [ "x$DIRCOUNT" != "x1" ]; then
    error `gettext "Error: skin file should only contain one directory."`
    exit 1
fi

DIRNAME=`tar -tf $1 | grep '/$'`
tar -tf $1 ${DIRNAME}fcitx_skin.conf >/dev/null 2>&1

if [ $? != 0 ]; then
    error `gettext "Error: skin file doesn't contain skin config."`
    exit 1
fi

SKINPATH=

if [ ! -z "$XDG_CONFIG_HOME" ]; then
    SKINPATH=$XDG_CONFIG_HOME/fcitx/skin
elif [ ! -z "$HOME" ]; then
    SKINPATH=$HOME/.config/fcitx/skin
fi

if [ -z "$SKINPATH" ]; then
    error `gettext 'Error: $HOME or $XDG_CONFIG_HOME is not set, cannot determine the install path'`
    exit 1
fi

mkdir -p $SKINPATH || ( echo "Error: cannot create skin dir" ; exit 1 )

tar -C $SKINPATH -zxf $1

if [ $? != 0 ]; then
    error `gettext "Error: skin failed to install"`
    exit 1
else
    message `gettext "Successfully Installed skin"`
fi

#!/bin/sh

# don't do anything on non-CJK at this time.
if locale | grep LC_CTYPE | grep -qv "[zh\|ja\|ko]_"; then
    return
fi

detect_binary()
{
    which "$1" > /dev/null 2>&1
}

detect_binary fcitx || return

detect_gtk()
{
    version=${1}.0
    detect_binary gtk-query-immodules-${version}
}

detect_gtk_im_module()
{
    gtk-query-immodules-${version} | grep -q 'fcitx'
}

detect_qt()
{
    detect_binary qmake
}

detect_qt_im_module()
{
    QTPATH=`qmake -query QT_INSTALL_PLUGINS`
    ls $QTPATH/inputmethods/*fcitx* &> /dev/null
}

if [ -z "$XMODIFIERS" ]; then
    export XMODIFIERS="@im=fcitx"
fi

if [ -z "$GTK_IM_MODULE" ]; then
    if detect_gtk 2 || detect_gtk 3; then
        if detect_gtk_im_module 2 || detect_gtk_im_module 3; then
            GTK_IM_MODULE=fcitx
        else
            GTK_IM_MODULE=xim
        fi

        export GTK_IM_MODULE
    fi
fi

if [ -z "$QT_IM_MODULE" ]; then
    if detect_qt; then
        if detect_qt_im_module; then
            QT_IM_MODULE=fcitx
        else
            QT_IM_MODULE=xim
        fi

        export QT_IM_MODULE
    fi
fi

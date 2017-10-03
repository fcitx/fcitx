#!/bin/sh
#--------------------------------------
# fcitx-config
#

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

if which gettext > /dev/null 2>&1; then
    _() {
        gettext "$@"
    }
else
    _() {
        echo "$@"
    }
fi



# from xdg-open

detectDE() {
    # see https://bugs.freedesktop.org/show_bug.cgi?id=34164
    unset GREP_OPTIONS

    if [ -n "${XDG_CURRENT_DESKTOP}" ]; then
      case "${XDG_CURRENT_DESKTOP}" in
         GNOME)
           DE=gnome;
           ;;
         KDE)
           DE=kde;
           ;;
         LXDE)
           DE=lxde;
           ;;
         XFCE)
           DE=xfce
      esac
    fi

    if [ x"$DE" = x"" ]; then
      # classic fallbacks
      if [ x"$KDE_FULL_SESSION" = x"true" ]; then DE=kde;
      elif xprop -root KDE_FULL_SESSION 2> /dev/null | grep ' = \"true\"$' > /dev/null 2>&1; then DE=kde;
      elif [ x"$GNOME_DESKTOP_SESSION_ID" != x"" ]; then DE=gnome;
      elif [ x"$MATE_DESKTOP_SESSION_ID" != x"" ]; then DE=mate;
      elif dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager > /dev/null 2>&1 ; then DE=gnome;
      elif xprop -root _DT_SAVE_MODE 2> /dev/null | grep ' = \"xfce4\"$' >/dev/null 2>&1; then DE=xfce;
      elif xprop -root 2> /dev/null | grep -i '^xfce_desktop_window' >/dev/null 2>&1; then DE=xfce
      fi
    fi

    if [ x"$DE" = x"" ]; then
      # fallback to checking $DESKTOP_SESSION
      case "$DESKTOP_SESSION" in
         gnome)
           DE=gnome;
           ;;
         LXDE|Lubuntu)
           DE=lxde;
           ;;
         xfce|xfce4|'Xfce Session')
           DE=xfce;
           ;;
      esac
    fi

    if [ x"$DE" = x"" ]; then
      # fallback to uname output for other platforms
      case "$(uname 2>/dev/null)" in
        Darwin)
          DE=darwin;
          ;;
      esac
    fi

    if [ x"$DE" = x"gnome" ]; then
      # gnome-default-applications-properties is only available in GNOME 2.x
      # but not in GNOME 3.x
      which gnome-default-applications-properties > /dev/null 2>&1  || DE="gnome3"
    fi
}

run_kde() {
    if xprop -root KDE_SESSION_VERSION 2>&1 | grep -q "= 5"; then
        if (kcmshell5 --list 2>/dev/null | grep ^kcm_fcitx > /dev/null 2>&1); then
            if [ x"$1" != x ]; then
                exec kcmshell5 kcm_fcitx --args "$1"
            else
                exec kcmshell5 kcm_fcitx
            fi
        fi
    fi

    # not harm to also check kde4 version on kf5
    if (kcmshell4 --list 2>/dev/null | grep ^kcm_fcitx > /dev/null 2>&1); then
        if [ x"$1" != x ]; then
            exec kcmshell4 kcm_fcitx --args "$1"
        else
            exec kcmshell4 kcm_fcitx
        fi
    fi
}

run_gtk() {
    if which fcitx-config-gtk > /dev/null 2>&1; then
        exec fcitx-config-gtk "$1"
    fi
}

run_gtk3() {
    if which fcitx-config-gtk3 > /dev/null 2>&1; then
        exec fcitx-config-gtk3 "$1"
    fi
}

run_xdg() {
    case "$DE" in
        kde)
            message "$(_ "You're currently running KDE, but KCModule for fcitx couldn't be found, the package name of this KCModule is usually kcm-fcitx or kde-config-fcitx. Now it will open config file with default text editor.")"
            ;;
        *)
            message "$(_ "You're currently running Fcitx with GUI, but fcitx-configtool couldn't be found, the package name is usually fcitx-config-gtk, fcitx-config-gtk3 or fcitx-configtool. Now it will open config file with default text editor.")"
            ;;
    esac

    if command="$(which xdg-open 2>/dev/null)"; then
        detect_im_addon $1
        if [ x"$filename" != x ]; then
            exec $command "$HOME/.config/fcitx/conf/$filename.config"
        else
            exec "$command" "$HOME/.config/fcitx/config"
        fi
    fi
}

_which_cmdline() {
    cmd="$(which "$1")" || return 1
    shift
    echo "$cmd $*"
}

detect_im_addon() {
    filename=$1
    addonname=
    if [ x"$filename" != x ]; then
        addonname=$(fcitx-remote -m $1 2>/dev/null)
        if [ "$?" != "0" ]; then
            filename=
        elif [ x"$addonname" != x ]; then
            filename=$addonname
        fi
    fi

    if [ ! -f "$HOME/.config/fcitx/conf/$filename.config" ]; then
        filename=
    fi
}

run_editor() {
    if command="$(_which_cmdline ${EDITOR} 2>/dev/null)" ||
        command="$(_which_cmdline ${VISUAL} 2>/dev/null)"; then
        detect_im_addon $1
        if [ x"$filename" != x ]; then
            exec $command "$HOME/.config/fcitx/conf/$filename.config"
        else
            exec $command "$HOME/.config/fcitx/config"
        fi
    fi
}

if [ ! -n "$DISPLAY" ] && [ ! -n "$WAYLAND_DISPLAY" ]; then
    run_editor "$1"
    echo 'Please run it under X, or set $EDITOR or $VISUAL' >&2
    exit 0
fi

detectDE

# even if we are not on KDE, we should still try kde, some wrongly
# configured kde desktop cannot be detected (usually missing xprop)
# and if kde one can work, so why not use it if gtk, gtk3 not work?
# xdg/editor is never a preferred solution
case "$DE" in
    kde)
        order="kde gtk3 gtk xdg editor"
        ;;
    *)
        order="gtk3 gtk kde xdg editor"
        ;;
esac

for cmd in $order; do
    run_${cmd} "$1"
done

echo 'Cannot find a command to run.' >&2

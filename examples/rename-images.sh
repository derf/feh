#!/bin/sh

# Interactively rename images

USAGE="USAGE:
$0: Print this help and exit
$0 -h|--help: Print this help and exit
$0 [FLAGS] -f FILE : rename pictures read from FILE interactively
$0 [FLAGS] PICTURES...: rename PICTURES interactively
where FLAGS are one or more of:

-s|--separator SEPARATOR: replace contiguous strings of spaces with SEPARATOR

PREFIX FLAGS (last one called wins):
-p|--prefix: retain the current basename as a prefix and display it. This is default.
-k|--keep-prefix: keep the current filename as a prefix, but do not display it.
-K|--no-keep-prefix: discard the current filename.

EDIT FLAGS (last one called wins):
-E|--no-edit: rotations and flips done with < > | _ will not save to disk.
    This is the default.
-e|--edit: rotations and flips done with < > | _ will save to disk. This is the default.
"

help() {
    warn "$USAGE"
    warn ""
    instructions
    die 0
}

warn() {
    echo "$@" 1>&2
}

die() {
    status="$1"
    shift
    warn "$@"
    exit "$status"
}

instructions() {
    cat <<EOF
    Default keybindings:

    0|Enter: Interactively rename.
    1: Prefill with last name entered incremented by 1.
    2: Prefill with last name entered.
    8: Edit current log and script, without advancing.
       Handy if you just realized you want to number a series of photos.
    9: Edit current script, without advancing.

    Space: next image
    p|Backspace|Left: previous image
    d: toggle filename display
    m: show menu
    q|Esc: quit feh and review 
    Delete: remove image from filelist 
    Ctrl+Delete: delete image and remove from filelist
    <>: rotate left / right.

    Renaming will be done after you quit and confirm,
    so you can cycle through the file list multiple times,
    or edit the rename script before running it.

    see man 1 feh for further keyindings.
EOF
}

separate() {
    text="$1"
    if [ -n "$separator" ]; then
        printf '%s' "$text" | sed "s/  */ /; s/ /$separator/g"
    else
        printf '%s' "$text"
    fi
}

get_blank() {
    printf ''
}

get_duplicate() {
    sed '$!d' "$log"
}

get_increment() {
    get_duplicate | awk '
        function reverse(string, reversed, i, l) {
            l = length(string)
            for (i=l; i != 0; i--)
                reversed = reversed substr(string, i, 1);
            return reversed
        }
        function strip_number_suffix(string,  reversed) {
            reversed = reverse(string)
            sub(/[0-9]*/, "", reversed)
            return reverse(reversed)
        }

        {
            l_string = length()
            prefix = strip_number_suffix($0)
            l_prefix = length(prefix)
            l_number = l_string - l_prefix
            number = substr($0, l_prefix +1)
            sub(/0*/, "", number)
            number++
            printf("%s%0*d", prefix, l_number, number)
        }'
}

get_suggestion() {
    suggestion="$(get_"$prefill_type")"
    test "$prefix_do" = "display" && suggestion="$basename-$suggestion"
    printf '%s' "$suggestion"
}

process() {
    if ( printf "%s" "$pic" | grep -F '.' >/dev/null ); then
        extension="$(printf "%s" "$pic" | sed 's,.*\(\.[^.]\+\)$,\1,')"
    fi
    dirname="$(dirname "$pic")"
    basename="$(basename "$pic" "$extension")"
    suggestion="$(get_suggestion)"
    if [ "$prefix_do" = "display" ] || [ "$prefill_type" != "blank" ]; then
        entry_text="$suggestion"
    fi
    entered="$(zenity --width=500 --entry --text "New name:" --entry-text "$entry_text")"
    case "$prefix_do" in
        keep)
            printf "%s\n" "$entered" >>"$log"
            entered="${basename}-${entered}"
            ;;
        display)
            printf "%s\n" "${entered#"$suggestion"}" >>"$log"
            ;;
        discard)
            printf "%s\n" "$entered" >>"$log"
            ;;
    esac

    new_name="$(separate "$entered")"
    if [ -n "$new_name" ] && [ "$new_name" != "$basename" ]; then
        printf 'mv -v %s %s\n' "$pic" "$dirname/$new_name$extension" >>"$script"
    fi
}

edit() {
    vim "$script"
    printf '\n----\n'
    review
}

quit() {
    exit 0
}

run() {
    chmod 755 "$script"
    "$script"
}

review() {
    printf "Here is the edit file:\n\n"
    cat "$script"
    do_this=""
    while [ -z "$do_this" ]; do
        printf "\ne (edit), q (quit), or r (run)?\n"
        read reply
        test "$reply" = "e" && do_this=edit
        test "$reply" = "q" && do_this=quit
        test "$reply" = "r" && do_this=run
    done
    "$do_this"
}

test "$#" -eq 0 && die 0 "$USAGE"

if [ "$1" = "--process" ]; then
    if [ "$#" -lt 6 ] || [ "$#" -gt 7 ]; then
        die 2 "$USAGE"
    fi
    pic="$2"
    script="$3"
    log="$4"
    prefix_do="$5"
    prefill_type="$6"
    separator="$7"
    process "$pic"
elif [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    help
else
    prefix_do="display"
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --process)
            die 1 "$USAGE"
            ;;
            -f|--filelist)
            filelist="$2"
            shift 2
            ;;
            -s|--separator)
            separator="$2"
            shift 2
            ;;
            -k|--keep-prefix)
            prefix_do="keep"
            shift 1
            ;;
            -K|--no-keep-prefix)
            prefix_do="discard"
            shift 1
            ;;
            -p|--prefix)
            prefix_do="display"
            shift 1
            ;;
            -e|--edit)
            edit="--edit"
            shift
            ;;
            -E|--no-edit)
            edit=""
            shift
            ;;
            --)
            shift
            break
            ;;
            -*)
            warn "unsupported flag $1"
            die 1 "$USAGE"
            ;;
            *)
            test -n "$filelist" && die 1 "$USAGE"
            break
            ;;
        esac
    done

    temp="$(mktemp -d)"
    script="$temp/script"
    log="$temp/log"
    printf '#!/bin/sh\n\n' >"$script"
    instructions
    printf 'Building script at %s, and logging at %s\n' "$script" "$log"
    args="$edit --draw-filename -F"
    action="$0 --process %F $script $log $prefix_do blank $separator" 
    action1="$0 --process %F $script $log $prefix_do increment $separator" 
    action2="$0 --process %F $script $log $prefix_do duplicate $separator" 
    action8=";vim -O $log $script" 
    action9=";vim $script" 
    if [ -z "$filelist" ]; then
        # shellcheck disable=SC2086
        feh $args \
        --action "$action" \
        --action1 "$action1" \
        --action2 "$action2" \
        --action8 "$action8" \
        --action9 "$action9" \
        "$@"
    elif [ -r "$filelist" ]; then
        feh $args \
        --action "$action" \
        --action1 "$action1" \
        --action2 "$action2" \
        --action8 "$action8" \
        --action9 "$action9" \
        --filelist "$filelist"
    else
        warn "We should never get here"
        die 1 "$USAGE"
    fi
    review
fi

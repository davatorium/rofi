#!/usr/bin/env bash
#
# This code is released in public domain by Dave Davenport <qball@gmpclient.org> 
# This converts from old style theme (< 1.4) to new style theme (>= 1.4)
#
function update_color ()
{
    var=${1}
    var="${var#"${var%%[![:space:]]*}"}"   # remove leading whitespace characters
    var="${var%"${var##*[![:space:]]}"}"   # remove trailing whitespace characters
    if [[ ${var} =~ argb:[0-9a-fA-F]{6,8} ]]
    then
        echo "#${var:5}"
    else
        echo ${var}
    fi
}

function parse_window_color ()
{
    OLDIFS=${IFS}
    IFS=","
    entries=( ${1} )
    echo "@window {"
    echo "    background: $( update_color ${entries[0]});"
    echo "    foreground: $( update_color ${entries[1]});"
    echo "}"
    if [ -n "${entries[2]}" ]
    then
        echo "@separator {"
        echo "    foreground: $( update_color ${entries[2]});"
        echo "}"
        echo "@scrollbar {"
        echo "    foreground: $( update_color ${entries[2]});"
        echo "}"
    fi
    IFS=${OLDIFS}
}

function parse_color ()
{
    state=$1
    OLDIFS=${IFS}
    IFS=","
    entries=( ${2} )
    echo "@textbox normal.${state} { "
    echo "    background: $( update_color ${entries[0]});"
    echo "    foreground: $( update_color ${entries[1]});"
    echo "}"
    echo "@textbox selected.${state} { "
    echo "    background: $( update_color ${entries[3]});"
    echo "    foreground: $( update_color ${entries[4]});"
    echo "}"
    echo "@textbox alternate.${state} { "
    echo "    background: $( update_color ${entries[2]});"
    echo "    foreground: $( update_color ${entries[1]});"
    echo "}"
    IFS=${OLDIFS}
}

while read LINE
do
    if [[ ${LINE} =~ ^rofi\.color-normal: ]]
    then
        parse_color "normal" "${LINE:18}" 
    elif [[ ${LINE} =~ ^rofi\.color-urgent: ]]
    then
        parse_color "urgent" "${LINE:18}" 
    elif [[ ${LINE} =~ ^rofi\.color-active: ]]
    then
        parse_color "active" "${LINE:18}" 
    elif [[ ${LINE} =~ ^rofi\.color-window: ]]
    then
        parse_window_color "${LINE:18}"
    fi
done

#!/usr/bin/env bash

##
# Script used to create a screenshot of rofi.
# License: See rofi
##
RESET="\e[0m"
BOLD="\e[1m"
COLOR_BLACK="\e[0;30m"
COLOR_RED="\e[0;31m"
COLOR_GREEN="\e[0;32m"
COLOR_YELLOW="\e[0;33m"
COLOR_BLUE="\e[0;34m"

XRDB_FILE=$1
shift

OUTPUT_PNG=$1
shift

XVFB=$(which Xvfb 2> /dev/null)
XDOTOOL=$(which xdotool 2> /dev/null)
XRDB=$(which xrdb 2> /dev/null)
ROFI=$(which rofi 2> /dev/null)

function check_tool()
{
    if [ -z "${1}" ]
    then
        echo -e "${COLOR_RED}Failed to find:${RESET} $2"
        exit 1
    fi
}

XPID=
function create_fake_x ( )
{
    export DISPLAY=":$1"
    echo "Starting fake X: ${DISPLAY}"
    ${XVFB} ${DISPLAY}  -screen 0 800x600x24 &
    XPID=$!
    sleep 1
}

function destroy_fake_x ( )
{
    if [ -n "${XPID}" ]
    then
        echo "Stopping fake X: ${XPID}"
        kill ${XPID}
        wait ${XPID}
    fi
}

function generate()
{
    echo "Normal"
    echo "Alternative"
    echo "Urgent"
    echo "Urgent alternative"
    echo "Active"
    echo "Active alternative"
    echo "Normal selected"
}

# Check required tools
check_tool "${XVFB}" "Xvfb (X on virtual framebuffer)"
check_tool "${XDOTOOL}" "commandline X11 automation tool"
check_tool "${XRDB}" "X server resource database utility"
check_tool "${ROFI}" "Rofi, the tool we are screenshotting"

# Create random display number
VDISPLAY=${RANDOM}
let "VDISPLAY %= 20"
VDISPLAY=$((VDISPLAY+100))

echo "Xvfb:            ${XVFB}"
echo "Xresources:      ${XRDB_FILE}"
echo "Xvfb Display:    ${VDISPLAY}"

ROFI_OPTIONS="-selected-row 6 -u 2,3 -a 4,5 -location 0 -width 100 -lines 7 -columns 1"

export DISPLAY=${VDISPLAY}

if [ -n "${OUTPUT_PNG}" ]
then
    export ROFI_PNG_OUTPUT="${OUTPUT_PNG}"
fi

# Create fake X11
create_fake_x ${VDISPLAY}

# Load Xresources if specified.
if [ -n "${XRDB_FILE}" ]
then
    echo -e "${COLOR_YELLOW}Loading Xresources:${RESET} ${XRDB_FILE}"
    ${XRDB} -retain -load ${XRDB_FILE}
fi

(generate | ${ROFI} -config "${XRDB_FILE}" -dmenu ${ROFI_OPTIONS} > /dev/null )&
sleep 1
${XDOTOOL} key Alt+S
${XDOTOOL} key Return
sleep 2
destroy_fake_x

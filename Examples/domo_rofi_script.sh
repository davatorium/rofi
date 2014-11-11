#!/usr/bin/env bash 

LIGHT_HOST=192.150.0.113
LIGHT_PORT=8888

prompt() {
    printf "%s\n" "$@"
}

function set_lights ()
{
    menu=(
        "0.   Lights 0%"
        "10.  Lights 10%"
        "20.  Lights 20%"
        "30.  Lights 30%"
        "40.  Lights 40%"
        "50.  Lights 50%"
        "60.  Lights 60%"
        "70.  Lights 70%"
        "80.  Lights 80%"
        "90.  Lights 90%"
        "100. Lights 100%"
    )
    prompt "${menu[@]}"
}
function set_max_lights ()
{
    menu=(
        "max 0.   Lights 0%"
        "max 10.  Lights 10%"
        "max 20.  Lights 20%"
        "max 30.  Lights 30%"
        "max 40.  Lights 40%"
        "max 50.  Lights 50%"
        "max 60.  Lights 60%"
        "max 70.  Lights 70%"
        "max 80.  Lights 80%"
        "max 90.  Lights 90%"
        "max 100. Lights 100%"
    )
    prompt "${menu[@]}"
}
function set_min_lights ()
{
    menu=(
        "min 0.   Lights 0%"
        "min 10.  Lights 10%"
        "min 20.  Lights 20%"
        "min 30.  Lights 30%"
        "min 40.  Lights 40%"
        "min 50.  Lights 50%"
        "min 60.  Lights 60%"
        "min 70.  Lights 70%"
        "min 80.  Lights 80%"
        "min 90.  Lights 90%"
        "min 100. Lights 100%"
    )
    prompt "${menu[@]}"
}

function set_minimum()
{
    case "$(get_range_value)" in
    esac
}


function get_maximum()
{
    sleep 0.15
    echo "getmax" | nc ${LIGHT_HOST} ${LIGHT_PORT} | tail -n1 
}

function get_minimum()
{
    sleep 0.15
    echo "getmin" | nc ${LIGHT_HOST} ${LIGHT_PORT} | tail -n1
}
function configure()
{
    menu=(
        "Auto lights mode setup"
        "a.  Set minimum value."
        "b.  Set maximum value."
        ""
        "Current minimum: $(get_minimum)"
        "Current maximum: $(get_maximum)"
    )


    prompt "${menu[@]}"
}
function show_menu()
{
    menu=(
        "Lights on/off"
        ""
        "10.  Lights Low"
        "60.  Lights Middle"
        "100. Lights Full"
        "%.   Lights (advanced)"
        ""
        "Configure"
    )
    prompt "${menu[@]}"
}

function menu()
{

    case "$@" in
        "Lights on/off") echo "toggle"  "switch" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
        notify-send "Domotica" "Toggle light."
            ;;
        0.*) echo "0" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 0%."
            ;;
        10.*) echo "2" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 10%."
            ;;
        20.*) echo "4" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 20%."
            ;;
        30.*) echo "8" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 30%."
            ;;
        40.*) echo "16" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 40%."
            ;;
        50.*) echo "32" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 50%."
            ;;
        60.*) echo "64" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 60%."
            ;;
        70.*) echo "128" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 70%."
            ;;
        80.*) echo "256" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 80%."
            ;;
        90.*) echo "512" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 90%."
            ;;
        100.*) echo "1024" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT};
            notify-send "Domotica" "Set light level: 100%."
            ;;
        a.*)
            set_min_lights
            ;;
        b.*)
            set_max_lights
            ;;
        max\ 0.*) echo "setmax 0" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 0%."
        ;;
        max\ 10.*) echo "setmax 2" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 10%."
            ;;
        max\ 20.*) echo "setmax 4" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 20%."
            ;;
        max\ 30.*) echo "setmax 8" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 30%."
            ;;
        max\ 40.*) echo "setmax 16" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 40%."
            ;;
        max\ 50.*) echo "setmax 32" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 50%."
            ;;
        max\ 60.*) echo "setmax 64" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 60%."
            ;;
        max\ 70.*) echo "setmax 128" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 70%."
            ;;
        max\ 80.*) echo "setmax 256" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 80%."
            ;;
        max\ 90.*) echo "setmax 512" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 90%."
            ;;
        max\ 100.*) echo "setmax 1024" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-maximum level: 100%."
            ;;
        min\ 0.*) echo "setmin 0" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 0%."
            ;;
        min\ 10.*) echo "setmin 2" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 10%."
            ;;
        min\ 20.*) echo "setmin 4" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 20%."
            ;;
        min\ 30.*) echo "setmin 8" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 30%."
            ;;
        min\ 40.*) echo "setmin 16" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 40%."
            ;;
        min\ 50.*) echo "setmin 32" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 50%."
            ;;
        min\ 60.*) echo "setmin 64" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 60%."
            ;;
        min\ 70.*) echo "setmin 128" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 70%."
            ;;
        min\ 80.*) echo "setmin 256" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 80%."
            ;;
        min\ 90.*) echo "setmin 512" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 90%."
            ;;
        min\ 100.*) echo "setmin 1024" | nc -q0 ${LIGHT_HOST} ${LIGHT_PORT}
            notify-send "Domotica" "Set auto-minimum level: 100%."
            ;;
        \%.*)
            set_lights;;
        Configure)
                configure
            ;;
    esac
}

if [ -z "$@" ]
then
    show_menu
else
    menu "$@"
fi

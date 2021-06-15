#!/usr/bin/env bash

function shout()
{
echo -en "Normal Row\nAlternateRow\nNormal Row active\nAlternateRow Active\nNormal Row urgent\nAlternateRow urgent"
echo -en "\nSelected Row"
echo -en "\n1\n2\n3\n4\n5"
}

sleep 5
( shout | rofi -no-hide-scrollbar -columns 1 -width 1200 -location 0 -u 2,3 -a 4,5 -dmenu -p "Prompt" -padding 20 -line-margin 10 -selected-row 6 ) &
P=$!
sleep 5
scrot
sleep 1
killall rofi
wait $P

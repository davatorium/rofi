#!/usr/bin/env bash

function shout()
{
echo -en "Normal Row\nAlternateRow\nNormal Row active\nAlternateRow Active\nNormal Row urgent\nAlternateRow urgent"
echo -en "\nSelected Row"
}


shout | rofi -u 2,3 -a 4,5 -dmenu -p "Prompt:" -padding 20 -line-margin 10 -selected 6

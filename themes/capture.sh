#!/usr/bin/env bash

THEMES=../../../themes/*.rasi
ROFI_BIN=../../../build/rofi


function generate_options()
{
  echo -en "rofi\0icon\x1frofi\n"
  echo -en "help browser\0icon\x1fhelp-browser\n"
  echo -en "thunderbird\0icon\x1fthunderbird\n"
  echo -en "Urgent\0icon\x1femblem-urgent\n"
  echo -en "Active\0icon\x1fface-wink\n"
  echo -en "folder\0icon\x1ffolder\n"
  echo -en "Icon font 🐢 🥳\n"
  echo -en "Font icon\0icon\x1f<span size='x-large' color='red'>:-)</span>\n"
  echo -en "Quit\0icon\x1fapplication-exit\n"
}

function run_theme
{
  theme=$1
  BASE=$(basename ${theme})
  NAME=${BASE%.rasi}
  export ROFI_PNG_OUTPUT="${NAME}.png" 
  if [ ${NAME} = "default" ]
  then
	  echo "# Default theme" >> themes.md
  else
	  echo "# [${NAME}](https://github.com/davatorium/rofi/blob/next/themes/${BASE})" >> themes.md
  fi
  echo "" >> themes.md
  generate_options | ${ROFI_BIN} -theme-str "@theme \"${theme}\"" \
    -no-config -dmenu -p "mode" -show-icons \
      -u 3 -a 4 -mesg "Message box for extra information" \
      -take-screenshot-quit 1500 

  echo "![${NAME}](${NAME}.png)" >> themes.md
  echo "" >> themes.md
}

echo "# Included Themes" > themes.md

echo "Below is a list of themes shipped with rofi." >> themes.md
echo "Use \`rofi-theme-selector\` to select and use one of these themes." >> themes.md


Xvfb :1234 -screen 0 1920x1080x24 &
XEPHYR_PID=$!
export DISPLAY=:1234
sleep 0.5;
run_theme "default"
for theme in ${THEMES}
do
  run_theme ${theme}
done
kill ${XEPHYR_PID}

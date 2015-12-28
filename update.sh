#!/usr/bin/env bash

# Get copy from theme website

TREPO="temp-repo"

git clone https://github.com/DaveDavenport/rofi-themes "${TREPO}" 

if [ ! -d "${TREPO}" ]
then
    echo "Failed to checkout themes."
    exit 1
fi

echo "---" > p05-Themes.md
echo "layout: default" >> p05-Themes.md
echo "github: DaveDavenport" >> p05-Themes.md
echo "title: Themes" >> p05-Themes.md
echo "---" >> p05-Themes.md

cat "./${TREPO}/README.md" >> p05-Themes.md

cp -r "./${TREPO}/Screenshots" ./

git add p05-Themes.md
git add ./Screenshots/*.png

rm -rf "./${TREPO}"

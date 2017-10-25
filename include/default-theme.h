/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ROFI_DEFAULT_THEME
#define ROFI_DEFAULT_THEME

const char *default_theme =
    "* {"
    "    spacing:    2;"
    "    background-color:            transparent;"
    "    background:                  #FDF6E3FF;"
    "    foreground:                  #002B36FF;"
    "    border-color:                 @foreground;"
    "    separatorcolor:              @foreground;"
    "    red:                         #DC322FFF;"
    "    blue:                        #268BD2FF;"
    "    lightbg:                     #EEE8D5FF;"
    "    lightfg:                     #586875FF;"
    "    normal-foreground:           @foreground;"
    "    normal-background:           @background;"
    "    urgent-foreground:           @red;"
    "    urgent-background:           @background;"
    "    active-foreground:           @blue;"
    "    active-background:           @background;"
    "    selected-normal-foreground:  @lightbg;"
    "    selected-normal-background:  @lightfg;"
    "    selected-urgent-foreground:  @background;"
    "    selected-urgent-background:  @red;"
    "    selected-active-foreground:  @background;"
    "    selected-active-background:  @blue;"
    "    alternate-normal-foreground: @foreground;"
    "    alternate-normal-background: @lightbg;"
    "    alternate-urgent-foreground: @red;"
    "    alternate-urgent-background: @lightbg;"
    "    alternate-active-foreground: @blue;"
    "    alternate-active-background: @lightbg;"
    "}"
    "#window {"
    "    border:     1;"
    "    padding:    5;"
    "    background-color: @background;"
    "}"
    "#mainbox {"
    "    border:  0;"
    "    padding: 0;"
    "}"
    "#message {"
    "    border:  2px dash 0px 0px ;"
    "    padding: 2px 0px 0px ;"
    "    border-color: @separatorcolor;"
    "}"
    "#textbox {"
    "    text-color: @foreground;"
    "}"
    "#listview {"
    "    border-color: @separatorcolor;"
    "}"
    "#listview {"
    "    fixed-height: 0;"
    "    border:       2px dash 0px 0px ;"
    "    padding:      2px 0px 0px ;"
    "}"
    "#element {"
    "    border: 0;"
    "}"
    "#element normal.normal {"
    "    text-color: @normal-foreground;"
    "    background-color: @normal-background;"
    "}"
    "#element normal.urgent {"
    "    text-color: @urgent-foreground;"
    "    background-color: @urgent-background;"
    "}"
    "#element normal.active {"
    "    text-color: @active-foreground;"
    "    background-color: @active-background;"
    "}"
    "#element selected.normal {"
    "    text-color: @selected-normal-foreground;"
    "    background-color: @selected-normal-background;"
    "}"
    "#element selected.urgent {"
    "    text-color: @selected-urgent-foreground;"
    "    background-color: @selected-urgent-background;"
    "}"
    "#element selected.active {"
    "    text-color: @selected-active-foreground;"
    "    background-color: @selected-active-background;"
    "}"
    "#element alternate.normal {"
    "    text-color: @alternate-normal-foreground;"
    "    background-color: @alternate-normal-background;"
    "}"
    "#element alternate.urgent {"
    "    text-color: @alternate-urgent-foreground;"
    "    background-color: @alternate-urgent-background;"
    "}"
    "#element alternate.active {"
    "    text-color: @alternate-active-foreground;"
    "    background-color: @alternate-active-background;"
    "}"
    "#scrollbar {"
    "    border:  0;"
    "    width: 4px;"
    "    padding: 0;"
    "    handle-color: @normal-foreground;"
    "}"
    "#sidebar {"
    "    border:       2px dash 0px 0px ;"
    "    border-color: @separatorcolor;"
    "}"
    "#button selected {"
    "    background-color: @selected-normal-background;"
    "    text-color:       @selected-normal-foreground;"
    "}"
    "#inputbar, case-indicator, entry, prompt, button {"
    "    spacing: 0;"
    "    text-color:      @normal-foreground;"
    "}";
#endif

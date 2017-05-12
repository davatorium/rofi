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
    "    background:                  #FFFDF6E3;"
    "    foreground:                  #FF002B36;"
    "    bordercolor:                 @foreground;"
    "    separatorcolor:              @foreground;"
    "    red:                         #FFDC322F;"
    "    blue:                        #FF268BD2;"
    "    lightbg:                     #FFEEE8D5;"
    "    lightfg:                     #FF586875;"
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
    "    foreground: @foreground;"
    "    background: #00000000;"
    "    padding:    5;"
    "}"
    "#window.box {"
    "    background: @background;"
    "    foreground: @bordercolor;"
    "}"
    "#window.mainbox {"
    "    border:  0;"
    "    padding: 0;"
    "}"
    "#window.mainbox.message.box {"
    "    border:  1px dash 0px 0px ;"
    "    padding: 2px 0px 0px ;"
    "    foreground: @separatorcolor;"
    "}"
    "#window.mainbox.message.normal {"
    "    foreground: @foreground;"
    "}"
    "#window.mainbox.listview {"
    "    fixed-height: 0;"
    "    border:       1px dash 0px 0px ;"
    "    padding:      2px 0px 0px ;"
    "}"
    "#window.mainbox.listview.element {"
    "    border: 0;"
    "}"
    "#window.mainbox.listview.element.normal.normal {"
    "    foreground: @normal-foreground;"
    "    background: @normal-background;"
    "}"
    "#window.mainbox.listview.element.normal.urgent {"
    "    foreground: @urgent-foreground;"
    "    background: @urgent-background;"
    "}"
    "#window.mainbox.listview.element.normal.active {"
    "    foreground: @active-foreground;"
    "    background: @active-background;"
    "}"
    "#window.mainbox.listview.element.selected.normal {"
    "    foreground: @selected-normal-foreground;"
    "    background: @selected-normal-background;"
    "}"
    "#window.mainbox.listview.element.selected.urgent {"
    "    foreground: @selected-urgent-foreground;"
    "    background: @selected-urgent-background;"
    "}"
    "#window.mainbox.listview.element.selected.active {"
    "    foreground: @selected-active-foreground;"
    "    background: @selected-active-background;"
    "}"
    "#window.mainbox.listview.element.alternate.normal {"
    "    foreground: @alternate-normal-foreground;"
    "    background: @alternate-normal-background;"
    "}"
    "#window.mainbox.listview.element.alternate.urgent {"
    "    foreground: @alternate-urgent-foreground;"
    "    background: @alternate-urgent-background;"
    "}"
    "#window.mainbox.listview.element.alternate.active {"
    "    foreground: @alternate-active-foreground;"
    "    background: @alternate-active-background;"
    "}"
    "#window.mainbox.listview.scrollbar {"
    "    border:  0;"
    "    padding: 0;"
    "}"
    "#window.mainbox.sidebar.box {"
    "    border: 1px dash 0px 0px ;"
    "}"
    "#window.mainbox.inputbar {"
    "    spacing: 0;"
    "}"
    "#window.mainbox.inputbar.box {"
    "     border: 0px 0px 0px 0px;"
    "     "
    "}"
    "#window.mainbox.inputbar.normal {"
    "    foreground: @foreground;"
    "}";
#endif

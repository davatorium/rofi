# 1.7.8

## Fix drawing issue

In the last release we had some code that hit a bug in some graphics drivers.
The fix that went in had a side-effect causing borders not to be drawn as
expected. This release should fix that.

Issue: #2076

## DBusActivatable

The previously introduced DBusActivatable seems to cause some issues and
confusion. First if the application (dconf-editor looking at you) does not
acknowledge it launched rofi stays open until it times out. By default this is
15 seconds. We reduced the timeout to 1.5 seconds and added an option to
disable DBusActivatable.

The 2nd point is, that if you launch an application via DBusActivatable it is
not launched by rofi and does not inherits rofi's environment, but the one
systemd is configured to use for that user.
The archlinux documentation explains this and how to fix this [here](https://wiki.archlinux.org/title/Systemd/User#Environment_variables).

Issue: #2077

## CI Fixes

Because of some deprecation, the CI scripts are updated. It might be useful for
people who want to help to test the latest rofi that for every commit an
artifact is build with a 'source package' you can install.

You can find them
[here](https://github.com/davatorium/rofi/actions/workflows/build.yml) by
clicking on the commit you want and scrolling to the bottom of the page. This
contains a zip with the two normal source tarballs.

## Changelog

* Fix buffer overflow in rofi -e after reading from stdin (#2082) (Thanks to
Faule Socke)
* [DRun] Reduce DBus timeout to 1500ms add option to disable DBusActivatable.
  Issue: #2077
* [CI] Do explicit compare with 'true'?
* [CI] Fix typo in conditional.
* [CI] Only make dist on one build.
* [CI] Fix identical name?
* [CI] Bump upload action to v4
* [Widget] Actually remove ADD operator from border drawing.
  Issue: #2076
* [widget] Remove the ADD operator.
  Issue: #2076
* Add 1.7.7 docs to README.
* Mark as dev version

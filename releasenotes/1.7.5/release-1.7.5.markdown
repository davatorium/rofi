# 1.7.5:  We shell overcome

A quick bug-fix release to fix 2 small issues.

* In DMenu sync mode, the separator is left in the string.
* On special crafted delayed input in dmenu it shows updates to the list very slow.
  It now forces to push update (if there) every 1/10 of a second.
* In the view some of the update/redraw policies are fixed to reduced the
  perceived delay.

This makes it clear we need more people testing the development version to
catch these bugs. I only use a very limited set of features myself and do not
have the time nor energy to write and maintain a good automated test setup.
There was one in the past that tested some basic features by running rofi,
inputting user input and validating output. But it was not reliable and
difficult to keep running.


# Thanks

Big thanks to everybody reporting issues.
Special thanks goes to:

* Iggy
* Morgane Glidic
* Danny Colin

Apologies if I mistyped or missed anybody.

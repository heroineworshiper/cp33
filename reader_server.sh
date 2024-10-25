#!/bin/sh


export DISPLAY=:0
/usr/bin/fvwm2&
xinput --set-prop "McLionhead Trackpad" "libinput Accel Speed" 0

while [ 1 ]; do /root/reader; sleep 1; done



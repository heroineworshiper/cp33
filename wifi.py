#!/usr/bin/python



# start up wpa_supplicant, wait for a connection, then start up dhclient


import os
import sys
import subprocess


p = subprocess.Popen(['wpa_supplicant', 
        '-iwlan0', 
        '-c/etc/wpa_supplicant/wpa_supplicant.conf'],
    stdout=subprocess.PIPE)


while True:
    line = p.stdout.readline()
    print 'Got: ', line
    if "CTRL-EVENT-CONNECTED" in line:
        # start DHCP
        subprocess.call(['dhclient', '-v', 'wlan0'])




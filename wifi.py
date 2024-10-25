#!/usr/bin/python



# start up wpa_supplicant, wait for a connection, then start up dhclient


import os
import sys
import subprocess
import time

subprocess.call(['killall', '-9', 'wpa_supplicant'])
subprocess.call(['killall', '-9', 'dhclient'])
time.sleep(1)


p = subprocess.Popen(['wpa_supplicant', 
        '-iwlan0', 
        '-c/etc/wpa_supplicant/wpa_supplicant.conf'],
    stdout=subprocess.PIPE)


while True:
    line = p.stdout.readline()
    print 'Got: ', line
    if "CTRL-EVENT-CONNECTED" in line:
        # start DHCP
        subprocess.call(['killall', '-9', 'dhclient'])
        time.sleep(1)
        subprocess.call(['dhclient', '-v', 'wlan0'])
        print("Ran dhclient.  DNS:")
        subprocess.call(['cat', '/etc/resolv.conf'])



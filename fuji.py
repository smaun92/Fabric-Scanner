#!/usr/bin/env python
# This program is used to mimic AEB(Auto-Exposure Bracketing) with Fujifilm GFX100s
# It awaits signal from arduino and than trigger the camera and change shutterspeed upon receiving signal
# 19/5/2022

import RPi.GPIO as GPIO
#import logging
import os
import subprocess
import sys
import time
import gphoto2 as gp

GPIO.setmode(GPIO.BCM)
GPIO.setup(12, GPIO.IN)
GPIO.setup(12, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(24, GPIO.IN)
GPIO.setup(24, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

aeb = GPIO.input(12)
wake = GPIO.input(24)

print("Bootup")

while True:
    if GPIO.input(12):
        #time.sleep(2)
        #if GPIO.input(12):
            
            
            from subprocess import call
            
            os.system("sudo uhubctl -l 1-1 -a 0")
            os.system("sudo uhubctl -l 1-1 -a 1")
            time.sleep(5)
            
            call(["gphoto2", "--set-config", "shutterspeed=2s"])
            #time.sleep(2)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(2)
            call(["gphoto2", "--set-config", "shutterspeed=13s"])
            time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(3)
            call(["gphoto2", "--set-config", "shutterspeed=5s"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(2)
            call(["gphoto2", "--set-config", "shutterspeed=3s"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(2)
            call(["gphoto2", "--set-config", "shutterspeed=1.3s"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=0.8s"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/2"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            #call(["gphoto2", "--wait-event=1s"])
            
            os.system("sudo uhubctl -l 1-1 -a 0")  
            os.system("sudo uhubctl -l 1-1 -a 1")
            time.sleep(5)
            
            call(["gphoto2", "--set-config", "shutterspeed=1/3"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/5"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/8"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/10"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/15"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/25"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/40"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=1/50"])
            #time.sleep(1)
            call(["gphoto2", "--trigger-capture"])
            time.sleep(1)
            call(["gphoto2", "--set-config", "shutterspeed=2s"])
            #time.sleep(1)
                
            os.system("sudo uhubctl -l 1-1 -a 0")
            os.system("sudo uhubctl -l 1-1 -a 1")
            #time.sleep(60)
            #os.system("sudo uhubctl -l 1-1 -a 0")
            #os.system("sudo uhubctl -l 1-1 -a 1")"""
            
    elif GPIO.input(24):
        #time.sleep(2#
        #if GPIO.input(24):
            print("Button 2 is pressed")
            os.system("sudo uhubctl -l 1-1 -a 0")
            os.system("sudo uhubctl -l 1-1 -a 1")         
            

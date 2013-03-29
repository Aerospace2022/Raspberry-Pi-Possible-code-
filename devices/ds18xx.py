#!/usr/bin/python

import time
from os import system
from protocols import i2c

class DS18XXFamily:
    (DS18B20,DS1822) = range(2)
    
class DS18XX:
    def __init__(self, family, serial):
        familyCode = 0
        if family == DS18XXFamily.DS18B20:
            familyCode = '28'
        elif family == DS18XXFamily.DS1822:
            familyCode = '22'
        else:
            print "ERROR"
            return
        self.path = """/sys/bus/w1/devices/%s-%s/w1_slave""" % (familyCode,serial)
        
        system('sudo modprobe w1-gpio')
        system('sudo modprobe w1-therm')
    
    def read(self):
        tempFile = open(self.path)
        tempText = tempFile.read()
        tempFile.close()
        
        temperatureData = tempText.split("\n")[1].split(" ")[9]
        temperature = float(temperatureData[2:]) / 1000.0
        
        return temperature
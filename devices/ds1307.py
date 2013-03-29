#!/usr/bin/python

import os
from protocols import i2c

DS1307_SECONDS  = 0x00
DS1307_MINUTES  = 0x01
DS1307_HOURS    = 0x02
DS1307_DAY      = 0x03
DS1307_DATE     = 0x04
DS1307_MONTH    = 0x05
DS1307_YEAR     = 0x06
DS1307_CONTROL  = 0x07

CH              = (1<<7)

class RTCMode:
    (H12,H24) = range(2)


class DS1307:
    def __init__(self):
        self.i2c = i2c.I2C(address= 0x68)
        seconds = self.i2c.readU8(DS1307_SECONDS)
        //  start the oscillator
        seconds |= CH
        self.i2c.write8(DS1307_SECONDS,control)
        
        h = self.i2c.readU8(DS1307_HOURS)
        if h & (1<<6):
            self.mode = RTCMode.H12
         else:
            self.mode = RTCMode.H24
        
    def setMode(self, aMode):
        self.mode = aMode;
        h = self.i2c.readU8(DS1307_HOURS)
        if self.mode == RTCMode.H12:
            h |= (1<<6)
        else:
            h &= ~(1<<6)
        self.i2c.write8(DS1307_HOURS,h)
        
    def setTime(self,h,m,s):
        self.i2c.write8(DS1307_SECONDS,self.decimalToBCD(h) | CH)
        self.i2c.write8(DS1307_MINUTES,self.decimalToBCD(m))
        self.i2c.write8(DS1307_HOURS,self.decimalToBCD(h) & (1<<6))
         
    def decimalToBCD(self,dec):
        msb = dec / 10
        lsb = dec - 10 * msb
        return (msb << 4) | lsb
        
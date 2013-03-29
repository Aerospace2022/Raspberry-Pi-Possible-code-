#!/usr/bin/python

import time
from protocols import i2c

""" Register addresses """
TMP102_TEMP     = 0x00    #   readonly
TMP102_CONFIG   = 0x01    #   R/W
TMP102_TLOW     = 0x02    #   R/W
TMP102_THIGH    = 0x03    #   R/W

class TMP102:
    def __init__(self, address):
        self.i2c = i2c.I2C(address = address)
        self.address = address
        
    def readTemp(self):
        rawtemp = self.i2c.readU16(TMP102_TEMP)
        msb = rawtemp >> 8
        lsb = rawtemp & ~(0xF0)
        
        val = msb
        if msb & 0x80 > 0:
            val |= 0x0F00
        val <<= 4
        val |= (lsb >> 4)
        convertedTemp = val * 0.0625
        
        return convertedTemp
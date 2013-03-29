#!/usr/bin/python

import time
from protocols import i2c

class ADC:
	def __init__(self, address):
		self.i2c = i2c.I2C(address = address)
		self.address = address
		
	def readChannel(self, channel):
		val_list = self.i2c.readList(channel,2)
		return val_list[0] + val_list[1]**8
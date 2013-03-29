#!/usr/bin/python

import time
from protocols import i2c

"""
Register addresses
"""
MCP23008_IODIR      = 0x00
MCP23008_IPOL       = 0x01
MCP23008_GPINTEN    = 0x02
MCP23008_DEFVAL     = 0x03
MCP23008_INTCON     = 0x04
MCP23008_IOCON      = 0x05
MCP23008_GPPU       = 0x06
MCP23008_INTF       = 0x07
MCP23008_INTCAP     = 0x08
MCP23008_GPIO       = 0x09
MCP23008_OLAT       = 0x0A

MCP23017_IODIRA     = 0x00
MCP23017_IODIRB     = 0x01
MCP23017_IPOLA      = 0x02
MCP23017_IPOLB      = 0x03
MCP23017_GPINTENA   = 0x04
MCP23017_GPINTENB   = 0x05
MCP23017_DEFVALA    = 0x06
MCP23017_DEFVALB    = 0x07
MCP23017_INTCONA    = 0x08
MCP23017_INTCONB    = 0x09
MCP23017_IOCON      = 0x0A
MCP23017_IOCON      = 0x0B
MCP23017_GPPUA      = 0x0C
MCP23017_GPPUB      = 0x0D
MCP23017_INTFA      = 0x0E
MCP23017_INTFB      = 0x0F
MCP23017_INTCAPA    = 0x10
MCP23017_INTCAPB    = 0x11
MCP23017_GPIOA      = 0x12
MCP23017_GPIOB      = 0x13
MCP23017_OLATA      = 0x14
MCP23017_OLATB      = 0x15

class MCP230XXDirection:
    (OUTPUT,INPUT) = range(2)
    
class MCP23017:
    
    def __init__(self, address):
        self.i2c = i2c.I2C(address = address)
        self.address = address
        
        self.i2c.write8(MCP23017_IODIRA, 0xFF)  # all inputs on PORTA
        self.i2c.write8(MCP23017_IODIRB, 0xFF)  # all inputs on PORTB
        
        """ read directions """
        self.iodira = self.i2c.readU8(MCP23017_IODIRA)
        self.iodirb = self.i2c.readU8(MCP23017_IODIRB)
        
        """ disable pullups """
        self.i2c.write8(MCP23017_GPPUA, 0x00)
        self.i2c.write8(MCP23017_GPPUB, 0x00)
        
    def config(self, bank, pin, mode):
        """ read the current value of the direction registers """
        self.iodira = self.i2c.readU8(MCP23017_IODIRA)
        self.iodirb = self.i2c.readU8(MCP23017_IODIRB)
        if mode == MCP230XXDirection.INPUT:
            if bank == 'A':
                self.iodira |= 1 << pin
            else:
                self.iodirb |= 1 << pin
        else:
            if bank == 'A':
                self.iodira &= ~(1 << pin)
            else:
                self.iodirb &= ~(1 << pin)
        """ write the new configuration """
        if bank == 'A':
            self.i2c.write8(MCP23017_IODIRA, self.iodira)
        else:
            self.i2c.write8(MCP23017_IODIRB, self.iodirb)
        
    def output(self, bank, pin, value):
        if pin > 7:
            return
        
        """ get current value for the bank """
        if bank == 'A':
            self.gpioa = self.i2c.readU8(MCP23017_GPIOA)
            """ alter with current value """
            if value == 1:
                self.gpioa |= 1 << pin
            else:
                self.gpioa &= ~(1 << pin)
            """ write new value """
            self.i2c.write8(MCP23017_GPIOA,self.gpioa)
        else:
            self.gpiob = self.i2c.readU8(MCP23017_GPIOB)
            if value == 1:
                self.gpiob |= (1 << pin)
            else:
                self.gpiob &= ~(1 << pin)
            """ write new value """
            self.i2c.write8(MCP23017_GPIOB,self.gpiob)
    def input(self, bank, pin):
        if pin > 7:
            return
        
        data = 0
        if bank == 'A':
            data = self.i2c.readU8(MCP23017_GPIOA)
        else:
            data = self.i2c.readU8(MCP23017_GPIOB)
            
        return (data >> pin) & 0x1

class MCP23008:
    OUTPUT = 0
    INPUT = 1
    
    def __init__(self, address):
        self.i2c = i2c.I2C(address = address)
        self.address = address
        
        self.i2c.write8(MCP23008_IODIR, 0xFF)   # all inputs
        self.iodir = self.i2c.readU8(MCP23008_IODIR)
        
        """ all pullups disabled """
        self.i2c.write8(MCP23008_GPPU, 0)
    
    def config(self, pin, mode):
        """ read the current value of IODIR """
        self.iodir = self.i2c.readU8(MCP23008_IODIR)
        if mode == MCP230XXDirection.INPUT:
            self.iodir |= 1 << pin
        else:
            self.iodir &= ~(1 << pin)
        
        """ write the new configuration """
        self.i2c.write8(MCP23008_IODIR, self.iodir)
        
    def output(self, pin, value):
        if pin > 7:
            return
        
        self.gpio = self.i2c.readU8(MCP23008_GPIO)
        
        """ alter with current value """
        if value == 1:
            self.gpio |= 1 << pin
        else:
            self.gpio &= ~(1 << pin)
        
        """ write the new value """
        self.i2c.write8(MCP23008_GPIO, self.gpio)
        
    def input(self, pin):
        if pin > 7:
            return
        
        data = self.i2c.readU8(MCP23008_GPIO)
        return (data >> pin) & 0x1
        
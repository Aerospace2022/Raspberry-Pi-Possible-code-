#!/usr/bin/python

from protocols  import i2c
from devices import bmp085
from devices import mcp2300x
from devices import ds18xx
from devices import adc
from devices import gps
from devices import camera
import threading
import os
import re
import sqlite3 as sql
import datetime as utcclock
import time
from collections import deque

""" Configuration """
HELIUM_VERSION          = '0.1.0'
DEBUG                   = 1

""" Device addresses """
I2C_ADDRESS_BMP085      = 0x77
I2C_ADDRESS_MCP23017    = 0x20
I2C_ADDRESS_ADC             = 0x26

""" DS18B20 identifiers """
DS18B20_ID_INTERNAL     = '0000025edf21'
DS18B20_ID_EXTERNAL             = '0000025ef092'

class HeliumMode:
    (ModePreflight,ModeStart,ModeAscent,ModeDescent) = range(4)

class Hardware:
    def __init__(self):
        if DEBUG:
            print """INFO | initializing altimeter (BMP085 @ 0x%x)""" % (I2C_ADDRESS_BMP085)
        self.bmp = bmp085.BMP085(I2C_ADDRESS_BMP085)

        if DEBUG:
            print """INFO | initializing external GPIO (MCP23017 @ 0x%x""" % (I2C_ADDRESS_MCP23017,)
        self.gpio = mcp2300x.MCP23017(I2C_ADDRESS_MCP23017)
        
        """ configure the external GPIO """
        MCP23017Direction = mcp2300x.MCP230XXDirection.OUTPUT
        for pin in range(8):
            self.gpio.config('A',pin,MCP23017Direction)
            self.gpio.config('B',pin,MCP23017Direction)
            
        if DEBUG:
            print """INFO | set GPS serial redirect to APSR"""
        self.gpio.output('B',6,0)
        self.gpio.output('B',7,0)

        """ configure 1-wire temp sensors """
        if DEBUG:
            print "INFO | initializing 1-Wire devices"
        DSDeviceFamily = ds18xx.DS18XXFamily.DS18B20
        self.extTemp = ds18xx.DS18XX(DSDeviceFamily,DS18B20_ID_EXTERNAL)
        self.intTemp = ds18xx.DS18XX(DSDeviceFamily,DS18B20_ID_INTERNAL)

        

        """ configure the ADC """
        if DEBUG:
            print "INFO | initializing the ADC"
        self.adc = adc.ADC(0x26);

        """     configure the GPS """
        if DEBUG:
            print "INFO | initializing the GPS"
        self.gps = gps.GPS()

        """ configure the camera """
        if DEBUG:
            print "INFO | initializing the camera"
        """ ensure that image path exists """
        imagepath = '/mnt/FLASH_DRIVE'
        if os.path.exists(imagepath):
            if os.path.isdir(imagepath):
                if os.access(imagepath, os.W_OK):
                    print "INFO | Image path is OK"
                else:
                    print "WARN | Image path is not writable"
            else:
                print "WARN | Image path is not a directory"
        else:
            print "WARN | Image path does not exist"
        self.camera = camera.Camera('/mnt/FLASH_DRIVE')
        if self.camera.isavailable():
            print "INFO | Camera is available"
        else:
            print "WARN | Camera is not available"


    def readAltitude(self):
        return self.bmp.readAltitude()

    def readPressure(self):
        return self.bmp.readPressure()

    def readBMPTemp(self):
        return self.bmp.readTemperature()

    def readExternalTemp(self):
        return self.extTemp.read()

    def readInternalTemp(self):
        return self.intTemp.read()

    def readHumidity(self):
        rawADC = self.adc.readChannel(2)
        #print rawADC
        volts = 3.3 * float(rawADC)/1023.0
        return volts/0.03068

class CPU:
    def readCPUTemp(self):
        t = os.popen('cat /sys/class/thermal/thermal_zone0/temp').read()
        return t

class DB:
    def __init__(self):
        self.con = None
    def connect(self):
        self.con = sql.connect('/var/www/webpy/data/helium.db')
    def getVersion(self):
        cursor = self.con.cursor()
        cursor.execute('SELECT SQLITE_VERSION()')
        data = cursor.fetchone()
        self.con.close()
        return data

class Menu:
    def getOption(self):
        opt = raw_input('HELIUM> ')
        return opt

    def printHelp(self):
        print "pa\tPrint altitude in meters"
        print "pp\tPrint barometric pressure in pascal"
        print "slp n t\tPrint sea-level pressure for given altitude (m) & temp (C)"
        print "pte\tPrint external temperature in degrees C"
        print "pti\tPrint internal temperature in degrees C"
        print "pct\tPrint CPU temperature in degrees C"
        print "pat\tPrint ADC temperature in degrees C"
        print "prh\tPrint relative humidity in %"
        print "cap\tCapture test image from camera"
        print "sqv\tPrint the sqlite version"
        print "start\tStart launch sequence"
        print "v\tPrint software version"
        print "quit\tExit the helium shell"


    def calculateSLP(self,P,h,T):
        scaledHeight = 0.0065 * float(h)
        bottomSum = float(T) + scaledHeight + 273.15
        inner = 1 - (scaledHeight/bottomSum)
        innerRaised = inner ** -5.257
        return P * innerRaised
    
class GPSRedirect:
    (CPU,APRS) = range(2)

print """WELCOME TO HELIUM VERSION %s\n""" % (HELIUM_VERSION,)
print """The time is %s UTC""" % utcclock.datetime.today()
#radio = i2c.I2C(address = 0x48)
#radio.bus.write_i2c_block_data(0x48,1,)
#radio.write8(0x51, 0x54)
#print radio.readS8(0x48)

cpu = CPU()
hw = Hardware()
menu = Menu()
db = DB()
gpsredirect = GPSRedirect.APRS
mode = HeliumMode.ModePreflight         # begin in preflight mode
gpsalts = deque([])                                     # gps altitudes
sensoralts = deque([])                          # sensor altitudes
ALTITUDE_STACK_SIZE = 20                        # size of altitude stacks

cputemp = 0             #       temperature of RPi CPU
adctemp = 0             #       temperature of ADC chip
extemp = 0              #       exterior temperature
intemp = 0              #       interior payload temperature
bmptemp = 0             #       temperature from the BMP085 sensor
humid = 0               #       % relative humidity
bmp = 0                 #       barometric pressure
slp = 0                 #       sea-level pressure
accelx = 0              #       acceleration X-axis
accely = 0              #       acceleration Y-axis
accelz = 0              #       acceleration Z-axis

"""     local GPS data"""
latitude = 0
longitude = 0
altitude = 0
kts = 0
trkangle = 0
trkmag = 0
gpstime = utcclock.datetime.today()
satcount = 0
quality = 0


""" MENU HANDLERS """

def printAltitude():
    print "%0.2f" % hw.readAltitude()
def printPressure():
    print "%0.2f" % hw.readPressure()
def printExternalTemp():
    print "%0.2f" % hw.readExternalTemp()
def printInternalTemp():
    print "%0.2f" % hw.readInternalTemp()
def printCPUTemp():
    t = float(cpu.readCPUTemp())/1000
    print "%0.2f" % t
def printADCTemp():
    print hw.adc.readChannel(0x3F)
def printRelativeHumidity():
    print "%0.2f" % hw.readHumidity()
def printSQLiteVersion():
    db.connect()
    print db.getVersion()
def captureTestImage():
    ImageCaptureThread = threading.Thread(target=hw.camera.captureimage())
    ImageCaptureThread.start()
def startLaunch():
    print "Starting run sequence"
    global mode
    mode = HeliumMode.ModeStart

def printVersion():
    print HELIUM_VERSION
def exitApp():
    exit(1)

optionVectors = {
        'help': menu.printHelp,
        'pa'  : printAltitude,
        'pp'  : printPressure,
        'pte' : printExternalTemp,
        'pti' : printInternalTemp,
        'pct' : printCPUTemp,
        'pat' : printADCTemp,
        'prh' : printRelativeHumidity,
        'sqv' : printSQLiteVersion,
        'cap' : captureTestImage,
        'start' : startLaunch,
        'v'   : printVersion,
        'x'   : exitApp,
        'quit': exitApp
}

def measureExtTemp():
    global extemp
    extemp = hw.readExternalTemp()

def measureIntTemp():
    global intemp
    intemp = hw.readInternalTemp()

now = utcclock.datetime.now()
ept = (time.mktime(now.timetuple()))
epochtime = 0
gpslistentime = 0



def extTempMonitorService():
    """     service the exterior temperature monitor """
    ReadExtTempThread = threading.Thread(target=measureExtTemp)
    ReadExtTempThread.start()


def intTempMonitorService():
    """ service the interior temperature monitor """
    ReadIntTempThread = threading.Thread(target=measureIntTemp)
    ReadIntTempThread.start()

def cpuTempMonitorService():
    """ service the CPU temperature monitor """
    cputemp = float( cpu.readCPUTemp())/1000.0

def humidityMonitorService():
    """ service the humidity sensor """
    humidity = hw.readHumidity()

def bmpMonitorService():
    """ service the barometric pressure sensor """
    global currentalt,bmp,bmptemp,sersoralts,slp
    currentalt = hw.readAltitude()
    bmp = hw.readPressure()
    bmptemp = hw.readBMPTemp()
    sensoralts.append(currentalt)
    if len(sensoralts) > ALTITUDE_STACK_SIZE:
        sensoralts.popleft()
    slp = menu.calculateSLP(bmp,currentalt,extemp)

def saveSensorData():
    """ dump accumulated sensor data to sqlite db"""
    if len(sensoralts) > 0:
        query = """INSERT INTO sensors(time,oat,iat,cput,bmpt,rh,accelx,accely,accelz,alt,bp,slp) \
        VALUES('%s','%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%0.1f')""" % \
        (utcclock.datetime.now(),extemp,intemp,cputemp,bmptemp,humid,accelx,accely,accelz,sensoralts[-1],bmp,slp)
        #print query
        try:
            con = sql.connect('/var/www/webpy/data/helium.db')
            cursor = con.cursor()
            cursor.execute(query)
            con.commit()
        except sql.Error, e:
            print "Error %s:" % e.args[0]
            sys.exit(1)
        finally:
            if con:
                con.close()

def saveGPSData():
    """ dump accumulated position data to sqlite db """
    query = "INSERT INTO fixes(latitude,longitude,altitude,kts,trkangle,trkmag,time,quality,satcount) \
                    VALUES('%0.6f','%0.6f','%0.1f' , '%0.1f' ,'%0.1f' , '%0.1f' ,'%s' , '%d' , '%d')" % \
                    (latitude,longitude,altitude,kts,trkangle,trkmag,utcclock.datetime.now(),quality,satcount)
    try:
        con = sql.connect('/var/www/webpy/data/helium.db')
        cursor = con.cursor()
        cursor.execute(query)
        con.commit()
    except sql.Error, e:
        print "Error %s:" % e.args[0]
        sys.exit(1)
    finally:
        con.close()
        
def listenToGPS():
    self.gpio.output('B',6,1)
    self.gpio.output('B',7,0)
    """ we'll listen for 2 seconds then allow the APRS tracker to listen for the remaining 8 seconds """
    gpslistentime = epochtime

""" capture image from the camera and store on USB stick in separate thread """
def captureImage():
    ImageCaptureThread = threading.Thread(target=hw.camera.captureimage())
    ImageCaptureThread.start()

""" since we need not perform every task on each trip through the run loop, this structure
sets up a mechanism for launching tasks at intervals
"""
serviceevents = [ {'interval':15, 'time':0,     'vector' : extTempMonitorService},
                                  {'interval':15, 'time':ept+1, 'vector' : intTempMonitorService},
                                  {'interval':15, 'time':ept+2, 'vector' : cpuTempMonitorService},
                                  {'interval':15, 'time':ept+3, 'vector' : humidityMonitorService},
                                  {'interval':15, 'time':ept+4, 'vector' : bmpMonitorService},
                                  {'interval':60, 'time':0,     'vector' : captureImage},
                                  {'interval':12, 'time':ept+5, 'vector' : saveSensorData},
                                  {'interval':5,  'time':ept+2, 'vector' : saveGPSData,
                                   'interval':10, 'time':ept+6, 'vector' : listenToGPS,}
                                ]

while 1:
    now = utcclock.datetime.now()
    epochtime = (time.mktime(now.timetuple()))
    """ in preflight mode look for commands from console """
    if mode == HeliumMode.ModePreflight:
        option = menu.getOption()
        if option != None:
            slpPattern = 'slp\s+(\d+)\s+(-?\d+)'
            m = re.search(slpPattern, option)
            if m != None:
                P = int(hw.readPressure())/100
                slp = menu.calculateSLP(P,m.group(1),m.group(2))
                print "%0.2f" % slp
            else:
                try:
                    vector = optionVectors[option]
                except KeyError as err:
                    print "UNRECOGNIZED CMD.  Try 'help' for usage"
                else:
                    if vector == None:
                        print "UNRECOGNIZED COMMAND. Try 'help' to get a list of commands"
                    else:
                        vector()
    else:
        """     in some flight mode """
        for event in serviceevents:
            if epochtime - event['time'] > event['interval']:
                event['vector']()
                event['time'] = epochtime

        """ service the GPS every pass through the run loop"""
        if gpsredirect == GPSRedirect.CPU:
            """ we're listening to the GPS serial currently"""
            hw.gps.readline()
            if hw.gps.lastMessage.sentenceType == 'VTG':
                trkangle = hw.gps.lastMessage.trueTrack
                trkmag = hw.gps.lastMessage.magneticTrack
                kts = hw.gps.lastMessage.groundSpeedKnots
            elif hw.gps.lastMessage.sentenceType == 'GGA':
                latitude = hw.gps.lastMessage.fix.latitude
                longitude = hw.gps.lastMessage.fix.longitude
                altitude = hw.gps.lastMessage.altitude
                gpsalts.append(altitude)
                if len(gpsalts) > ALTITUDE_STACK_SIZE:
                    gpsalts.popleft()
            if epochtime - gpslistentime > 2:
                """ if we've already listened for two seconds then redirect back to APRS"""
                gpsredirect = GPSRedirect.APRS
                """ switch the multiplexer to the APRS """
                hw.gpio.output('B',6,0)
                hw.gpio.outout('B',7,0)

            #adctemp = hw.adc.readChannel(0x3F)

#!/usr/bin/env python

import serial
import re
from datetime import tzinfo, timedelta, datetime

""" GPSMesage corresponds to an NMEA sentence """
"""	This class is not a full implementation of an NMEA parser
	Rather, it is intended to parse key data from the following high altitude
	GPS: UniTraQ GPS Module GT-320FW(AS) which is available from Argent Data Systems
"""
class GPSMessage:
	def __init__(self,text):
		self.text = text
		self.sentenceType = ''
		if self.text[0:3] == '$GP':
			self.text = self.text[3:]
			self.list = self.text.split(',')
			self.sentenceType = self.list[0]
	"""	parse an RMC message: Recommended minimum data """
	def _parseRMC(self):
		self.date = GPSDate(self.list[1],self.list[9])
		#print self.date.time
		self.fix = GPSFix(self.list[3],self.list[4],self.list[5],self.list[6])
		self.speed = float(self.list[7])
		self.trackAngle = float(self.list[8])
	
	""" parse a GGA message: Fix information """
	def _parseGGA(self):
		self.fix = GPSFix(self.list[2],self.list[3],self.list[4],self.list[5])
		self.quality = int(self.list[6])
		self.satelliteCount = int(self.list[7])
		self.altitude = float(self.list[9])
		
	""" parse a GSA message:  satellite data """
	"""	note that this is incomplete """
	def _parseGSA(self):
		self.fixType = self.list[2]
		
	""" parse a VTG message: velocity made good """
	def _parseVTG(self):
		self.trueTrack = float(self.list[1])
		self.magneticTrack = float(self.list[3])
		self.groundSpeedKnots = float(self.list[5])
		self.groundSpeecKmh = float(self.list[7])
		
	""" parse a ZDA message: date/time """
	def _parseZDA(self):
		rawTime = self.list[1]
		hh = int(rawTime[0:1])
		mm = int(rawTime[2:3])
		ss = int(rawTime[4:5])
		
		day = int(self.list[2])
		month = int(self.list[3])
		year = int(self.list[4])
		
		utc = UTC()
		self.time = datetime(year,month,day,hh,mm,ss,tzinfo=utc)
		
	def parse(self):
		self.vectors = {'RMC' : self._parseRMC,
		'GGA' : self._parseGGA,
		'GSA' : self._parseGSA,
		'VTG' : self._parseVTG,
		'ZDA' : self._parseZDA
		}
		if self.sentenceType in self.vectors:
			self.vectors[self.sentenceType]()
		
class UTC(tzinfo):
	"""UTC"""
	
	def utcoffset(self, dt):
		return timedelta(0)
	def tzname(self, dt):
		return "UTC"
	def dat(self, dt):
		return timedelta(0)

			
class GPSDate:
	def __init__(self,rawTime,rawDate):
		utc = UTC()
		rawYear = int(rawDate[4:6])
		if rawYear > 82:
			year = rawYear + 1900
		else:
			year = rawYear + 2000
		month = int(rawDate[2:4])
		day = int(rawDate[0:2])
		hour = int(rawTime[0:2]);
		minute = int(rawTime[2:4])
		second = int(rawTime[4:6])
		self.time = datetime(year,month,day,hour,minute,second,tzinfo=utc)
		
class GPSFix:
	def __init__(self,latnum,latdir,longnum,longdir):
		latitudeObj = GPSLatitude(latnum,latdir)
		longitudeObj = GPSLongitude(longnum, longdir)
		self.latitude = latitudeObj.value
		self.longitude = longitudeObj.value
		
class GPSFixComponent:
	def __init__(self,number,direction):
		self.degrees = int(number[0:2]) if len(number) == 8 else int(number[0:3])
		self.minutes = float(number[2:])/60.0 if len(number) == 8 else float(number[3:])/60.0
		
	@property
	def value(self):
		return self.degrees + self.minutes
		

class GPSLatitude(GPSFixComponent):
	def __init__(self,number,direction):
		GPSFixComponent.__init__(self,number,direction)
		self.direction = direction
		
	@property 
	def value(self):
		if self.direction == 'S':
			return -GPSFixComponent.value.fget(self) # should be -
		else:
			return GPSFixComponent.value.fget(self)
		
class GPSLongitude(GPSFixComponent):
	def __init__(self,number,direction):
		GPSFixComponent.__init__(self,number, direction)
		self.direction = direction

	@property
	def value(self):
		if self.direction == 'W':
			return -GPSFixComponent.value.fget(self) # should be -
		else:
			return GPSFixComponent.value.fget(self)
		
class GPS:
	def __init__(self):
		self.port = serial.Serial("/dev/ttyAMA0", 4800, timeout=2.0)
		self.lastMessage = None
		
	def readline(self):
		gpsline = self.port.readline()
		msg = GPSMessage(gpsline)
		msg.parse()
		self.lastMessage = msg
		if msg.sentenceType == 'ZDA':
			self.time = msg.time
		
	
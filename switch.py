import web
import time
import os
from devices import bmp085
from devices import mcp2300x
from devices import tmp102
from devices import ds18xx
from protocols import i2c

urls = (
    '/',                'index',
    '/temp/workshop',   'getWorkshopTemp',
    '/test/gpio',       'testGPIO',
    '/test/tmp102',     'testTMP102',
    '/test/ds18b20',    'testDS18B20',
    '/test/time',       'testDS1307',
    '/test/altitude',   'getAltitude'
)

class testDS1307:
    def GET(self):
        text = os.popen("sudo hwclock -r").read()
        components = text.split()
        dateComponents = components[4].split(':')
        info = {}
        info['hr'] = dateComponents[0]
        info['min'] = dateComponents[1]
        info['sec'] = dateComponents[2]
        template = web.template.render('templates/')
        web.header('Content-Type', 'text/html')
        return template.time(info)
        
class getWorkshopTemp:
    def GET(self):
        bmp = bmp085.BMP085(0x77)
        temp = bmp.readTemperature();
        
        template = web.template.render('templates/')
        web.header('Content-Type', 'text/html')
        info = {}
        info['temp'] = """%0.2f""" % temp
        return template.temperature(info)
        
class getAltitude:
    def GET(self):
        bmp = bmp085.BMP085(0x77)
        alt = bmp.readAltitude();
        
        return alt

class testGPIO:
    def GET(self):        
        mcp = mcp2300x.MCP23017(0x20)
        for pin in range(6):
            mcp.config('A',pin,mcp2300x.MCP230XXDirection.OUTPUT)
        for pin in range(6):
            mcp.output('A',pin,0)
            time.sleep(0.2)
            mcp.output('A', pin,1)
            time.sleep(0.2)
        
        
class testTMP102:
    def GET(self):
        tmp = tmp102.TMP102(0x48)
        return tmp.readTemp()
    
class testDS18B20:
    def GET(self):
        dsTemp = ds18xx.DS18XX(ds18xx.DS18XXFamily.DS18B20,'0000025edf21')
        return dsTemp.read()
        
class index:
    def GET(self):
        template = web.template.render('templates/')
        web.header('Content-Type', 'text/html')
        return template.webpy401(1)

if __name__ == "__main__":
    app = web.application(urls, globals())
    app.run()

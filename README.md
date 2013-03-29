## High altitude balloon project: FMC Python source ##

### About ###

Like others, I'm launching a high-altitude balloon to take photographs and obtain environmental data from the edge of space.  But mostly just for the enjoyment of doing it.  I started out planning to use an AVR MCU for the main flight management computer onboard the payload.  In fact, if you want to see the original code for that project, it's [here](https://github.com/cocoa-factory/High-altitude-balloon-project-Master2) on github.  I'm no longer maintaining it; and it never flew.  So caveat emptor.

The new firmware is mostly written in Python.  I'm an iOS and Mac developer by day, so it's not very pythonic; but being able to work at that level of abstraction made the entire project go much faster than writing string parsing code, etc. in C would have permitted.  The firmware runs on [Raspberry Pi](http://www.raspberrypi.org) which is tiny Linux-based computer.

### Supported hardware ###

#### USB-connected camera ####

The `camera` module uses `gphoto2` to capture images from a compatible USB-connected camera and stores them on a mounted flash drive.

#### Humidity monitor ####

The payload hardware features a small [HIH-4030](http://sensing.honeywell.com/index.php?ci_id=3108&la_id=1&pr_id=53969) analog humidity monitor that is mounted to the outside of the package.


#### 1-Wire temperature monitors ####

The firmware measures interior and exterior temperatures using DS18B20 1-wire devices.  One is mounted on the accessory circuit board inside the payload.  The other is external.

#### Barometric pressure monitor ####

We measure barometric pressure and altitude using a [BMP085](http://www.bosch-sensortec.com/homepage/products_3/environmental_sensors_1/bmp085_1/bmp085) which is available as a [module](https://www.sparkfun.com/products/9694) and which is mounted on the accessory circuit board.

#### Accelerometer ####

The firmware polls an ADXL335 analog 3-axis accelerometer for acceleration data via an I2C bridge.  The bridge is provided by an ATtinyx61 as I described [here](http://alanduncan.me/blog/2013/03/24/adc-for-raspberry-pi/).  This makes up for the lack of ADC onboard the Raspberry Pi.

#### GPS ####

We support a serial-connected GPS.  Note that for high-altitude applications, you need to ensure that your GPS will function above 18 km in altitude.  Most do not.

### Getting started ###

The software may or may not be of use to you as-is without the same hardware as I'm using; but you can be the judge of that.  At a minimum, you can look at the drivers, protocols, etc. to see how I solved the various problems.
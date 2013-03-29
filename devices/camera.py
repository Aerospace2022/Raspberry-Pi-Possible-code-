#!/usr/bin/python

""" camera module """

""" The camera module allows the flight computer to capture photographs from
        a USB-connected digital camera and store them in a filesystem location
        designated at initialization.
        
        To create a new instance of the class, caminstance = camera.Camera('path_to_writable_img_store')
        
        To capture an image: caminstance.captureimage() when the image is captured it will be returned
        downloaded from the camera into the path designated at initialization and will be renamed
        with a timestamp.
        
        NOTE: This module requires a helper shell script 'captphoto.sh'

"""
import subprocess
import datetime as utcclock
import time
import re
import shutil
from os import rename

HELPER_SCRIPT_PATH = '/var/www/webpy/scripts/captphoto.sh'

class Camera:
    def __init__(self,path):
        self.storagepath = path
        self.imgregex = re.compile('(IMG_\d{4}.JPG)')

    def captureimage(self):
        d = utcclock.datetime.now()
        fn = """%02d_%02d_%02d_%02d_%02d_%02d.jpg""" % (d.year,d.month,d.day,d.hour,d.minute, d.second)
        """ capture the image from the camera """
        procname = HELPER_SCRIPT_PATH
        proc = subprocess.Popen([procname],stdout=subprocess.PIPE,shell=True)
        (out,err) = proc.communicate()
        #print "OUTPUT = %s END" % out
        matches = self.imgregex.findall(out)
        if matches != None:
            if len(matches) != 0:
                ofn = matches[0]
                oldname = '%s/%s' % (self.storagepath,ofn)
                newname = '%s/%s' % (self.storagepath,fn)
                try:
                    rename(oldname,newname)
                except OSError as err:
                    print 'ERROR | %s old = %s and new = %s' % (err,oldname,newname)
                finally:
                    return
            
    def isavailable(self):
        """ returns True if the camera is available, False if not"""
        return True
                        

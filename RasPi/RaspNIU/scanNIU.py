#!/usr/bin/env python2.7

import subprocess
import os,re
import logging
import sys
import argparse
import time
import datetime
import signal
import traceback
from bluepy.btle import Scanner, DefaultDelegate
from threading import Timer
import thread
import globals
import niu

try:
    import queue
except ImportError:
    import Queue as queue

class ScanDelegate(DefaultDelegate):
   def __init__(self):
      DefaultDelegate.__init__(self)

   def handleDiscovery(self, dev, isNewDev, isNewData):
      if isNewDev or isNewData:
         mac = dev.addr
         rssi = dev.rssi
         connectable = dev.connectable
         addrType = dev.addrType
         name = ''
         data =''
         manuf =''
         action = {}
         onlypresent = False
         logging.debug('SCANNER------'+str(dev.getScanData()) +' '+str(connectable) +' '+ str(addrType) +' '+ str(mac))
         findDevice=False
         for (adtype, desc, value) in dev.getScanData():
            if desc == 'Complete Local Name':
               name = value.strip()
            elif 'Service Data' in desc:
               data = value.strip()
            elif desc == 'Manufacturer':
               manuf = value.strip()
         for device in globals.COMPATIBILITY:
            if device().isvalid(name,manuf):
               findDevice=True
               if device().ignoreRepeat and mac in globals.IGNORE:
                  return
               globals.IGNORE.append(mac)
               logging.debug('SCANNER------This is a ' + device().name + ' device ' +str(mac))
               if mac.upper() in globals.KNOWN_DEVICES:
                  if globals.LEARN_MODE:
                     logging.debug('SCANNER------Known device and in Learn Mode ignoring ' +str(mac))
                     return
                  globals.KNOWN_DEVICES[mac.upper()]['localname'] = name
               globals.PENDING_ACTION = True
               try:
                  action = device().parse(data,mac,name)
               except Exception, e:
                  logging.debug('SCANNER------Parse failed ' +str(mac) + ' ' + str(e))
               if not action:
                  return
               globals.PENDING_ACTION = False
               action['id'] = mac.upper()
               action['type'] = device().name
               action['name'] = name
               action['rssi'] = rssi
               action['source'] = globals.daemonname
               action['rawdata'] = str(dev.getScanData())
               logging.debug('SCANNER------It\'s a known packet and I don\'t known this device so I learn ' +str(mac))
               action['learn'] = 1
               if 'version' in action:
                  action['type']= action['version']
               logging.debug(action)
         if not findDevice:
            action['id'] = mac.upper()
            action['type'] = 'default'
            action['name'] = name
            action['rssi'] = rssi
            action['source'] = globals.daemonname
            action['rawdata'] = str(dev.getScanData())
            if mac in globals.IGNORE:
               return
            globals.IGNORE.append(mac)
            if mac.upper() not in globals.KNOWN_DEVICES:
               if not globals.LEARN_MODE:
                  logging.debug('SCANNER------It\'s an unknown packet but not sent because this device is not Included and I\'am not in learn mode ' +str(mac))
                  return
               else:
                  if globals.LEARN_MODE_ALL == 0:
                     logging.debug('SCANNER------It\'s a unknown packet and I don\'t known but i\'m configured to ignore unknow packet ' +str(mac))
                     return
                  logging.debug('SCANNER------It\'s a unknown packet and I don\'t known this device so I learn ' +str(mac))
                  action['learn'] = 1
                  logging.debug(action)
                  #globals.JEEDOM_COM.add_changes('devices::'+action['id'],action)
            else:
               if len(action) > 2:
                  if globals.LEARN_MODE:
                     logging.debug('SCANNER------It\'s an unknown packet i know this device but i\'m in learn mode ignoring ' +str(mac))
                     return
                  logging.debug('SCANNER------It\'s a unknown packet and I known this device so I send ' +str(mac))
                  logging.debug(action)
                  #globals.JEEDOM_COM.add_changes('devices::'+action['id'],action)

def scan():
   global scanner
   logging.info("Start scanning...")
   scanner = Scanner().withDelegate(ScanDelegate())
   logging.info("Preparing Scanner...")
   try:
      while 1:
         try:
            scanner.scan(3)
         except Exception, e:
            logging.error("Exception on scanner (didn't resolve there is an issue with bluetooth) : %s" % str(e))
   except KeyboardInterrupt:
      logging.error("GLOBAL------KeyboardInterrupt, shutdown")
      shutdown()

def handler(signum=None, frame=None):
   logging.debug("Signal %i caught, exiting..." % int(signum))
   shutdown()

def shutdown():
   logging.debug("GLOBAL------Shutdown")
   logging.debug("GLOBAL------Closing all potential bluetooth connection")
   for device in list(globals.KEEPED_CONNECTION):
      logging.debug("GLOBAL------This antenna should not keep a connection with this device, disconnecting " + str(device))
      try:
         globals.KEEPED_CONNECTION[device].disconnect(True)
         logging.debug("Connection closed for " + str(device))
      except Exception, e:
         logging.debug(str(e))
   logging.debug("Exit 0")
   sys.stdout.flush()
   os._exit(0)

parser = argparse.ArgumentParser(description='Bluetooth Daemon')
args = parser.parse_args()

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

logging.info('Start NIU button blutooth scan: please push button(s) once')
signal.signal(signal.SIGINT, handler)
signal.signal(signal.SIGTERM, handler)
try:
   scan()
except Exception,e:
   logging.error('GLOBAL------Fatal error : '+str(e))
   logging.debug(traceback.format_exc())
   shutdown()

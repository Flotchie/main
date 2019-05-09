#!/usr/bin/env python2.7

import os,re
import logging
import sys
import time
import datetime
import signal
import traceback
from bluepy.btle import Scanner, DefaultDelegate
import globals
import niu
import ConfigParser

try:
    import queue
except ImportError:
    import Queue as queue

class ScanDelegate(DefaultDelegate):
   import globals
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
         if mac not in globals.IGNORE:
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
               if mac.upper() not in globals.KNOWN_DEVICES:
                  logging.debug('SCANNER------It\'s a known packet but not decoded because this device is not Included and I\'am not in learn mode ' +str(mac))
                  return
               else :
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
               action['rawdata'] = str(dev.getScanData())
               if len(action) > 2:
                  if globals.LAST_TIME_EVENT != action['datetime']:
                     logging.debug(action)
                     globals.LAST_TIME_EVENT = action['datetime']
                     obj  = globals.KNOWN_DEVICES[mac.upper()]
                     f = open(obj['file'], "w")
                     f.write("{\n")
                     f.write("\"name\"=\"%s\",\n"%obj['name'])
                     f.write("\"date\"=\"%s\",\n"%datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
                     f.write("\"value\"=%s,\n"%action['buttonid'])
                     f.write("\"color\"=\"%s\",\n"%action['color'])
                     f.write("\"action\"=\"%s\",\n"%action['button'])
                     f.write("\"battery\"=%s\n"%action['battery'])
                     f.write("}\n")
         if not findDevice:
            action['id'] = mac.upper()
            action['type'] = 'default'
            action['name'] = name
            action['rssi'] = rssi
            action['rawdata'] = str(dev.getScanData())
            if mac in globals.IGNORE:
               return
            globals.IGNORE.append(mac)
            if mac.upper() not in globals.KNOWN_DEVICES:
               logging.debug('SCANNER------It\'s an unknown packet but not sent because this device is not Included and I\'am not in learn mode ' +str(mac))
            else:
               if len(action) > 2:
                  logging.debug('SCANNER------It\'s a unknown packet and I known this device so I send ' +str(mac))
                  logging.debug(action)
                  #globals.JEEDOM_COM.add_changes('devices::'+action['id'],action)

def addDevice(name, mac):
   mac = mac.upper()
   logging.debug('Add device[%s] MAC[%s]'%(name, mac))
   dirpath = '/home/pi/data/niu'
   if not os.path.exists(dirpath):
      os.makedirs(dirpath)
   device = { 'name' : name, 'id' : mac, 'file' : "%s/%s"%(dirpath,name) }
   globals.KNOWN_DEVICES[mac] = device

def listen():
   global scanner
   logging.info("GLOBAL------Start listening...")
   globals.SCANNER = Scanner().withDelegate(ScanDelegate())
   logging.info("GLOBAL------Preparing Scanner...")
   try:
      while 1:
         try:
            if (globals.LAST_CLEAR + 29)  < int(time.time()):
               globals.SCANNER.clear()
               globals.IGNORE[:] = []
               globals.LAST_CLEAR = int(time.time())
            globals.SCANNER.start()
            globals.SCANNER.process(0.3)
            globals.SCANNER.stop()
            if globals.SCAN_ERRORS > 0:
               logging.info("GLOBAL------Attempt to recover successful, reseting counter")
               globals.SCAN_ERRORS = 0
            while globals.PENDING_ACTION:
               time.sleep(0.01)
         except Exception, e:
            logging.error("GLOBAL------Exception on scanner : %s" % str(e))
            if not globals.PENDING_ACTION:
               if globals.SCAN_ERRORS < 5:
                  globals.SCAN_ERRORS = globals.SCAN_ERRORS+1
                  globals.SCANNER = Scanner().withDelegate(ScanDelegate())
               elif globals.SCAN_ERRORS >=5 and globals.SCAN_ERRORS< 8:
                  globals.SCAN_ERRORS = globals.SCAN_ERRORS+1
                  logging.warning("GLOBAL------Exception on scanner (trying to resolve by myself " + str(globals.SCAN_ERRORS) + "): %s" % str(e))
                  os.system('hciconfig ' + globals.device + ' down')
                  os.system('hciconfig ' + globals.device + ' up')
                  globals.SCANNER = Scanner().withDelegate(ScanDelegate())
               else:
                  logging.error("GLOBAL------Exception on scanner (didn't resolve there is an issue with bluetooth) : %s" % str(e))
                  logging.info("GLOBAL------Shutting down due to errors")
                  time.sleep(2)
                  shutdown()
         time.sleep(0.02)
   except KeyboardInterrupt:
      logging.error("GLOBAL------KeyboardInterrupt, shutdown")
      shutdown()

def handler(signum=None, frame=None):
   logging.debug("GLOBAL------Signal %i caught, exiting..." % int(signum))
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

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.INFO)

config = ConfigParser.RawConfigParser()
config.read('RaspNIU.ini')
for (name,mac) in config.items('BUTTON'):
   print (name,mac)
   
   addDevice(name, mac)

logging.info('Start Blutooth daemon')
signal.signal(signal.SIGINT, handler)
signal.signal(signal.SIGTERM, handler)
try:
   listen()

except Exception,e:
   logging.error('Fatal error : '+str(e))
   logging.debug(traceback.format_exc())
   shutdown()

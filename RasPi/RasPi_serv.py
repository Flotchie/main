#!/usr/bin/python
# coding: utf8

import logging
import logging.handlers
import argparse
import sys
import zmq
import os
from os.path import isfile, join

# Defaults
zmqPort = 5556
relayDir='/home/pi/data/relay/'
tProbeDir='/home/pi/data/tprobe/'
LOG_FILENAME = "/home/pi/log/RasPi_serv.log"
LOG_LEVEL = logging.INFO  # Could be e.g. "DEBUG" or "WARNING"

# Define and parse command line arguments
parser = argparse.ArgumentParser(description="Raspberry Piscine access service")
parser.add_argument("-l", "--log", help="file to write log to (default '" + LOG_FILENAME + "')")

# Configure logging to log to a file, making a new file at midnight and keeping the last 3 day's data
# Give the logger a unique name (good practice)
logger = logging.getLogger(__name__)
# Set the log level to LOG_LEVEL
logger.setLevel(LOG_LEVEL)
# Make a handler that writes to a file, making a new file at midnight and keeping 3 backups
handler = logging.handlers.TimedRotatingFileHandler(LOG_FILENAME, when="midnight", backupCount=3)
# Format each log message like this
formatter = logging.Formatter('%(asctime)s %(levelname)-8s %(message)s')
# Attach the formatter to the handler
handler.setFormatter(formatter)
# Attach the handler to the logger
logger.addHandler(handler)

# Make a class we can use to capture stdout and sterr in the log
class MyLogger(object):
   def __init__(self, logger, level):
     """Needs a logger and a logger level."""
     self.logger = logger
     self.level = level

   def write(self, message):
     # Only log if there is a message (not just a new line)
     if message.rstrip() != "":
       self.logger.log(self.level, message.rstrip())

def readRelay(relayId):
   dataFile='%sR%d'%(relayDir,relayId)
   value=""      
   try:
      fData = open(dataFile)
   except IOError:
      pass
   else:
      line = fData.readline()
      value=line.rstrip()
   return value

def writeRelay(relayId, value):
   if relayId<1 or relayId>8:
      return
   if not os.path.exists(relayDir):
      os.makedirs(relayDir)      
   dataFile='%sR%d'%(relayDir,relayId)
   try:
      fData = open(dataFile,"w")
   except IOError:
      pass
   else:
      fData.write(str(value))
 
def readTemp(tempId):
   dataFile=tProbeDir+tempId
   value=""      
   try:
      fData = open(dataFile)
   except IOError:
      pass
   else:
      line = fData.readline()
      logger.info( "%s" % line)
      pos = line.find("temp[")
      if pos > -1:
         posEnd = line.find("]", pos)
         value = line[pos+5:posEnd]
   return value
   
def readTempQuality(tempId):
   dataFile=tProbeDir+tempId
   value=""      
   try:
      fData = open(dataFile)
   except IOError:
      pass
   else:
      line = fData.readline()
      logger.info( "%s" % line)
      pos = line.find("quality[")
      if pos > -1:
         posEnd = line.find("]", pos)
         value = line[pos+8:posEnd]
   return value
   
def readTempDate(tempId):
   dataFile=tProbeDir+tempId
   value=""      
   try:
      fData = open(dataFile)
   except IOError:
      pass
   else:
      line = fData.readline()
      logger.info( "%s" % line)
      pos = line.find("date[")
      if pos > -1:
         posEnd = line.find("]", pos)
         value = line[pos+5:posEnd]
   return value
   
def listTProbe():
   l =""
   for f in os.listdir(tProbeDir):
      if isfile(join(tProbeDir, f)):
         l = l + " " + f
   return l
   
# Replace stdout with logging to file at INFO level
sys.stdout = MyLogger(logger, logging.INFO)
# Replace stderr with logging to file at ERROR level
sys.stderr = MyLogger(logger, logging.ERROR)

# remise à 0 des Relais
for relayId in range(1,9):
   writeRelay(relayId, 0)
   
context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:%d" % zmqPort)

StopServer=False
while not StopServer:
   #  Wait for next request from client
   message = socket.recv()
   logger.info("Received request: "+ message)
   args = message.split()
   if args[0] == "READ":
      value=""
      if len(args)==2:
         try:
            relayId = int(args[1])
            value=readRelay(relayId)
         except:
            pass
      logger.info( " -> %s" % value)
      socket.send(value)     
   elif args[0] == "WRITE":
      if len(args)==3:
         try:      
            relayId = int(args[1])
            value = int(args[2])
            writeRelay(relayId, value)
         except:
            pass            
      socket.send("")
   elif args[0] == "DS18B20" or args[0] == "TEMP":
      temp=""
      if len(args)==2:
         temp=readTemp(args[1])
      logger.info( " -> %s" % temp)
      socket.send(temp)
   elif args[0] == "TEMP_QUALITY":
      temp=""
      if len(args)==2:
         temp=readTempQuality(args[1])
      logger.info( " -> %s" % temp)
      socket.send(temp)   
   elif args[0] == "TEMP_DATE":
      temp=""
      if len(args)==2:
         temp=readTempDate(args[1])
      logger.info( " -> %s" % temp)
      socket.send(temp)       
   elif args[0] == "TPROBE":
      datas = listTProbe()
      socket.send(datas)     
   elif args[0] == "STOP":
      logger.info("Stopping...")
      socket.send("")
      StopServer=True
   else:
      logger.error("unknown command ["+args[0]+"]")
      socket.send("unknown command ["+args[0]+"]" )

# remise à 0 des Relais
for relayId in range(1,9):
   writeRelay(relayId, 0)

	
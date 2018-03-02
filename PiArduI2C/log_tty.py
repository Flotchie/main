#!/usr/bin/python
import serial
import sys
port="/dev/ttyUSB0"

s1 = serial.Serial(port,57600)
s1.flushInput()

while True:
   if s1.inWaiting()>0:
      inputValue = s1.readline()
      sys.stdout.write(inputValue)
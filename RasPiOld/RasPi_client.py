#!/usr/bin/python
# coding: utf8

import RPi.GPIO as GPIO
import zmq
import sys

port = "5556"

if len(sys.argv) < 2:
	print "NOK"
	sys.exit()
args=""
for i in range(1,len(sys.argv)):
    args+= " " + sys.argv[i]
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect ("tcp://localhost:%s" % port)

socket.send (args)
#  Get the reply.
message = socket.recv()
print message
#print "Received reply [", message, "]"
#print "OK"
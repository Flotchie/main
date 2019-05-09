from bluepy.btle import Scanner, DefaultDelegate
import time
import logging
import globals

class Niu():
	def __init__(self):
		self.name = 'niu'
		self.ignoreRepeat = False

	def isvalid(self,name,manuf=''):
		if name.lower() == self.name:
			return True

	def parse(self,data,mac,name):
		action={}
		logging.debug('Parsing data ' + data)
		colortrame = data[26:28]
		batterytrame = data[28:30]
		buttontrame = data[34:36]
		battery = str(int(batterytrame,16))
		if colortrame == '01':
			color = 'No color'
		elif colortrame == '02':
			color = 'White'
		elif colortrame == '03':
			color = 'TechBlue'
		elif colortrame == '04':
			color = 'CozyGrey'
		elif colortrame == '05':
			color = 'Wazabi'
		elif colortrame == '06':
			color = 'Lagoon'
		elif colortrame == '07':
			color = 'Softberry'
		else:
			color = 'No color'
		if buttontrame == '01':
			button = 'simple appui'
		elif buttontrame == '02':
			button = 'double appui'
		elif buttontrame == '03':
			button = 'appui long'
		elif buttontrame == '04':
			button = 'relachement'
		elif buttontrame == '05':
			buttontrame = '02'
			button = 'double appui'
		elif buttontrame == '06':
			buttontrame = '02'
			button = 'double appui'
		elif buttontrame == '07':
			buttontrame = '02'
			button = 'double appui'
		else:
			button = ''
		action['color'] = color
		action['battery'] = battery
		action['button'] = button
		action['buttonid'] = buttontrame[1:2]
		action['datetime'] = data[30:34]
		return action

globals.COMPATIBILITY.append(Niu)
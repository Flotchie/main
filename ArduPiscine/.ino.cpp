#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2018-02-25 20:22:23

#include "Arduino.h"
#include "Arduino.h"
#include <Wire.h>
#include <util/atomic.h>
#include "OneWire.h"
#include "arduPi.h"
void incPos( int& pos ) ;
void addData( const DATA& data ) ;
bool readData(DATA& data) ;
void setup() ;
void loop() ;
void sendRelayState(char relayId) ;
void setRelayState(char relayId, char onOff ) ;
void readOneWire(void) ;
static inline unsigned long diff(long A, long B) ;
void receiveRFProtocol(unsigned int changeCount) ;
void rfSignalChange() ;
void dataRequestFromI2CMaster() ;
void receiveEventFromI2CMaster(int howMany) ;


#include "arduPi.ino"

#endif

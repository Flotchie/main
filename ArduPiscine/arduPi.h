/*
 * testArdu.h
 *
 *  Created on: 10 févr. 2017
 *      Author: Bertrand
 */

#ifndef ARDUPI_H_
#define ARDUPI_H_

enum DataId
{
   DataId_WSPT1=1,
   DataId_DS18x20=2,
   DataId_Relay=3,
};

struct WSPT1_DATA
{
   int16_t start;
   int16_t temperature;
   int16_t id;
   char checksum;
};

struct DS18x20_DATA
{
   char ROM[8];
   char data[10];
   int16_t temperature;
};

struct RELAY_DATA
{
   char relayId;
   char onOff;
};

struct DATA
{
   char dataId;
   char buffer[28];
};

enum ReadOneWireState
{
   ROWS_Idle,
   ROWS_WaitConversion,
};

#define MAX_CHANGES 128
volatile unsigned int timings[MAX_CHANGES];
volatile unsigned int changeCount;
volatile unsigned long lastTime;
volatile unsigned int nReceivedBitlength;
volatile char sReceivedBits[64];
volatile long lastData;
volatile long lastBlink;
volatile long lastAlive;
bool rfOK;
ReadOneWireState readOneWireState;
long ReadOneWireNextStateTime;
uint8_t ReadOneWireAddr[8];
byte  ReadOneWireType_s;

void incPos( int& pos ) ;
void addData( const DATA& data ) ;
bool readData(DATA& data) ;
void setup() ;
void loop() ;
void sendRelayState(char relayId) ;
void setRelayState(char relayId, char onOff ) ;
void readOneWire(long ct) ;
static inline unsigned long diff(long A, long B) ;
void receiveRFProtocol(unsigned int changeCount) ;
void rfSignalChange() ;
void dataRequestFromI2CMaster() ;
void receiveEventFromI2CMaster(int howMany) ;

#endif /* ARDUPI_H_ */

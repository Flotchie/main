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

#define MAX_CHANGES 128
volatile unsigned int timings[MAX_CHANGES];
volatile unsigned int changeCount;
volatile unsigned long lastTime;
volatile unsigned int nReceivedBitlength;
volatile char sReceivedBits[64];
volatile long lastData;

#endif /* ARDUPI_H_ */

#include "Arduino.h"
#include <Wire.h>
#include <util/atomic.h>
#include "OneWire.h"
#include "arduPi.h"

const byte pin02_RFData = 2;  // RF data signal
const byte pin05_12_relay[] = {5,6,7,8,9,10,11,12};  // relay 1-8 pins
const byte pin13_led = 13;    // led carte
byte stateLed = LOW;
int bufferSize = 0;

// interface OneWire
OneWire  ds(3);  // on pin 3 (a 4.7K resistor is necessary)

const int nbData = 20;
DATA datas[nbData];
int posDataRead=0;
int posDataWrite=0;

void incPos( int& pos )
{
   pos = (pos+1)%nbData;
}

void addData( const DATA& data )
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      memcpy(&(datas[posDataWrite]),&data,sizeof(DATA));
      incPos(posDataWrite);
      if(posDataWrite==posDataRead) incPos(posDataRead);
   }
}

bool readData(DATA& data)
{
   bool ret = false;
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      if( posDataRead != posDataWrite )
      {
         memcpy(&data, &(datas[posDataRead]),sizeof(DATA));
         incPos(posDataRead);
         ret = true;
      }
   }
   return ret;
}

//The setup function is called once at startup of the sketch
void setup()
{
	Wire.begin(8);                // join i2c bus with address #8
	Wire.onRequest(dataRequestFromI2CMaster); // register event
	Wire.onReceive(receiveEventFromI2CMaster); // register event
   pinMode(pin02_RFData, INPUT);
   pinMode(pin13_led, OUTPUT);

   // 8 outputs for relays
   for(unsigned int rel=0;rel<8;++rel)
   {
      digitalWrite(pin05_12_relay[rel], HIGH);
      pinMode(pin05_12_relay[rel], OUTPUT);
   }

   // gpio2 -> interrupt 0
   attachInterrupt(0, rfSignalChange, CHANGE);
   lastData = -200001L;
   rfOK = false;
   ReadOneWireNextStateTime = 0L;
   readOneWireState = ROWS_Idle;
   Serial.begin(57600); // start serial for debug traces
   for(char relayId=1;relayId<=8;++relayId)
   {
      sendRelayState(relayId);
   }
   Serial.println("setup() achieved");
}

// The loop function is called in an endless loop
void loop()
{
   long ct = millis();
   if( (ct-lastAlive)>=5000L)
   {
       Serial.print("Alive(");
       Serial.print(ct, 10);
       Serial.print(",");       
       Serial.print(rfOK, 10);
       Serial.println(")");
       lastAlive = ct;
   }         
   if( (ct-lastData)<200000L )
   {
      // reception dans les 200 dernières secondes : led allumée fixe
      digitalWrite(pin13_led, HIGH);
      rfOK = true;
   }
   else
   {
      rfOK = false;
      if( (ct-lastBlink)>=1000L)
      {
         // pas de reception dans les 200 dernières secondes : led clignotante
         digitalWrite(pin13_led, stateLed = (stateLed+1)%2);
         lastBlink = ct;
      }
   }
   readOneWire(ct);
   delay(1);
}

void sendRelayState(char relayId)
{
   DATA data;
   data.dataId = (char)DataId_Relay;
   memset(data.buffer,0,sizeof(data.buffer));
   RELAY_DATA* gpio = (RELAY_DATA*)data.buffer;
   gpio->relayId = relayId;
   gpio->onOff = digitalRead(pin05_12_relay[relayId-1])==0?1:0;
   addData(data);
}

void setRelayState(char relayId, char onOff )
{
   Serial.print("setRelayState(");
   Serial.print(relayId,10);
   Serial.print(",");
   Serial.print(onOff,10);
   Serial.println(")");
   if(1<=relayId && relayId<=8)
   {
      if(onOff==1)
      {
         digitalWrite(pin05_12_relay[relayId-1], LOW);
      }
      else if(onOff==2)
      {
         digitalWrite(pin05_12_relay[relayId-1], HIGH);
         delay(100);
         digitalWrite(pin05_12_relay[relayId-1], LOW);
      }
      else
      {
         digitalWrite(pin05_12_relay[relayId-1], HIGH);
      }
      sendRelayState(relayId);
   }
   else
   {
      Serial.println("relay id invalid");
   }
}

void readOneWire(long ct)
{
   //Serial.print("readOneWire(");
   //Serial.print(ct,10);
   //Serial.print(",");
   //Serial.print(ReadOneWireNextStateTime,10);
   //Serial.println(")");   
   byte i;
   switch(readOneWireState)
   {
      case ROWS_Idle:
      {
         if(ct>=ReadOneWireNextStateTime)
         {
            Serial.println("readOneWire() - Search Device");
            if ( !ds.search(ReadOneWireAddr))
            {
               Serial.println("  No more addresses.");
               Serial.println();
               ds.reset_search();
               ReadOneWireNextStateTime = ct + 10000L;
               return;
            }

            Serial.print("  ROM =");
            for( i = 0; i < 8; i++)
            {
               Serial.write(' ');
               Serial.print(ReadOneWireAddr[i], HEX);
            }

            if (OneWire::crc8(ReadOneWireAddr, 7) != ReadOneWireAddr[7])
            {
               Serial.println("CRC is not valid!");
               return;
            }
            Serial.println();

            // the first ROM byte indicates which chip
            switch (ReadOneWireAddr[0])
            {
               case 0x10:
                  Serial.println("  Chip = DS18S20");  // or old DS1820
                  ReadOneWireType_s = 1;
               break;
               case 0x28:
                  Serial.println("  Chip = DS18B20");
                  ReadOneWireType_s = 0;
               break;
               case 0x22:
                  Serial.println("  Chip = DS1822");
                  ReadOneWireType_s = 0;
               break;
               default:
                  Serial.println("Device is not a DS18x20 family device.");
                  ReadOneWireNextStateTime = ct + 500L;
                  return;
            }

            ds.reset();
            ds.select(ReadOneWireAddr);
            ds.write(0x44, 1);        // start conversion, with parasite power on at the end
            ReadOneWireNextStateTime = ct + 850L;
            readOneWireState = ROWS_WaitConversion;
         }
      }
      break;
      case ROWS_WaitConversion:
      {
         byte present = 0;
         byte data[12];
         float celsius;

         present = ds.reset();
         ds.select(ReadOneWireAddr);
         ds.write(0xBE);         // Read Scratchpad

         Serial.print("  Data = ");
         Serial.print(present, HEX);
         Serial.print(" ");
         for ( i = 0; i < 9; i++)
         {  // we need 9 bytes
            data[i] = ds.read();
            Serial.print(data[i], HEX);
            Serial.print(" ");
         }
         Serial.print(" CRC=");
         Serial.print(OneWire::crc8(data, 8), HEX);
         Serial.println();

         // Convert the data to actual temperature
         // because the result is a 16 bit signed integer, it should
         // be stored to an "int16_t" type, which is always 16 bits
         // even when compiled on a 32 bit processor.
         int16_t raw = (data[1] << 8) | data[0];
         if (ReadOneWireType_s)
         {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10)
            {
               // "count remain" gives full 12 bit resolution
               raw = (raw & 0xFFF0) + 12 - data[6];
            }
         }
         else
         {
            byte cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
            //// default is 12 bit resolution, 750 ms conversion time
         }
         celsius = (float)raw / 16.0;
         Serial.print("  Temperature = ");
         Serial.print(celsius);
         Serial.println();
         Serial.println();

         // mémorise la donnée pour le Raspberrry
         DATA dataToSend;
         dataToSend.dataId = (char)DataId_DS18x20;
         memset(dataToSend.buffer,0,sizeof(dataToSend.buffer));
         DS18x20_DATA* ds18 = (DS18x20_DATA*)dataToSend.buffer;
         for( i = 0; i < 8; i++)
         {
            ds18->ROM[i] = ReadOneWireAddr[i];
         }
         for( i = 0; i < 9; i++)
         {
            ds18->data[i] = data[i];
         }
         ds18->temperature = celsius*100;
         addData(dataToSend);
         ReadOneWireNextStateTime = ct + 500L;
         readOneWireState = ROWS_Idle;
      }
      break;
   }
}

/* helper function for the receiveProtocol method */
static inline unsigned long diff(long A, long B)
{
  return abs(A - B);
}

void receiveRFProtocol(unsigned int changeCount)
{
   //Serial.print("receiveRFProtocol(");
   //Serial.print(changeCount);
   //Serial.println(")");

   unsigned int start=0;
   unsigned int temperature=0;
   unsigned int objectid=0;
   unsigned int checksum=0;

   const unsigned long delay0 = timings[0];
   const unsigned long delay1 = delay0*3;
   const unsigned long delay0Tolerance = delay0 * 25 / 100;
   const unsigned long delay1Tolerance = delay1 * 25 / 100;
   unsigned int code = 0;
   unsigned int nbBit = 0;
   unsigned int nInfo = 0;
   unsigned int nInfoBit = 0;
   for (unsigned int i = 0; i < changeCount - 1; i += 2)
   {
       code <<= 1;
       if ( diff(timings[i], delay0) < delay0Tolerance
            &&
            ( diff(timings[i + 1], delay1) < delay1Tolerance
              ||  timings[i + 1] > 24000
            )
           )
       {
           // zero
           sReceivedBits[nbBit]  = '0';
           nbBit++;
       }
       else if ( diff(timings[i], delay1) < delay1Tolerance
                 &&
                 ( diff(timings[i + 1], delay0) < delay0Tolerance
                   || timings[i + 1] > 24000
                 )
               )
       {
           // one
           code |= 1;
           sReceivedBits[nbBit]  = '1';
           nbBit++;
           if(nInfo==0) nInfo=1;
       }
       else
       {
          sReceivedBits[nbBit]  = 0;
          //Serial.print("  truncated=");
          //Serial.println((char*)sReceivedBits);
          code = 0;
          nbBit = 0;
       }
       if((nInfo>0) && (nbBit>0)) ++nInfoBit;
       if( (nInfo==1) && (nInfoBit==5) )
       {
          start = code;
          code = 0;
          nInfoBit = 0;
          nInfo=2;
       }
       else if( (nInfo==2) && (nInfoBit==10) )
       {

          temperature = code;
          code = 0;
          nInfoBit = 0;
          nInfo=3;
       }
       else if( (nInfo==3) && (nInfoBit==10) )
       {
          objectid = code;
          code = 0;
          nInfoBit = 0;
          nInfo=4;
       }
       else if( (nInfo==4) && (nInfoBit==4) )
       {
          checksum = code;
          code = 0;
          nInfoBit = 0;
          nInfo=5;
          break;
       }
   }

   if (nInfo==5)
   {    // trame complete
       sReceivedBits[nbBit]  = 0;
       nReceivedBitlength = nbBit;
       DATA data;
       data.dataId = (char)DataId_WSPT1;
       memset(data.buffer,0,sizeof(data.buffer));
       WSPT1_DATA* wspt1 = (WSPT1_DATA*)data.buffer;
       wspt1->start = start;
       //Serial.print("  start=");
       //Serial.println(start,HEX);
       wspt1->temperature = temperature-500;
       wspt1->checksum = checksum;
       //Serial.print("  checksum=");
       //Serial.println(checksum,HEX);
       wspt1->id = objectid;
       //Serial.print("  objectid=");
       //Serial.println(objectid,HEX);
       //Serial.print("  trame=");
       //Serial.println((char*)sReceivedBits);
       //Serial.print("  temperature=");
       //Serial.print((float)wspt1->temperature/10.0);
       //Serial.println("");
       addData(data);
       lastData = millis();
   }
   else
   {
      sReceivedBits[nbBit]  = 0;
      //Serial.print("  truncated=");
      //Serial.println((char*)sReceivedBits);
   }
   //Serial.println();
}

/**
 * changement du signal
 * - mémorise les changements
 * si le dernier changement a duré moins de 1,9ms recommence l'enregistrement à 0
 * le premier changement doit être entre 1,9 et 2.1ms
 */
void rfSignalChange()
{
   //byte state = digitalRead(inPin);
   unsigned long time = micros();
   if(lastTime==0) lastTime = time;
   const unsigned int duration = time - lastTime;
   if( duration < 1900 || (changeCount==0 && duration > 2100) )
   {
      changeCount = 0;
   }
   else
   {
      timings[changeCount++] = duration;
      if (duration > 24000)
      {
         if(changeCount>7)
         {
            receiveRFProtocol( changeCount );
            changeCount = 0;
         }
         else
         {
            changeCount=0;
         }
      }
      // detect overflow
     if (changeCount >= MAX_CHANGES)
     {
       changeCount = 0;
     }
  }
  lastTime = time;
}

// requete du maître I2C (Raspberry)
// pour avoir des données
void dataRequestFromI2CMaster()
{
   //Serial.println("dataRequestFromI2CMaster ");
   DATA data;
   if(readData(data))
   {
      //Serial.print("requestEvent() -> data\n");
      Wire.write((const unsigned char*)&data, sizeof(DATA));
   }
   else
   {
      Wire.write("\0");
   }
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEventFromI2CMaster(int howMany)
{
   //Serial.print("receiveEventFromI2CMaster ");
   //Serial.println(howMany);         // print the integer
   Wire.read();   // first is always 0
   int state=0; // 0 :wait command, 1: relayCommandArg1, 2: relayCommandArg2
   // relayCommand: 0x34, relayId, on/off(1/0)
   const char CmdRelay=0x34;
   char relayId=0;
   while (0 < Wire.available())
   {
    const char c = Wire.read(); // receive byte as a character
    if(state==0)
    {
       if(c==CmdRelay)
       {
          state = 1;
       }
    }
    else if (state==1)
    {
       relayId = c;
       state=2;
    }
    else if (state==2)
    {
       setRelayState(relayId, c);
       state=0;
    }
   }
}

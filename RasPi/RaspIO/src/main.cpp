//============================================================================
// Name        : main.cpp
// Author      : Bertrand
// Version     :
// Copyright   : Youpee
// Description : Interface Entrées/Sorties
//               - sondes température DS18B20
//               - transceiver RFM69HW vers Arduino/RFM69HW
//               - sorties digitales du GPIO - relais
//               -
//============================================================================

#include "main.h"

#include "RH_RF69.h"
#include "RHHardwareSPI.h"
#include "bcm2835.h"
#include <iostream>
#include <unistd.h>
#include <map>
#include <vector>
#include <set>
#include <stdio.h>
#include <iomanip>
#include <ctime>

#include "ini/SimpleIni.h"
#include "StringTools.h"
using namespace std;

string ini_file = "/home/pi/remote-debugging/RaspIO.ini";
string tprobe_path = "/home/pi/data/tprobe/";
string w1_devices_path = "/sys/bus/w1/devices/";

uint8_t rf69InterruptPin   =  5; // RF69 interrupt
uint8_t OLEDResetPin       =  6; // OLED RESET
uint8_t OLEDSPIChipSelect  =  7; // SPI CS1
uint8_t rf69SlaveSelectPin =  8; // SPI CS0
uint8_t SPI_MISO           =  9; // SPI MISO
uint8_t SPI_MOSI           = 10; // SPI MOSI
uint8_t SPI_SCLK           = 11; // SPI SCLK
uint8_t OLEDSPIDataCommand = 13; // OLED DC

#define DELTAY 2
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static unsigned char logo16_glcd_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };


int main()
{
	map<string,string> ds18b20s;

   CSimpleIniA ini(true, true, true);
   ini.Reset();
   if (ini.LoadFile(ini_file.c_str()) < 0)
   {
      Serial.println("ini load failed");
      return 1;
   }

   CSimpleIniA::TNamesDepend keys;
   ini.GetAllKeys("DS18B20", keys);
   for(auto& key : keys)
   {
      const char * pszValue = ini.GetValue("DS18B20",  key.pItem);
      ds18b20s.insert(make_pair(key.pItem,pszValue));
   }

   // suppression des sondes absente du .ini
   set<string> previousTempProbes;
   ListDir(tprobe_path, previousTempProbes);
   for(auto& probe:previousTempProbes)
   {
      if(  (ds18b20s.end()==ds18b20s.find(probe))
        && (probe!="eau")
        )
      {  // non trouvé
         remove(form("%s%s", tprobe_path.c_str(), probe.c_str()).c_str());
      }
   }

	// Initialisation interface avec BCM2835
	if(bcm2835_init()!=1)
	{
	   Serial.println("bcm2835_init failed");
	   return 2;
	}

	RHHardwareSPI hardware_spi(RHGenericSPI::Frequency1MHz, RHGenericSPI::BitOrderMSBFirst, RHGenericSPI::DataMode0);

	hardware_spi.begin();

	// Initialisation afficheur
	ArduiPi_OLED display(hardware_spi);
   if ( !display.init(OLEDSPIDataCommand, OLEDResetPin, OLEDSPIChipSelect
         , OLED_ADAFRUIT_SPI_128x64) )
   {
      Serial.println("OLED::init() failed");
      return 3;
   }
   display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
   display.display();

   // Initialisation RFM69HW
	RH_RF69 rf69(rf69SlaveSelectPin, rf69InterruptPin, hardware_spi);
   if (!rf69.init())
   {
	    Serial.println("rf69::init() failed");
	    return 4;
   }
   if (!rf69.setFrequency(868.0))
   {
      Serial.println("setFrequency failed");
      return 5;
   }
   // The encryption key has to be the same as the one in the client
   uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
   rf69.setEncryptionKey(key);

   uint8_t rf69Buf[RH_RF69_MAX_MESSAGE_LEN];
   auto previous = chrono::system_clock::now();
   while(true)
   {
      auto now = chrono::system_clock::now();

      // RF69 : température eau piscine
      rf69.checkForReception();
      if (rf69.available())
      {
        // Should be a message for us now
        uint8_t len = sizeof(rf69Buf);
        if (rf69.recv(rf69Buf, &len))
        {
           Serial.print("got frame: ");
           rf69Buf[len] = 0;
           Serial.println((char*)rf69Buf);
           if(strlen((const char*)rf69Buf)==28)
           {
              string sTemp = string((const char*)rf69Buf).substr(12,2)+string((const char*)rf69Buf).substr(15,2);
              double temp = atof(sTemp.c_str())/100.0;
              writeProbe("eau", now, temp);
           }
           else
           {
              Serial.println("not a temperature");
           }
        }
        else
        {
           Serial.println("recv failed");
        }
      }
      if( chrono::duration_cast<std::chrono::seconds>(now-previous).count()>=60)
      {
         // DS18B20 : sondes de température locales
         map<string,double> tprobes;
         set<string> oneWireProbes;
         ListDir(w1_devices_path, oneWireProbes);
         for(auto& probe:oneWireProbes)
         {
           if(probe.find("28-")==0)
           {
              // une sonde trouvée
              string path = form("%s%s/w1_slave"
                    , w1_devices_path.c_str(), probe.c_str());
              char line[255];
              FILE* fp = fopen(path.c_str(), "r");
              if(fp!=nullptr)
              {
                 if( (nullptr!=fgets(line, 255,fp ))
                   && (nullptr!=fgets(line, 255,fp ))
                   )
                 {
                    char* tpos = strstr(line, "t=");
                    if(tpos!=nullptr)
                    {
                       double temp = atol(tpos+2)/1000.0;
                       tprobes.insert(make_pair(probe,temp));
                    }
                 }
                 fclose(fp);
              }
           }
         }
         for(auto& pprobe:ds18b20s)
         {
            if(tprobes.end()!=tprobes.find(pprobe.second))
            {  // trouvé
               writeProbe(pprobe.first, now, tprobes[pprobe.second]);
            }
         }
         previous = now;
      }
      // pause de 500ms
      usleep(500000);
   }
   return 0;
}

/**
 * écriture du fichier probe au format JSON
 * {
 *    "name": "eau",
 *    "date": "2018-12-30 15:55:48",
 *    "value": 19.2
 * }
 */
bool writeProbe(const std::string& probe, const std::chrono::system_clock::time_point& now, double temperature)
{
   string path = form("%s%s", tprobe_path.c_str(), probe.c_str());
   FILE* fp = fopen(path.c_str(), "w");
   if(fp!=NULL)
   {
      time_t tt = chrono::system_clock::to_time_t(now);
      char date[40];
      strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&tt));
      // format json
      fputs("{\n",fp);
      fputs(form("\"name\": \"%s\",\n", probe.c_str()).c_str(),fp);
      fputs(form("\"date\": \"%s\",\n", date).c_str(),fp);
      fputs(form("\"value\": %.1f\n", temperature).c_str(),fp);
      fputs("}\n",fp);
      fclose(fp);
   }
   else
   {
      return false;
   }
   return true;
}


void testdrawbitmap(ArduiPi_OLED& display, const uint8_t *bitmap, uint8_t w, uint8_t h)
{
   uint8_t icons[NUMFLAKES][3];
   srandom(666);     // whatever seed

   // initialize
   for (uint8_t f=0; f< NUMFLAKES; f++)
   {
      icons[f][XPOS] = random() % display.width();
      icons[f][YPOS] = 0;
      icons[f][DELTAY] = random() % 5 + 1;

      printf("x: %d", icons[f][XPOS]);
      printf("y: %d", icons[f][YPOS]);
      printf("dy: %d\n", icons[f][DELTAY]);
   }

   while (1)
   {
      // draw each icon
      for (uint8_t f=0; f< NUMFLAKES; f++)
      {
         display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, WHITE);
      }
      display.display();
      usleep(100000);

      // then erase it + move it
      for (uint8_t f=0; f< NUMFLAKES; f++)
      {
         display.drawBitmap(icons[f][XPOS], icons[f][YPOS],  logo16_glcd_bmp, w, h, BLACK);
         // move it
         icons[f][YPOS] += icons[f][DELTAY];
         // if its gone, reinit
         if (icons[f][YPOS] > display.height())
         {
            icons[f][XPOS] = random() % display.width();
            icons[f][YPOS] = 0;
            icons[f][DELTAY] = random() % 5 + 1;
         }
      }
   }
}

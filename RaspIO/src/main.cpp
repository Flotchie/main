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

#include "RH_RF69.h"
#include "bcm2835.h"
#include <iostream>
#include <unistd.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <ctime>

#include "ini/SimpleIni.h"
#include "StringTools.h"
using namespace std;

string ini_file = "/home/pi/remote-debugging/RaspIO.ini";
string tprobe_path = "/home/pi/data/tprobe/";
string w1_devices_path = "/sys/bus/w1/devices/";
uint8_t slaveSelectPin = SS;
uint8_t interruptPin = 5;

bool writeProbe(const std::string& probe, const std::chrono::system_clock::time_point& now, double temperature);

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
	   return 1;
	}
	RH_RF69 rf69(slaveSelectPin, interruptPin);
   if (!rf69.init())
   {
	    Serial.println("rf69::init() failed");
	    return 2;
   }

   if (!rf69.setFrequency(868.0))
   {
      Serial.println("setFrequency failed");
      return 3;
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

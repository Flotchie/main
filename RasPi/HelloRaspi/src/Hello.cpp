//============================================================================
// Name        : Hello.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "RH_RF69.h"
#include "bcm2835.h"
#include <unistd.h>
#include <dirent.h>
using namespace std;

int main()
{
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!

    //----- SET SPI MODE -----
    //SPI_MODE_0 (0,0) 	CPOL = 0, CPHA = 0, Clock idle low, data is clocked in on rising edge, output data (change) on falling edge
    //SPI_MODE_1 (0,1) 	CPOL = 0, CPHA = 1, Clock idle low, data is clocked in on falling edge, output data (change) on rising edge
    //SPI_MODE_2 (1,0) 	CPOL = 1, CPHA = 0, Clock idle high, data is clocked in on falling edge, output data (change) on rising edge
    //SPI_MODE_3 (1,1) 	CPOL = 1, CPHA = 1, Clock idle high, data is clocked in on rising, edge output data (change) on falling edge
    //spi_mode = SPI_MODE_0;
    //----- SET BITS PER WORD -----
    //spi_bitsPerWord = 8;
    //----- SET SPI BUS SPEED -----
    //spi_speed = 1000000;		//1000000 = 1MHz (1uS per bit)

	//Spi spi(0,SPI_MODE_0,8,1000000);
	//spi.open();
	//spi.close();
	string w1_devices_path = "/sys/bus/w1/devices/";
	uint8_t slaveSelectPin = SS;
	uint8_t interruptPin = 5;
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

   // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
   // No encryption
   if (!rf69.setFrequency(868.0))
   {
      Serial.println("setFrequency failed");
      return 3;
   }
   // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
   // ishighpowermodule flag set like this:
   //rf69.setTxPower(14, true);
   // The encryption key has to be the same as the one in the client
   uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
   rf69.setEncryptionKey(key);
   uint8_t rf69Buf[RH_RF69_MAX_MESSAGE_LEN];
   while(true)
   {
      // RF69 : température eau piscine
      if (rf69.available())
      {
        // Should be a message for us now
        uint8_t len = sizeof(rf69Buf);
        if (rf69.recv(rf69Buf, &len))
        {
           Serial.print("got frame: ");
           rf69Buf[len] = 0;
           Serial.println((char*)rf69Buf);
        }
        else
        {
           Serial.println("recv failed");
        }
      }
      // DS18B20 : sondes de température locales

      DIR *dir;
      struct dirent *ent;
      if ((dir = opendir (w1_devices_path.c_str())) != NULL) {
        // print all the files and directories within directory
        while ((ent = readdir (dir)) != NULL)
        {
           if(strncmp(ent->d_name,"28-",3)==0)
           {
              printf ("%s%s\n", w1_devices_path.c_str(), ent->d_name);
           }
        }
        closedir (dir);
      }
      // pause de 500ms
      usleep(500000);
   }
   return 0;
}

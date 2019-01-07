/******************************************************************
 This is the common ArduiPi include file for ArduiPi board
 
02/18/2013  Charles-Henri Hallard (http://hallard.me)
            Modified for compiling and use on Raspberry ArduiPi Board
            LCD size and connection are now passed as arguments on 
            the command line (no more #define on compilation needed)
            ArduiPi project documentation http://hallard.me/arduipi

07/26/2013  Charles-Henri Hallard (http://hallard.me)
            Done generic library for different OLED type
            
 Written by Charles-Henri Hallard for Fun .
 All text above must be included in any redistribution.
            
 ******************************************************************/

#ifndef _ArduiPi_OLED_lib_H
#define _ArduiPi_OLED_lib_H

#include <stdio.h>
#include <stdarg.h>  
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

// Configuration Pin for ArduiPi board
#define OLED_SPI_RESET RPI_V2_GPIO_P1_22 /* GPIO 25 pin 22  */
#define OLED_SPI_DC    RPI_V2_GPIO_P1_18 /* GPIO 24 pin 18  */
#define OLED_I2C_RESET RPI_V2_GPIO_P1_22 /* GPIO 25 pin 12  */

// OLED type I2C Address
#define ADAFRUIT_I2C_ADDRESS   0x3C /* 011110+SA0+RW - 0x3C or 0x3D */
// Address for 128x32 is 0x3C
// Address for 128x32 is 0x3D (default) or 0x3C (if SA0 is grounded)

#define SEEED_I2C_ADDRESS   0x3C /* 011110+SA0+RW - 0x3C or 0x3D */

#define SH1106_I2C_ADDRESS   0x3C 

  
// Oled supported display
#define OLED_ADAFRUIT_SPI_128x32  0
#define OLED_ADAFRUIT_SPI_128x64  1
#define OLED_ADAFRUIT_I2C_128x32  2
#define OLED_ADAFRUIT_I2C_128x64  3
#define OLED_SEEED_I2C_128x64     4
#define OLED_SEEED_I2C_96x96      5
#define OLED_SH1106_I2C_128x64    6
#define OLED_SH1106_SPI_128x64    7

#define OLED_LAST_OLED            8 /* always last type, used in code to end array */

const char* getOledType(unsigned int iType);

#endif

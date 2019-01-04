// RasPi.cpp
//
// Routines for implementing RadioHead on Raspberry Pi
// using BCM2835 library for GPIO
//
// Contributed by Mike Poublon and used with permission


#include <RadioHead.h>

#if (RH_PLATFORM == RH_PLATFORM_RASPI)
#include <sys/time.h>
#include <time.h>
#include "RasPi.h"
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <map>

//Initialize the values for sanity
timeval RHStartTime;
std::map<uint8_t,void (*)()> MapInterrupt;
std::mutex MapInterrupt_mutex;
pthread_t thread = 0;


void SPIClass::begin()
{
  //Set SPI Defaults
  uint16_t divider = BCM2835_SPI_CLOCK_DIVIDER_256;
  uint8_t bitorder = BCM2835_SPI_BIT_ORDER_MSBFIRST;
  uint8_t datamode = BCM2835_SPI_MODE0;

  begin(divider, bitorder, datamode);
}

void SPIClass::begin(uint16_t divider, uint8_t bitOrder, uint8_t dataMode)
{
  setClockDivider(divider);
  setBitOrder(bitOrder);
  setDataMode(dataMode);

  //Set CS pins polarity to low
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, 0);

  bcm2835_spi_begin();

  //Initialize a timestamp for millis calculation
  gettimeofday(&RHStartTime, NULL);
}

void SPIClass::end()
{
  //End the SPI
  bcm2835_spi_end();
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
  //Set the SPI bit Order
  bcm2835_spi_setBitOrder(bitOrder);
}

void SPIClass::setDataMode(uint8_t mode)
{
  //Set SPI data mode
  bcm2835_spi_setDataMode(mode);
}

void SPIClass::setClockDivider(uint16_t rate)
{
  //Set SPI clock divider
  bcm2835_spi_setClockDivider(rate);
}

byte SPIClass::transfer(byte _data)
{
  //Set which CS pin to use for next transfers
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
  //Transfer 1 byte
  byte data;
  data = bcm2835_spi_transfer((uint8_t)_data);
  return data;
}

void pinMode(unsigned char pin, unsigned char mode)
{
  if (mode == OUTPUT)
  {
    bcm2835_gpio_fsel(pin,BCM2835_GPIO_FSEL_OUTP);
  }
  else
  {
    bcm2835_gpio_fsel(pin,BCM2835_GPIO_FSEL_INPT);
  }
}

void digitalWrite(unsigned char pin, unsigned char value)
{
  bcm2835_gpio_write(pin,value);
}

unsigned char digitalRead(unsigned char pin)
{
   return bcm2835_gpio_lev(pin);
}

unsigned long millis()
{
  //Declare a variable to store current time
  struct timeval RHCurrentTime;
  //Get current time
  gettimeofday(&RHCurrentTime,NULL);
  //Calculate the difference between our start time and the end time
  unsigned long difference = ((RHCurrentTime.tv_sec - RHStartTime.tv_sec) * 1000);
  difference += ((RHCurrentTime.tv_usec - RHStartTime.tv_usec)/1000);
  //Return the calculated value
  return difference;
}

void delay (unsigned long ms)
{
  //Implement Delay function
  struct timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=(ms * 1000);
  nanosleep(&ts,&ts);
}

long random(long min, long max)
{
  long diff = max - min;
  long ret = diff * rand() + min;
  return ret;
}

void SerialSimulator::begin(int baud)
{
  //No implementation neccesary - Serial emulation on Linux = standard console
  //
  //Initialize a timestamp for millis calculation - we do this here as well in case SPI
  //isn't used for some reason
  gettimeofday(&RHStartTime, NULL);
}

size_t SerialSimulator::println(const char* s)
{
  print(s);
  return printf("\n");
}

size_t SerialSimulator::print(const char* s)
{
  return printf(s);
}

size_t SerialSimulator::print(unsigned int n, int base)
{
  if (base == DEC)
    return printf("%d", n);
  else if (base == HEX)
    return printf("%02x", n);
  else if (base == OCT)
    return printf("%o", n);
  // TODO: BIN
  return 0;
}

size_t SerialSimulator::print(char ch)
{
   return printf("%c", ch);
}

size_t SerialSimulator::println(char ch)
{
   return printf("%c\n", ch);
}

size_t SerialSimulator::print(unsigned char ch, int base)
{
  return print((unsigned int)ch, base);
}

size_t SerialSimulator::println(unsigned char ch, int base)
{
  print((unsigned int)ch, base);
  return printf("\n");
}

void *WatchInterrupt(void *pinInterrupt)
{
   while(true)
   {
      usleep(50000); // 50ms
      MapInterrupt_mutex.lock();
      for(auto& pI: MapInterrupt)
      {
         if(digitalRead(pI.first)) pI.second();
      }
      MapInterrupt_mutex.unlock();
   }
   pthread_exit(NULL);
}

void attachInterrupt(uint8_t interruptNumber, void(* isr)(), uint8_t mode)
{
   /*
   if(thread==0)
   {
      pthread_create(&thread, NULL, WatchInterrupt, (void *)0);
   }
   MapInterrupt_mutex.lock();
   MapInterrupt.insert(std::make_pair(interruptNumber, isr));
   MapInterrupt_mutex.unlock();
   */
}


#endif

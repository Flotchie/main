/*
 * Spi.cpp
 *
 *  Created on: 6 Dec 2018
 *      Author: pi
 */

#include "Spi.h"

#include <fcntl.h>				//Needed for SPI port
#include <sys/ioctl.h>			//Needed for SPI port
#include <unistd.h>			//Needed for SPI port
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>

Spi::Spi(int device, unsigned char mode, unsigned char bitsPerWord, unsigned int speed)
: m_device(device)
, m_mode(mode)
, m_bitsPerWord(bitsPerWord)
, m_speed(speed)
, m_cs_fd(0)
{
}

Spi::~Spi()
{
}

//m_device	0=CS0, 1=CS1
int Spi::open()
{
	int status_value = -1;

    if (m_device)
    	m_cs_fd = ::open(std::string("/dev/spidev0.1").c_str(), O_RDWR);
    else
    	m_cs_fd = ::open(std::string("/dev/spidev0.0").c_str(), O_RDWR);

    if (m_cs_fd < 0)
    {
        perror("Error - Could not open SPI device");
        exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_WR_MODE, &m_mode);
    if(status_value < 0)
    {
        perror("Could not set SPIMode (WR)...ioctl fail");
        exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_RD_MODE, &m_mode);
    if(status_value < 0)
    {
      perror("Could not set SPIMode (RD)...ioctl fail");
      exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_WR_BITS_PER_WORD, &m_bitsPerWord);
    if(status_value < 0)
    {
      perror("Could not set SPI bitsPerWord (WR)...ioctl fail");
      exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_RD_BITS_PER_WORD, &m_bitsPerWord);
    if(status_value < 0)
    {
      perror("Could not set SPI bitsPerWord(RD)...ioctl fail");
      exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_speed);
    if(status_value < 0)
    {
      perror("Could not set SPI speed (WR)...ioctl fail");
      exit(1);
    }

    status_value = ioctl(m_cs_fd, SPI_IOC_RD_MAX_SPEED_HZ, &m_speed);
    if(status_value < 0)
    {
      perror("Could not set SPI speed (RD)...ioctl fail");
      exit(1);
    }
    return status_value;
}



//************************************
//************************************
//********** SPI CLOSE PORT **********
//************************************
//************************************
int Spi::close()
{
	int status_value = -1;

    status_value = ::close(m_cs_fd);
    if(status_value < 0)
    {
    	perror("Error - Could not close SPI device");
    	exit(1);
    }
    return(status_value);
}



//*******************************************
//*******************************************
//********** SPI WRITE & READ DATA **********
//*******************************************
//*******************************************
//data		Bytes to write.  Contents is overwritten with bytes read.
int Spi::writeAndRead (unsigned char *data, int length)
{
	struct spi_ioc_transfer spi[length];
	int i = 0;
	int retVal = -1;

	//one spi transfer for each byte
	for (i = 0 ; i < length ; i++)
	{
		memset(&spi[i], 0, sizeof (spi[i]));
		spi[i].tx_buf        = (unsigned long)(data + i); // transmit from "data"
		spi[i].rx_buf        = (unsigned long)(data + i) ; // receive into "data"
		spi[i].len           = sizeof(*(data + i)) ;
		spi[i].delay_usecs   = 0 ;
		spi[i].speed_hz      = m_speed ;
		spi[i].bits_per_word = m_bitsPerWord ;
		spi[i].cs_change = 0;
	}

	retVal = ioctl(m_cs_fd, SPI_IOC_MESSAGE(length), &spi) ;

	if(retVal < 0)
	{
		perror("Error - Problem transmitting spi data..ioctl");
		exit(1);
	}

	return retVal;
}

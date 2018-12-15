/*
 * Spi.h
 *
 *  Created on: 6 Dec 2018
 *      Author: pi
 */

#ifndef SPI_H_
#define SPI_H_

#include <linux/spi/spidev.h>	//Needed for SPI port

class Spi {
public:
	Spi(int device, unsigned char mode, unsigned char bitsPerWord, unsigned int  speed);
	virtual ~Spi();
	int open();
	int close();
	int writeAndRead (unsigned char *data, int length);

private:
	int           m_device;
	unsigned char m_mode;
	unsigned char m_bitsPerWord;
	unsigned int  m_speed;
	int           m_cs_fd;
};

#endif /* SPI_H_ */

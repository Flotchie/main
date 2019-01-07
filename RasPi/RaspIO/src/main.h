/*
 * main.h
 *
 *  Created on: 5 janv. 2019
 *      Author: Bertrand
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <string>
#include <chrono>
#include "ArduiPi_OLED.h"

bool writeProbe(const std::string& probe, const std::chrono::system_clock::time_point& now, double temperature);
void testdrawbitmap(ArduiPi_OLED& display, const uint8_t *bitmap, uint8_t w, uint8_t h);

#endif /* MAIN_H_ */

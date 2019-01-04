/*
 * Date.h
 *
 *  Created on: 23 déc. 2018
 *      Author: Bertrand
 */

#ifndef DATE_H_
#define DATE_H_

#include  <ctime>

class Date
{
public:
   Date();
   static Date Now();
   virtual ~Date();
private:

};

#endif /* DATE_H_ */

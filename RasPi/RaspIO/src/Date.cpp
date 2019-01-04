/*
 * Date.cpp
 *
 *  Created on: 23 déc. 2018
 *      Author: Bertrand
 */

#include "Date.h"
#include <chrono>

using namespace std;

Date::Date()
{
   // TODO Auto-generated constructor stub

}

Date Date::Now()
{
   auto end = chrono::system_clock::now();
}

Date::~Date()
{
   // TODO Auto-generated destructor stub
}


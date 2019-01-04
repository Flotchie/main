/*
 * main.h
 *
 *  Created on: 4 janv. 2019
 *      Author: Bertrand
 */

#ifndef MAIN_H_
#define MAIN_H_

class Data
{
public:
   Data(const std::string& name, const std::string& type)
   : m_name(name), m_type(type)
   {
   }
   std::string m_date;
   std::string m_name;
   std::string m_type;
   std::string m_value;
};

bool readData(Data& data);

#endif /* MAIN_H_ */

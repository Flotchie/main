/*
 * Data.h
 *
 *  Created on: 29 janv. 2019
 *      Author: Bertrand
 */

#ifndef DATA_H_
#define DATA_H_

#include <string>
#include <algorithm>
#include <map>
#include "ini/SimpleIni.h"

class VariableManager;
class Database;
class Variable;

class Data
{
   friend class VariableManager;
   friend class Database;
   friend class Variable;
public:
   bool isInput() { return m_input; }
   std::string m_date;
   bool        m_undefinedValue;
   enum DataType
   {
      DT_FLOAT,
      DT_BOOL
   };
   DataType getType() { return m_type; }
   static bool InitFromIni(CSimpleIniA& ini, std::map<std::string, Data*>& datas);
   static void SplitName(const std::string& fullName, std::string& path
         , std::string& shortName);
   static bool SetData(const std::string& dataPath, const std::string& path
         , const std::string& shortName
         , const std::string& date, const std::string& value );
   static std::string DateNow();
protected:
   Data(const std::string& name, DataType type, bool input)
   : m_undefinedValue(true)
   , m_name(name)
   , m_type(type)
   , m_input(input)
   , m_dbId(0)
   {
      SplitName(m_name, m_path, m_shortName);
   }

   virtual ~Data() {}

   static bool ReadData(const std::string& dataPath, Data& data, bool& changed);
   static bool SetData(const std::string& dataPath, Data& data);
   std::string m_name;
   std::string m_shortName;
   std::string m_path;
   DataType    m_type;
   bool        m_input;
   int64_t     m_dbId;
};

template<typename T> class TData : public Data
{
public:
   TData(const std::string& name, DataType type, bool readOnly) : Data(name,type,readOnly) {}
   T m_value;
};



#endif /* DATA_H_ */

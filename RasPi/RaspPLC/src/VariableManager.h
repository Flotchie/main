/*
 * VariableManager2.h
 *
 *  Created on: 8 janv. 2019
 *      Author: Bertrand
 */

#ifndef VARIABLEMANAGER_H_
#define VARIABLEMANAGER_H_

#include "sqlite3.h"
#include "ini/SimpleIni.h"
#include <string>
#include <vector>
#include <chrono>
#include <math.h>

class VariableManager;

class Data
{
   friend class VariableManager;
public:
   bool isInput() { return m_input; }
   std::string m_date;
   bool        m_undefinedValue;
protected:
   enum DataType
   {
      DT_FLOAT,
      DT_BOOL
   };

   Data(const std::string& name, DataType type, bool input)
   : m_undefinedValue(true)
   , m_name(name)
   , m_type(type)
   , m_input(input)
   , m_dbId(0)
   {
      m_dir = m_name;
      std::replace( m_dir.begin(), m_dir.end(), '.', '/');
      std::size_t posLastDot = m_name.find_last_of('.');
      if(posLastDot==std::string::npos)
         m_shortName = m_name;
      else
         m_shortName = m_name.substr(posLastDot+1);
   }

   virtual ~Data() {}

   std::string m_name;
   std::string m_shortName;
   std::string m_dir;
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

class Variable
{
   friend class VariableManager;
public:
   Variable(VariableManager& vm, const std::string& name);
   virtual ~Variable() {}
protected:
   virtual bool link(Data& data) = 0;
   void set(Data& data);
   std::string m_name;
   VariableManager& m_owner;
};

template<typename T> class TVariableBase : public Variable
{
   friend class VariableManager;
public:
   TVariableBase(VariableManager& vm, const std::string& name)
      : Variable(vm, name)
      , m_data(nullptr)
      {}
   T get() { return m_data->m_value; }
protected:
   bool link(Data& data) override;
   TData<T>* m_data;
};

template<typename T> class TVariable : public TVariableBase<T>
{
   friend class VariableManager;
public:
   TVariable(VariableManager& vm, const std::string& name) : TVariableBase<T>(vm, name) {}
   void set(T value);
protected:
   bool link(Data& data) override;
};

template<typename T> class TInput : public TVariableBase<T>
{
   friend class VariableManager;
public:
   TInput(VariableManager& vm, const std::string& name) : TVariableBase<T>(vm, name) {}
protected:
   bool link(Data& data) override;
};

class VariableManager
{
   friend class Variable;
public:
   VariableManager(CSimpleIniA& ini, const std::string& dataPath);
   void readInput();
   bool linkVariables();
protected:
   void addVariable(Variable* variable);
   bool setData(Data& data);
private:
   bool initDb();
   void readDataInDb();
   void initDataInDb();
   bool readData(Data& data);
   bool writeInDb(Data& data);

   std::string m_dataPath;
   std::map<std::string, Data*> m_datas;
   std::vector<Variable*> m_variables;
   sqlite3 *m_db;
   sqlite3_stmt* m_insertDataStatement;
   sqlite3_stmt* m_insertDataEventStatement;
   sqlite3_stmt* m_updateLastEventStatement;
   sqlite3_stmt* m_selectDataStatement;
};


template<class T> bool TVariableBase<T>::link(Data& data)
{
   m_data = &dynamic_cast<TData<T>&>(data);
   if(m_data==nullptr)
   {
      printf("ERROR - variable[%s] bad type", m_name.c_str());
      return false;
   }
   return true;
}

template<class T> void TVariable<T>::set(T value)
{
   if( (TVariableBase<T>::m_data->m_value!=value) || TVariableBase<T>::m_data->m_undefinedValue)
   {
      auto now = std::chrono::system_clock::now();
      std::time_t tt = std::chrono::system_clock::to_time_t(now);
      char date[40];
      strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&tt));
      TVariableBase<T>::m_data->m_date = date;
      TVariableBase<T>::m_data->m_value = value;
      Variable::set(*TVariableBase<T>::m_data);
   }
}

template<class T> bool TVariable<T>::link(Data& data)
{
   if(data.isInput())
   {
      printf("ERROR - variable[%s] : data is only input, instanciate TInput instead", Variable::m_name.c_str());
      return false;
   }
   return TVariableBase<T>::link(data);
}

template<class T> bool TInput<T>::link(Data& data)
{
   if(!data.isInput())
   {
      printf("ERROR - variable[%s] : data is not input only, instanciate TVariable instead", Variable::m_name.c_str());
      return false;
   }
   return TVariableBase<T>::link(data);
}

#endif /* VARIABLEMANAGER_H_ */

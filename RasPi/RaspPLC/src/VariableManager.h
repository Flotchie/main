/*
 * VariableManager2.h
 *
 *  Created on: 8 janv. 2019
 *      Author: Bertrand
 */

#ifndef VARIABLEMANAGER_H_
#define VARIABLEMANAGER_H_

#include "Database.h"
#include "ini/SimpleIni.h"
#include <string>
#include <vector>
#include <math.h>


class Variable
{
   friend class VariableManager;
public:
   Variable(VariableManager& vm, const std::string& name);
   virtual ~Variable() {}
   const std::string& getName() { return m_name; }
   virtual Data::DataType getType() =0;
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
   Data::DataType getType() override { return m_data->getType(); }
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

template<typename T> class TOutput : public TVariable<T>
{
   friend class VariableManager;
public:
   TOutput(VariableManager& vm, const std::string& name) : TVariable<T>(vm, name) {}
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
   const std::map<std::string, Data*>& getDatas();
protected:
   std::string m_dataPath;
   void addVariable(Variable* variable);
   bool setData(Data& data);
private:
   bool readData(Data& data);
   std::map<std::string, Data*> m_datas;
   std::vector<Variable*> m_variables;
   Database* m_database;
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
      TVariableBase<T>::m_data->m_date = Data::DateNow();
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

template<class T> bool TOutput<T>::link(Data& data)
{
   if(!data.isInput())
   {
      printf("ERROR - variable[%s] : data is not an input", Variable::m_name.c_str());
      return false;
   }
   return TVariableBase<T>::link(data);
}

#endif /* VARIABLEMANAGER_H_ */

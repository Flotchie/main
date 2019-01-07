/*
 * main.h
 *
 *  Created on: 4 janv. 2019
 *      Author: Bertrand
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "ini/SimpleIni.h"

class VariableManager;

class Data
{
   friend class VariableManager;
public:
   bool isInput() { return m_input; }
protected:
   enum DataType
   {
      DT_FLOAT,
      DT_BOOL
   };

   Data(const std::string& name, DataType type, bool input)
   : m_name(name)
   , m_type(type)
   , m_input(input)
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

   std::string m_date;
   std::string m_name;
   std::string m_shortName;
   std::string m_dir;
   DataType    m_type;
   bool        m_input;
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
   VariableManager(CSimpleIniA& ini);
   void readData(bool inputOnly);
   bool linkVariables();
protected:
   void addVariable(Variable* variable);
   bool setData(Data& data);
private:
   bool readData(Data& data);
   std::map<std::string, Data*> m_datas;
   std::vector<Variable*> m_variables;
};

#endif /* MAIN_H_ */

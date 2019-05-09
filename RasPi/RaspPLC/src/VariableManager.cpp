/*
 * VariableManager2.cpp
 *
 *  Created on: 8 janv. 2019
 *      Author: Bertrand
 */

#include "VariableManager.h"
#include "StringTools.h"


using namespace std;

VariableManager::VariableManager(CSimpleIniA& ini, const string& dataPath )
: m_dataPath(dataPath)
, m_database(nullptr)
{
   Data::InitFromIni(ini, m_datas);

   m_database = new Database(m_dataPath);

   m_database->initDb();

   // lecture des Data en base
   m_database->read(m_datas);

   // création des Data non présentes en base mais décrites dans le .ini
   m_database->init(m_datas);

   // lecture des entrées fichier
   readInput();
}


const std::map<std::string, Data*>& VariableManager::getDatas()
{
   return m_datas;
}


void VariableManager::addVariable(Variable* variable)
{
   m_variables.push_back(variable);
}


bool VariableManager::linkVariables()
{
   bool res = true;
   for(auto& variable: m_variables)
   {
      map<string, Data*>::iterator it = m_datas.find(variable->m_name);
      if(it==m_datas.end())
      {
         printf("ERROR - variable[%s] undefined", variable->m_name.c_str());
         res = false;
      }
      else
      {
         res &= variable->link(*(*it).second);
      }
   }
   return res;
}


void VariableManager::readInput()
{
   for(auto data:m_datas)
   {
      bool changed = false;
      if(data.second->m_input)
      {
         Data::ReadData(m_dataPath, *data.second, changed);
         if(changed) m_database->write(*data.second);
      }
   }
}

bool VariableManager::setData(Data& data)
{
   bool res = Data::SetData(m_dataPath, data);
   // écriture en base
   res &= m_database->write(data);
   return res;
}

Variable::Variable(VariableManager& owner, const std::string& name)
: m_name(name)
, m_owner(owner)
{
   m_owner.addVariable(this);
}

void Variable::set(Data& data)
{
   m_owner.setData(data);
}

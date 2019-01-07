//============================================================================
// Name        : main.cpp
// Author      : Bertrand
// Version     :
// Copyright   : Youpee
// Description : Gestion de la piscine
//               - protection contre le gelen fonction de la température local
//               - temps de filtration en fonction de la température de l'eau
//               - eclairages
//               - pompe nage à contre-courant
//============================================================================

#include <iostream>
#include <unistd.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <ctime>

#include "StringTools.h"
#include "json/json.h"
#include "main.h"
using namespace std;


string data_path = "/home/pi/data/";

bool writeProbe(const std::string& probe, const std::chrono::system_clock::time_point& now, double temperature);

int main()
{
   char exe[1024];
   int ret;
   ret = readlink("/proc/self/exe",exe,sizeof(exe)-1);
   if(ret ==-1) {
       fprintf(stderr,"ERRORRRRR\n");
       exit(1);
   }
   exe[ret] = 0;
   string ini_file = form("%s.ini", exe);

	map<string,string> ds18b20s;

   CSimpleIniA ini(true, true, true);
   ini.Reset();
   if (ini.LoadFile(ini_file.c_str()) < 0)
   {
      return 1;
   }

   VariableManager vm(ini);

   // lecture de toutes les données
   vm.readData(false);

   TInput<double> tempAir(vm, "tprobe.air");
   TInput<double> tempEau(vm, "tprobe.eau");
   TInput<double> tempLocal(vm, "tprobe.local");
   TVariable<bool> filteringPump(vm, "relay.filteringPump");

   if(!vm.linkVariables())
   {
      return 2;
   }

   //auto previous = chrono::system_clock::now();
   while(true)
   {
      // lecture des informations en entrée
      vm.readData(true);

      // Gestion de la piscine
      if(tempLocal.get()<20) filteringPump.set(true);


      // pause de 100ms
      usleep(100000);
   }
   return 0;
}


VariableManager::VariableManager(CSimpleIniA& ini)
{
   CSimpleIniA::TNamesDepend variableKeys;
   ini.GetAllKeys("variables", variableKeys);
   for(auto& key : variableKeys)
   {
      const char* value = ini.GetValue("variables",  key.pItem);
      vector<string> infos;
      Tokenize(value,",",infos);
      string sType = infos[0];
      bool input = false;
      if(infos.size()>1)
      {
         if(infos[1]=="input") input = true;
      }
      if(sType=="bool")
      {
         m_datas.insert(make_pair(key.pItem, new TData<bool>(key.pItem, Data::DT_BOOL, input)));
      }
      else if(sType=="float")
      {
         m_datas.insert(make_pair(key.pItem, new TData<double>(key.pItem, Data::DT_FLOAT, input)));
      }
      else
      {
         printf("ERROR - invalid type[%s]", sType.c_str());
      }
   }
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


void VariableManager::readData(bool inputOnly)
{
   for(auto& data:m_datas)
   {
      if(data.second->m_input || !inputOnly) readData(*data.second);
   }
}

/**
 * lecture d'une donnée
 * {
 *    "name": "eau",
 *    "date": "2018-12-30 15:55:48",
 *    "value": 19.2
 * }
 */
bool VariableManager::readData(Data& data)
{
   string path = form("%s%s", data_path.c_str(), data.m_dir.c_str());
   FILE* fp = fopen(path.c_str(), "rb");
   if(fp!=NULL)
   {
      fseek(fp, 0, SEEK_END);
      long fsize = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      char *json = (char*)malloc(fsize + 1);
      fread(json, fsize, 1, fp);
      fclose(fp);
      json[fsize] = 0;
      Json::Reader reader;
      Json::Value root;
      reader.parse(json,root,false);
      data.m_date = root["date"].asString();
      if(data.m_type==Data::DT_BOOL)
         dynamic_cast<TData<bool>&>(data).m_value = root["value"].asBool();
      else if(data.m_type==Data::DT_FLOAT)
         dynamic_cast<TData<double>&>(data).m_value = root["value"].asDouble();
   }
   else
   {
      return false;
   }
   return true;
}

bool VariableManager::setData(Data& data)
{
   auto now = chrono::system_clock::now();
   string path = form("%s%s", data_path.c_str(), data.m_dir.c_str());
   FILE* fp = fopen(path.c_str(), "wb");
   if(fp!=NULL)
   {
      time_t tt = chrono::system_clock::to_time_t(now);
      char date[40];
      strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&tt));
      fputs("{\n",fp);
      fputs(form("\"name\": \"%s\",\n", data.m_shortName.c_str()).c_str(),fp);
      fputs(form("\"date\": \"%s\",\n", date).c_str(),fp);
      if(data.m_type==Data::DT_BOOL)
         fputs(form("\"value\": %s\n", dynamic_cast<TData<bool>&>(data).m_value?"true":"false").c_str(),fp);
      else if(data.m_type==Data::DT_FLOAT)
         fputs(form("\"value\": %.1f\n", dynamic_cast<TData<double>&>(data).m_value).c_str(),fp);
      fputs("}\n",fp);
      fclose(fp);
   }
   else
   {
      return false;
   }
   return true;
}

Variable::Variable(VariableManager& vm, const std::string& name)
: m_name(name)
, m_owner(vm)
{
   vm.addVariable(this);
}

void Variable::set(Data& data)
{
   m_owner.setData(data);
}

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
   if(TVariableBase<T>::m_data->m_value != value)
   {
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


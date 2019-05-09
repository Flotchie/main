/*
 * Data.cpp
 *
 *  Created on: 29 janv. 2019
 *      Author: Bertrand
 */


#include "Data.h"
#include "StringTools.h"
#include "json/json.h"
#include <chrono>

using namespace std;

bool Data::InitFromIni(CSimpleIniA& ini, std::map<std::string, Data*>& datas)
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
          datas.insert(make_pair(key.pItem, new TData<bool>(key.pItem, Data::DT_BOOL, input)));
       }
       else if(sType=="float")
       {
          datas.insert(make_pair(key.pItem, new TData<double>(key.pItem, Data::DT_FLOAT, input)));
       }
       else
       {
          printf("ERROR - invalid type[%s]", sType.c_str());
          return false;
       }
   }
   return true;
}

void Data::SplitName(const std::string& fullName, std::string& path
      , std::string& shortName)
{
   path = fullName;
   std::replace( path.begin(), path.end(), '.', '/');
   std::size_t posLastDot = fullName.find_last_of('.');
   if(posLastDot==std::string::npos)
      shortName = fullName;
   else
      shortName = fullName.substr(posLastDot+1);
}

/**
 * lecture d'une donnée
 * {
 *    "name": "eau",
 *    "date": "2018-12-30 15:55:48",
 *    "value": 19.2
 * }
 */
bool Data::ReadData(const string& dataPath, Data& data, bool& changed)
{
   changed = false;
   string path = form("%s%s", dataPath.c_str(), data.m_path.c_str());
   FILE* fp = fopen(path.c_str(), "rb");
   if(fp!=NULL)
   {
      fseek(fp, 0, SEEK_END);
      long fsize = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      char *json = (char*)malloc(fsize + 1);
      if(json==nullptr)
      {
         fprintf(stderr, "VariableManager::readData: malloc failed");
         fclose(fp);
         return false;
      }
      fread(json, fsize, 1, fp);
      fclose(fp);
      json[fsize] = 0;
      Json::Reader reader;
      Json::Value root;
      reader.parse(json,root,false);
      if(data.m_type==Data::DT_BOOL)
      {
         bool newValue = root["value"].asBool();
         bool& value = dynamic_cast<TData<bool>&>(data).m_value;
         if( (newValue!=value) || data.m_undefinedValue)
         {
            data.m_undefinedValue = false;
            data.m_date = root["date"].asString();
            value = newValue;
            changed = true;
         }
      }
      else if(data.m_type==Data::DT_FLOAT)
      {
         double newValue = root["value"].asDouble();
         double& value = dynamic_cast<TData<double>&>(data).m_value;
         if( (newValue!=value) || data.m_undefinedValue)
         {
            data.m_undefinedValue = false;
            data.m_date = root["date"].asString();
            value = newValue;
            changed = true;
         }
      }
      free(json);
   }
   else
   {
      return false;
   }
   return true;
}

bool Data::SetData(const string& dataPath,Data& data)
{
   data.m_undefinedValue = false;
   string value;
   if(data.m_type==Data::DT_BOOL)
      value = dynamic_cast<TData<bool>&>(data).m_value?"true":"false";
   else if(data.m_type==Data::DT_FLOAT)
      value=form("%.1f", dynamic_cast<TData<double>&>(data).m_value);
   return SetData(dataPath, data.m_path, data.m_shortName, data.m_date, value);
}

bool Data::SetData(const string& dataPath, const string& path, const string& shortName
      , const string& date, const string& value )
{
   bool res = true;
   // écriture fichier
   string fullPath = form("%s%s", dataPath.c_str(), path.c_str());
   FILE* fp = fopen(fullPath.c_str(), "wb");
   if(fp!=NULL)
   {
      fputs("{\n",fp);
      fputs(form("\"name\": \"%s\",\n", shortName.c_str()).c_str(),fp);
      fputs(form("\"date\": \"%s\",\n", date.c_str()).c_str(),fp);
      fputs(form("\"value\": %s\n", value).c_str(),fp);
      fputs("}\n",fp);
      fclose(fp);
   }
   else
   {
      res=false;
   }
   return res;
}

string Data::DateNow()
{
   auto now = std::chrono::system_clock::now();
   std::time_t tt = std::chrono::system_clock::to_time_t(now);
   char date[40];
   strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   return string(date);
}

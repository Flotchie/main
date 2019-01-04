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

#include "ini/SimpleIni.h"
#include "StringTools.h"
#include "json/json.h"
#include "main.h"
using namespace std;

map<string, Data> datas;

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

   CSimpleIniA::TNamesDepend sections;
   ini.GetAllSections(sections);
   for(auto& section: sections)
   {
      CSimpleIniA::TNamesDepend keys;
      ini.GetAllKeys(section.pItem, keys);
      for(auto& key : keys)
      {
         const char* pszValue = ini.GetValue(section.pItem,  key.pItem);
         string name = form("%s.%s", section.pItem, pszValue);
         datas.insert(make_pair(name,Data(pszValue, section.pItem)));
      }
   }

   auto previous = chrono::system_clock::now();
   while(true)
   {
      // lecture des informations en entrée
      for(auto& data:datas)
      {
         readData(data.second);
      }

      // Gestion de la piscine

      //if(datas["local"].m_value)


      // pause de 100ms
      usleep(100000);
   }
   return 0;
}

/**
 * lecture d'une donnée
 * {
 *    "name": "eau",
 *    "date": "2018-12-30 15:55:48",
 *    "value": 19.2
 * }
 */
bool readData(Data& data)
{
   string path = form("%s%s/%s", data_path.c_str(), data.m_type.c_str(), data.m_name.c_str());
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
      data.m_value = root["value"].asString();
   }
   else
   {
      return false;
   }
   return true;
}

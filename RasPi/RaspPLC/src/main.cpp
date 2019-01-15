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
#include <iomanip>
#include <ctime>

#include "ini/SimpleIni.h"
#include "StringTools.h"
#include "main.h"
#include "VariableManager.h"

using namespace std;


string Data_path = "/home/pi/data/";

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
      fprintf(stderr,"cannot open ini file %s\n", ini_file.c_str());
      return 1;
   }

   VariableManager vm(ini, Data_path);

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
      vm.readInput();

      // Gestion de la piscine
      if(tempLocal.get()>=20)
         filteringPump.set(true);
      else
         filteringPump.set(false);

      // pause de 1s
      usleep(1000000);
   }
   return 0;
}

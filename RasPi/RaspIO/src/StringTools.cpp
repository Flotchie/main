/*
 * StrFormatter.cpp
 *
 *  Created on: 18 d�c. 2018
 *      Author: Bertrand
 */

#include <stdarg.h>
#include <dirent.h>
#include "StringTools.h"

#define MAX_STR 2048

std::string form(const char *fmt,...)
{
   va_list args;
   char buff[MAX_STR];

   va_start(args,fmt);
   vsnprintf(buff,MAX_STR,fmt,args);
   va_end(args);

   return buff;
}

bool ListDir( const std::string& path, std::set<std::string>& files )
{
   DIR *dir;
   struct dirent *ent;
   if ((dir = opendir (path.c_str())) != NULL)
   {
     // print all the files and directories within directory
     while ((ent = readdir (dir)) != NULL)
     {
        files.insert(ent->d_name);
     }
   }
   else
   {
      return false;
   }
   closedir(dir);
   return true;
}

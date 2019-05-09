/*
 * Database.h
 *
 *  Created on: 29 janv. 2019
 *      Author: Bertrand
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include "Data.h"
#include "sqlite3.h"
#include <map>
#include <vector>
#include <string>

class DataHistoric
{
public:
   Data* m_data;
   std::vector<std::string> m_dates;
   std::vector<double> m_values;
};

class Database
{
public:
   Database(const std::string& dataPath);
   virtual ~Database();
   bool initDb();
   void read(std::map<std::string, Data*>& datas);
   void init(const std::map<std::string, Data*>& datas);
   bool write(Data& data);
   void readHistoric(std::map<std::string, DataHistoric>& datas);
   void readHistoric(const std::string& name, DataHistoric& data);
private:
   std::string m_dataPath;
   sqlite3 *m_db;
   sqlite3_stmt* m_insertDataStatement;
   sqlite3_stmt* m_insertDataEventStatement;
   sqlite3_stmt* m_updateLastEventStatement;
   sqlite3_stmt* m_selectAllDataStatement;
};

#endif /* DATABASE_H_ */

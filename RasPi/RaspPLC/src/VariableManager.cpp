/*
 * VariableManager2.cpp
 *
 *  Created on: 8 janv. 2019
 *      Author: Bertrand
 */

#include "VariableManager.h"
#include "StringTools.h"
#include "json/json.h"


using namespace std;

VariableManager::VariableManager(CSimpleIniA& ini, const string& dataPath )
: m_dataPath(dataPath)
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

   initDb();

   // lecture des Data en base
   readDataInDb();

   // création des Data non présentes en base mais décrites dans le .ini
   initDataInDb();

   // lecture des entrées fichier
   readInput();
}

bool VariableManager::initDb()
{
   bool res = true;
   string dbName = form("%sdata.db", m_dataPath.c_str());
   int rc = sqlite3_open(dbName.c_str(), &m_db);
   if( rc )
   {
      printf("ERROR - can't open db %s\n", dbName.c_str());
      res = false;
   }
   else
   {
      const char* createDataSql = "CREATE TABLE IF NOT EXISTS data (id_data INTEGER PRIMARY KEY, name VARCHAR(100), last_event INTEGER );"
            "CREATE TABLE IF NOT EXISTS data_event (id_data_event INTEGER PRIMARY KEY, id_data INTEGER, date TEXT, value DOUBLE, FOREIGN KEY(id_data) REFERENCES data(id_data));"
            "CREATE INDEX IF NOT EXISTS index_last_event ON data(last_event);"
            "CREATE INDEX IF NOT EXISTS index_id_data ON data_event(id_data);";
      char *zErrMsg = 0;
      rc = sqlite3_exec(m_db, createDataSql, NULL, NULL, &zErrMsg);
      if( rc!=SQLITE_OK )
      {
         printf("initDb() createDataSql ERROR SQLITE: %s\n", sqlite3_errmsg(m_db));
         sqlite3_free(zErrMsg);
         res = false;
      }
      const char* insertDataSql = "INSERT INTO data (name) VALUES (?1)";
      rc = sqlite3_prepare_v3(m_db, insertDataSql, -1, 0, &m_insertDataStatement, nullptr);
      if( rc!=SQLITE_OK )
      {
         printf("initDb() insertDataSql ERROR SQLITE: %s\n", sqlite3_errmsg(m_db));
         res = false;
      }
      const char* insertDataEventSql = "INSERT INTO data_event (id_data, date, value ) VALUES (?1, ?2, ?3)";
      rc = sqlite3_prepare_v3(m_db, insertDataEventSql, -1, 0, &m_insertDataEventStatement, nullptr);
      if( rc!=SQLITE_OK )
      {
         printf("initDb() insertDataEventSql ERROR SQLITE: %s\n", sqlite3_errmsg(m_db));
         res = false;
      }
      const char* updateLastEventSql = "UPDATE data SET last_event=?1 WHERE id_data=?2";
      rc = sqlite3_prepare_v3(m_db, updateLastEventSql, -1, 0, &m_updateLastEventStatement, nullptr);
      if( rc!=SQLITE_OK )
      {
         printf("initDb() updateLastEventSql ERROR SQLITE: %s\n", sqlite3_errmsg(m_db));
         res = false;
      }
      const char* selectDataSql =
            "SELECT data.id_data, name, date, value FROM data "
            "LEFT JOIN data_event ON data.last_event=data_event.id_data_event";
      rc = sqlite3_prepare_v3(m_db, selectDataSql, -1, 0, &m_selectDataStatement, nullptr);
      if( rc!=SQLITE_OK )
      {
         printf("initDb() selectDataSql ERROR SQLITE: %s\n", sqlite3_errmsg(m_db));
         res = false;
      }
   }
   return res;
}


/*
 * création des Data décrites dans le .ini mais inexistante en base
 */
void VariableManager::initDataInDb()
{
   for(auto& pData: m_datas)
   {
      if(pData.second->m_dbId==0)
      {
         // écriture en base
         sqlite3_reset(m_insertDataStatement);
         sqlite3_bind_text (m_insertDataStatement, 1, pData.second->m_name.c_str(), -1, SQLITE_STATIC);
         int rc = sqlite3_step(m_insertDataStatement);
         if (rc != SQLITE_DONE)
         {
            printf("initDataInDb() insertDataStatement ERROR SQLITE: %d %s\n", rc, sqlite3_errmsg(m_db));
         }

         sqlite3_stmt* getDbIdStatement = nullptr;
         rc = sqlite3_prepare_v2(m_db, "SELECT last_insert_rowid()", -1, &getDbIdStatement, NULL);
         if (rc != SQLITE_OK)
         {
            printf("initDataInDb() getDbIdQuery ERROR SQLITE: %d %s\n", rc, sqlite3_errmsg(m_db));
         }
         else
         {
            if( SQLITE_ROW == sqlite3_step(getDbIdStatement) )
            {
               pData.second->m_dbId = sqlite3_column_int64(getDbIdStatement, 0); // id_data
            }
            sqlite3_finalize(getDbIdStatement);
         }
      }
   }
}


void VariableManager::readDataInDb()
{
   sqlite3_reset(m_selectDataStatement);
   while( SQLITE_ROW == sqlite3_step(m_selectDataStatement) )
   {
      int64_t dbId = sqlite3_column_int64(m_selectDataStatement, 0); // id_data
      string name = (const char*)sqlite3_column_text(m_selectDataStatement, 1); // name
      const char* date = (const char*)sqlite3_column_text(m_selectDataStatement, 2); // date
      double value     = sqlite3_column_double(m_selectDataStatement, 3); // value
      if(m_datas.find(name)!=m_datas.end())
      {
         Data& data = *m_datas[name];
         data.m_dbId = dbId;
         if (sqlite3_column_type(m_selectDataStatement, 2) != SQLITE_NULL)
         {
            data.m_undefinedValue = false;
            data.m_date = date;
            if(data.m_type==Data::DT_FLOAT)
            {
               dynamic_cast<TData<double>&>(data).m_value = value;
            }
            else if(data.m_type==Data::DT_BOOL)
            {
               dynamic_cast<TData<bool>&>(data).m_value = (value==1.0);
            }
         }
      }
   }
   sqlite3_finalize(m_selectDataStatement);
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
   for(auto& data:m_datas)
   {
      if(data.second->m_input) readData(*data.second);
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
   string path = form("%s%s", m_dataPath.c_str(), data.m_dir.c_str());
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
            writeInDb(data);
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
            writeInDb(data);
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

bool VariableManager::setData(Data& data)
{
   data.m_undefinedValue = false;
   bool res = true;
   // écriture fichier
   string path = form("%s%s", m_dataPath.c_str(), data.m_dir.c_str());
   FILE* fp = fopen(path.c_str(), "wb");
   if(fp!=NULL)
   {
      fputs("{\n",fp);
      fputs(form("\"name\": \"%s\",\n", data.m_shortName.c_str()).c_str(),fp);
      fputs(form("\"date\": \"%s\",\n", data.m_date.c_str()).c_str(),fp);
      if(data.m_type==Data::DT_BOOL)
         fputs(form("\"value\": %s\n", dynamic_cast<TData<bool>&>(data).m_value?"true":"false").c_str(),fp);
      else if(data.m_type==Data::DT_FLOAT)
         fputs(form("\"value\": %.1f\n", dynamic_cast<TData<double>&>(data).m_value).c_str(),fp);
      fputs("}\n",fp);
      fclose(fp);
   }
   else
   {
      res=false;
   }
   // écriture en base
   res &= writeInDb(data);
   return res;
}

/**
 * écriture en base d'un changement de valeur
 */
bool VariableManager::writeInDb(Data& data)
{
   bool res = true;
   // écriture dans la table data_event
   sqlite3_reset(m_insertDataEventStatement);
   sqlite3_bind_int64(m_insertDataEventStatement, 1, data.m_dbId);
   sqlite3_bind_text (m_insertDataEventStatement, 2, data.m_date.c_str(), -1, SQLITE_STATIC);
   double value = 0;
   if(data.m_type==Data::DT_BOOL)
      value = dynamic_cast<TData<bool>&>(data).m_value;
   else if(data.m_type==Data::DT_FLOAT)
      value = dynamic_cast<TData<double>&>(data).m_value;
   sqlite3_bind_double(m_insertDataEventStatement, 3, value );

   int rc = sqlite3_step(m_insertDataEventStatement);
   if (rc != SQLITE_DONE)
   {
      printf("writeInDb() m_insertDataEventStatement ERROR SQLITE: %d %s\n", rc, sqlite3_errmsg(m_db));
      res = false;
   }
   sqlite3_stmt* getDbIdStatement = nullptr;
   rc = sqlite3_prepare_v2(m_db, "SELECT last_insert_rowid()", -1, &getDbIdStatement, NULL);
   if (rc != SQLITE_OK)
   {
      printf("writeInDb() getDbIdQuery ERROR SQLITE: %d %s\n", rc, sqlite3_errmsg(m_db));
   }
   else
   {
      if( SQLITE_ROW == sqlite3_step(getDbIdStatement) )
      {
         int64_t lastEventDbId = sqlite3_column_int64(getDbIdStatement, 0); // id_data_event
         sqlite3_reset(m_updateLastEventStatement);
         sqlite3_bind_int64(m_updateLastEventStatement, 1, lastEventDbId);
         sqlite3_bind_int64(m_updateLastEventStatement, 2, data.m_dbId);
         int rc = sqlite3_step(m_updateLastEventStatement);
         if (rc != SQLITE_DONE)
         {
            printf("writeInDb() m_updateLastEventStatement ERROR SQLITE: %d %s\n", rc, sqlite3_errmsg(m_db));
            res = false;
         }
      }
      sqlite3_finalize(getDbIdStatement);
   }

   return res;
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

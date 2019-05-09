/*
 * Database.cpp
 *
 *  Created on: 29 janv. 2019
 *      Author: Bertrand
 */

#include "Database.h"
#include "StringTools.h"
using namespace std;

Database::Database(const string& dataPath)
: m_dataPath(dataPath)
, m_db(nullptr)
, m_insertDataStatement(nullptr)
, m_insertDataEventStatement(nullptr)
, m_updateLastEventStatement(nullptr)
, m_selectAllDataStatement(nullptr)
{
}

Database::~Database()
{
   // TODO Auto-generated destructor stub
}


bool Database::initDb()
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
      rc = sqlite3_prepare_v3(m_db, selectDataSql, -1, 0, &m_selectAllDataStatement, nullptr);
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
void Database::init(const map<string, Data*>& datas)
{
   for(auto& pData: datas)
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


void Database::read(map<string, Data*>& datas)
{
   sqlite3_reset(m_selectAllDataStatement);
   while( SQLITE_ROW == sqlite3_step(m_selectAllDataStatement) )
   {
      int64_t dbId = sqlite3_column_int64(m_selectAllDataStatement, 0); // id_data
      string name = (const char*)sqlite3_column_text(m_selectAllDataStatement, 1); // name
      const char* date = (const char*)sqlite3_column_text(m_selectAllDataStatement, 2); // date
      double value     = sqlite3_column_double(m_selectAllDataStatement, 3); // value
      if(datas.find(name)!=datas.end())
      {
         Data& data = *datas[name];
         data.m_dbId = dbId;
         if (sqlite3_column_type(m_selectAllDataStatement, 2) != SQLITE_NULL)
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
}


/**
 * écriture en base d'un changement de valeur
 */
bool Database::write(Data& data)
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

void readHistoric(std::map<std::string, DataHistoric>& datas)
{

}

void readHistoric(const std::string& name, DataHistoric& data)
{

}

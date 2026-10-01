#include "aug_sqlite.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static void persistent_database(void) {
  char path[]="/tmp/aug-sqlite-XXXXXX";int file=mkstemp(path);assert(file>=0);close(file);
  aug_native_error_v1 error={0};void *database=NULL;int64_t changed=0;
  assert(aug_sqlite_open_v1((const unsigned char*)path,strlen(path),&database,&error)==0);
  const char *create="CREATE TABLE records (value TEXT NOT NULL)";
  assert(aug_sqlite_execute_v1(database,(const unsigned char*)create,strlen(create),NULL,NULL,0,&changed,&error)==0);
  const char *insert="INSERT INTO records (value) VALUES ('persisted')";
  assert(aug_sqlite_execute_v1(database,(const unsigned char*)insert,strlen(insert),NULL,NULL,0,&changed,&error)==0);
  aug_sqlite_release_v1(database);database=NULL;
  assert(aug_sqlite_open_v1((const unsigned char*)path,strlen(path),&database,&error)==0);
  const char *query="SELECT value FROM records";void *output=NULL;uint64_t length=0;
  assert(aug_sqlite_scalar_v1(database,(const unsigned char*)query,strlen(query),NULL,NULL,0,&output,&length,&error)==0);
  assert(length==9&&!memcmp(output,"persisted",9));aug_sqlite_text_release_v1(output);
  aug_sqlite_release_v1(database);assert(unlink(path)==0);assert(aug_sqlite_live_connections_v1()==0);
}
static void rejects_filesystem_expansion(void) {
  char path[]="/tmp/aug-sqlite-escape-XXXXXX";int file=mkstemp(path);assert(file>=0);close(file);assert(unlink(path)==0);
  void *database=NULL,*output=NULL;uint64_t length=0;int64_t changed=0;aug_native_error_v1 error={0};
  assert(aug_sqlite_open_v1(":memory:",8,&database,&error)==0);
  char sql[512];snprintf(sql,sizeof(sql),"ATTACH DATABASE '%s' AS extra",path);
  assert(aug_sqlite_scalar_v1(database,sql,strlen(sql),NULL,NULL,0,&output,&length,&error)!=0);
  assert(access(path,F_OK)!=0&&output==NULL);
  snprintf(sql,sizeof(sql),"VACUUM INTO '%s'",path);
  assert(aug_sqlite_execute_v1(database,sql,strlen(sql),NULL,NULL,0,&changed,&error)!=0);
  assert(access(path,F_OK)!=0);aug_sqlite_release_v1(database);
  assert(aug_sqlite_live_connections_v1()==0);
}
int main(void){
 rejects_filesystem_expansion();persistent_database();for(int i=0;i<1000;i++){
 void *db=NULL,*text=NULL;uint64_t length=0;int64_t changed=0;aug_native_error_v1 e={0};
 assert(aug_sqlite_open_v1(":memory:",8,&db,&e)==0&&db);
 const char *create="CREATE TABLE users (name TEXT NOT NULL)";
 assert(aug_sqlite_execute_v1(db,create,strlen(create),NULL,NULL,0,&changed,&e)==0);
 const char *insert="INSERT INTO users (name) VALUES (?)";const void *values[]={"O'Reilly"};uint64_t lengths[]={8};
 assert(aug_sqlite_execute_v1(db,insert,strlen(insert),values,lengths,1,&changed,&e)==0&&changed==1);
 const char *query="SELECT name FROM users";
 assert(aug_sqlite_scalar_v1(db,query,strlen(query),NULL,NULL,0,&text,&length,&e)==0&&length==8&&memcmp(text,"O'Reilly",8)==0);aug_sqlite_text_release_v1(text);
 const char *bad="SELECT name FROM users; DROP TABLE users";text=NULL;
 assert(aug_sqlite_scalar_v1(db,bad,strlen(bad),NULL,NULL,0,&text,&length,&e)!=0&&text==NULL);
 const char *write="DELETE FROM users";
 assert(aug_sqlite_scalar_v1(db,write,strlen(write),NULL,NULL,0,&text,&length,&e)!=0&&text==NULL);
 aug_sqlite_release_v1(db);assert(aug_sqlite_live_connections_v1()==0);
 }puts("SQLite: parameter binding, query, rejection and 1000 cleanup cycles passed");}

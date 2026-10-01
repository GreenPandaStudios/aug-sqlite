#include "aug_native.h"
#include "sqlite3.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdatomic.h>
static _Atomic int64_t live_connections;
static int32_t fail(aug_native_error_v1 *e,int code,const char *message){if(e){e->code=code;size_t n=strlen(message);if(n>512)n=512;e->message_length=(uint32_t)n;memcpy(e->message,message,n);}return code?code:SQLITE_ERROR;}
static void clear(aug_native_error_v1 *e){if(e)memset(e,0,sizeof(*e));}
/* A connection can access only the database explicitly opened by its owner.
   ATTACH is also used internally by VACUUM INTO; reject before execution.
   PRAGMA can mutate connection and filesystem policy while stmt_readonly is true. */
static const unsigned char scalar_policy=1;
static int authorize(void *context,int action,const char *a,const char *b,const char *database,const char *trigger){
 (void)a;(void)b;(void)database;(void)trigger;
 if(action==SQLITE_ATTACH||action==SQLITE_DETACH||action==SQLITE_PRAGMA)return SQLITE_DENY;
 if(context){switch(action){
   case SQLITE_SELECT:case SQLITE_READ:case SQLITE_FUNCTION:case SQLITE_RECURSIVE:return SQLITE_OK;
   default:return SQLITE_DENY;
 }}
 return SQLITE_OK;
}
AUG_EXPORT int32_t aug_sqlite_open_v1(const void *path,uint64_t n,void **out,aug_native_error_v1 *e){
 if(out)*out=NULL;clear(e);if(!out||(!path&&n)||n>INT_MAX||memchr(path,0,n))return fail(e,SQLITE_MISUSE,"Invalid database path");
 char *text=malloc((size_t)n+1);if(!text)return fail(e,SQLITE_NOMEM,"Out of memory");memcpy(text,path,n);text[n]=0;
 sqlite3 *db=NULL;int status=sqlite3_open_v2(text,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,NULL);free(text);
 if(status!=SQLITE_OK){int result=fail(e,status,db?sqlite3_errmsg(db):"Cannot open database");if(db)sqlite3_close(db);return result;}
 status=sqlite3_set_authorizer(db,authorize,NULL);
 if(status!=SQLITE_OK){int result=fail(e,status,sqlite3_errmsg(db));sqlite3_close(db);return result;}
 *out=db;atomic_fetch_add(&live_connections,1);return 0;
}
static int prepare(sqlite3 *db,const void *sql,uint64_t n,const void *const *parameters,const uint64_t *lengths,uint64_t count,int scalar,sqlite3_stmt **out,aug_native_error_v1 *e){
 *out=NULL;if(!db||(!sql&&n)||n==0||n>INT_MAX||count>INT_MAX||(count&&(!parameters||!lengths))||memchr(sql,0,n))return fail(e,SQLITE_MISUSE,"Invalid statement input");
 int policy=sqlite3_set_authorizer(db,authorize,scalar?(void *)&scalar_policy:NULL);
 if(policy!=SQLITE_OK)return fail(e,policy,sqlite3_errmsg(db));
 const char *tail=NULL;int status=sqlite3_prepare_v3(db,sql,(int)n,0,out,&tail);
 if(status!=SQLITE_OK||!*out)return fail(e,status?status:SQLITE_ERROR,sqlite3_errmsg(db));
 const char *end=(const char *)sql+n;while(tail<end&&(*tail==' '||*tail=='\t'||*tail=='\n'||*tail=='\r'))tail++;
 if(tail!=end)return fail(e,SQLITE_MISUSE,"Exactly one SQL statement is required");
 if(sqlite3_bind_parameter_count(*out)!=(int)count)return fail(e,SQLITE_RANGE,"Parameter count does not match the statement");
 for(uint64_t i=0;i<count;i++){if((!parameters[i]&&lengths[i])||lengths[i]>INT_MAX)return fail(e,SQLITE_TOOBIG,"Parameter is too large");
 status=sqlite3_bind_text(*out,(int)i+1,parameters[i]?(const char *)parameters[i]:"",(int)lengths[i],SQLITE_TRANSIENT);if(status!=SQLITE_OK)return fail(e,status,sqlite3_errmsg(db));}
 return 0;
}
AUG_EXPORT int32_t aug_sqlite_execute_v1(void *db,const void *sql,uint64_t n,const void *const *p,const uint64_t *sizes,uint64_t count,int64_t *out,aug_native_error_v1 *e){
 if(out)*out=0;clear(e);if(!out)return fail(e,SQLITE_MISUSE,"Null output");if(db)sqlite3_mutex_enter(sqlite3_db_mutex(db));sqlite3_stmt *stmt=NULL;int status=prepare(db,sql,n,p,sizes,count,0,&stmt,e);
 if(!status){if(!sqlite3_stmt_readonly(stmt)){int step=sqlite3_step(stmt);if(step!=SQLITE_DONE)status=fail(e,step,sqlite3_errmsg(db));else *out=sqlite3_changes64(db);}else status=fail(e,SQLITE_MISUSE,"Use queryScalar for read-only statements");}
 int finalized=sqlite3_finalize(stmt);if(!status&&finalized!=SQLITE_OK)status=fail(e,finalized,sqlite3_errmsg(db));if(db){sqlite3_set_authorizer(db,authorize,NULL);sqlite3_mutex_leave(sqlite3_db_mutex(db));}return status;
}
AUG_EXPORT int32_t aug_sqlite_scalar_v1(const void *connection,const void *sql,uint64_t n,const void *const *p,const uint64_t *sizes,uint64_t count,void **out,uint64_t *length,aug_native_error_v1 *e){
 if(out)*out=NULL;if(length)*length=0;clear(e);if(!out||!length)return fail(e,SQLITE_MISUSE,"Null output");sqlite3 *db=(sqlite3 *)connection;if(db)sqlite3_mutex_enter(sqlite3_db_mutex(db));sqlite3_stmt *stmt=NULL;int status=prepare(db,sql,n,p,sizes,count,1,&stmt,e);
 if(!status&&!sqlite3_stmt_readonly(stmt))status=fail(e,SQLITE_MISUSE,"queryScalar accepts read-only statements");
 if(!status){int step=sqlite3_step(stmt);if(step!=SQLITE_ROW||sqlite3_column_count(stmt)!=1||sqlite3_column_type(stmt,0)==SQLITE_NULL)status=fail(e,SQLITE_MISMATCH,"Expected one non-null scalar result");
 else{const unsigned char *text=sqlite3_column_text(stmt,0);int size=sqlite3_column_bytes(stmt,0);if(!text)status=fail(e,SQLITE_NOMEM,"Out of memory");else{*out=malloc(size?size:1);if(!*out)status=fail(e,SQLITE_NOMEM,"Out of memory");else{memcpy(*out,text,size);*length=(uint64_t)size;if(sqlite3_step(stmt)!=SQLITE_DONE)status=fail(e,SQLITE_MISMATCH,"Expected exactly one result row");}}}}
 int finalized=sqlite3_finalize(stmt);if(!status&&finalized!=SQLITE_OK)status=fail(e,finalized,sqlite3_errmsg(db));if(status){free(*out);*out=NULL;*length=0;}if(db){sqlite3_set_authorizer(db,authorize,NULL);sqlite3_mutex_leave(sqlite3_db_mutex(db));}return status;
}
AUG_EXPORT void aug_sqlite_text_release_v1(void *text){free(text);}
AUG_EXPORT void aug_sqlite_release_v1(void *db){if(db){int result=sqlite3_close(db);if(result!=SQLITE_OK)abort();atomic_fetch_sub(&live_connections,1);}}
AUG_EXPORT int64_t aug_sqlite_live_connections_v1(void){return atomic_load(&live_connections);}

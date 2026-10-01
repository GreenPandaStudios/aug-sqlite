// aug-spec: "api.aug.md" explains this file. Read it before changes; refresh with aug spec.
import Database from bindings
import SqliteError and DatabaseStorage from contracts
extern C _open(string path) returns own Database unless SqliteError
extern C _execute(borrow Database database, string sql, List<string> parameters) returns int unless SqliteError changes database
extern C _queryScalar(Database database, string sql, List<string> parameters) returns string unless SqliteError
/** Open a serialized connection. Use :memory: for an in-memory database. */
NativeDatabaseStorage() implements DatabaseStorage:
    open(string path) returns own Database:
        unsafe:
            return _open(path)
open(resolve DatabaseStorage storage, string path) returns own Database:
    return storage.open(path)
/** Open an in-memory database without filesystem permission. */
openMemory() returns own Database:
    unsafe:
        return _open(path=":memory:")
/** Execute one parameterized statement. Return the number of changed rows. */
execute(borrow Database database, string sql, List<string> parameters) returns int:
    unsafe:
        return _execute(database, sql, parameters)
/** Query one non-null text value with SELECT. Reject PRAGMAs, transactions, savepoints and writes before execution. Copy the result before finalization. */
queryScalar(Database database, string sql, List<string> parameters) returns string:
    unsafe:
        return _queryScalar(database, sql, parameters)

test openMemory:
    when databases:
        it binds_and_queries:
            own Database database = openMemory()
            borrow database:
                execute(database, sql="CREATE TABLE users (name TEXT NOT NULL)", parameters=[])
                execute(database, sql="INSERT INTO users (name) VALUES (?)", parameters=["O'Reilly"])
            assert(queryScalar(database, sql="SELECT name FROM users", parameters=[]) == "O'Reilly")
        it rejects_connection_and_transaction_control:
            own Database database = openMemory()
            for statement in ["PRAGMA query_only=ON", "BEGIN", "COMMIT", "ROLLBACK", "SAVEPOINT hidden"]:
                rejected = false
                try:
                    queryScalar(database, sql=statement, parameters=[])
                catch SqliteError error:
                    rejected = error.code == 23
                assert(rejected)
            borrow database:
                execute(database, sql="CREATE TABLE rows (value TEXT)", parameters=[])
                execute(database, sql="INSERT INTO rows VALUES (?)", parameters=["unchanged"])
            assert(queryScalar(database, sql="SELECT value FROM rows", parameters=[]) == "unchanged")

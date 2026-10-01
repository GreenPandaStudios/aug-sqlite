import Database and SqliteError and DatabaseStorage and NativeDatabaseStorage and open and execute and queryScalar from "https://github.com/GreenPandaStudios/aug-sqlite#v0.1.2"
implement DatabaseStorage with NativeDatabaseStorage
try:
    own Database database = open(path=":memory:")
    borrow database:
        execute(database, sql="CREATE TABLE users (name TEXT NOT NULL)", parameters=[])
        execute(database, sql="INSERT INTO users (name) VALUES (?)", parameters=["August"])
    print(value=queryScalar(database, sql="SELECT name FROM users", parameters=[]))
catch SqliteError error:
    print(value=error.message)

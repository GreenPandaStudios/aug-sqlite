// aug-spec: "contracts.aug.md" explains this file. Read it before changes; refresh with aug spec.
import Database from bindings
SqliteError(int code, string message) implements Error:
    pass
/** Permission to open database files. Select the native adapter in main.aug. */
capability DatabaseStorage:
    open(string path) returns own Database unless SqliteError uses DatabaseStorage.open

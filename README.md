# aug-sqlite

SQLite connections with labeled SQL parameters and scalar queries. File-backed access is selected through the DatabaseStorage capability; openMemory needs no filesystem grant.

This package targets the August `0.21.0` LLVM preview on macOS 14 or later, ARM64. Its source is ready for qualification; consumption requires the matching public compiler and native release assets. It does not work with August 0.20.1.

## Use it

After those preview assets are published:

```sh
aug init native-example
cd native-example
aug add https://github.com/GreenPandaStudios/aug-sqlite#v0.1.2 --as sqlite
```

Replace `main.aug` with:

```aug
import Database and SqliteError and DatabaseStorage and NativeDatabaseStorage and open and execute and queryScalar from sqlite
implement DatabaseStorage with NativeDatabaseStorage
try:
    own Database database = open(path=":memory:")
    borrow database:
        execute(database, sql="CREATE TABLE users (name TEXT NOT NULL)", parameters=[])
        execute(database, sql="INSERT INTO users (name) VALUES (?)", parameters=["August"])
    print(value=queryScalar(database, sql="SELECT name FROM users", parameters=[]))
catch SqliteError error:
    print(value=error.message)
```

Run `aug run`. Expected output:

```text
August
```

August selects LLVM for this native package. It verifies the source revision, binding contract, native archive, and compiler pack. Consumers need Node 24 and the supported OS. They do not install Git, Clang, LLVM, CMake, or Rust and do not execute this repository's native build recipe. Commit `aug.lock.json`; subsequent `aug run --offline --frozen` uses only verified cached selections. Keep a deployed executable with its adjacent `lib` and `share` directories.

## Contracts and maintenance

`src/export.aug` is the public surface. `src/*.aug.md` describes the checked August code. `native.abi.json` records native symbols, ownership, input representations, checked errors, and call-duration loans. `native/include` contains the C ABI, and `native/src` contains the actual upstream adapter. Acquisition returns owned handles; their scope releases them, including on errors. C++ exceptions and Rust panics do not cross the ABI. Native code remains a trust boundary.

Maintain binding declarations and the descriptor together. Run `aug check .`, `aug test .`, and `aug spec .` with the matching preview. Native maintainers additionally run `node native/build.mjs`; this explicit source build needs the toolchain in `aug-package.json` and the pinned inputs in `native/sources.lock.json`. Rust builds select Rust 1.98.1 explicitly. The build runs independent native clients before succeeding. The candidate workflow builds and uploads the measured archive and candidate manifest for review. Copy the reviewed manifest into source, check the August tests, then tag that source. Publish exactly the archive whose SHA-256 is in the tagged manifest; rebuilding creates a new candidate.

The prebuilt archive includes upstream notices, provenance, a runtime dependency inventory and a whole-file manifest. Installing this package does not run build scripts. An unsupported target or missing artifact is an error; there is no automatic source-build fallback.

Linux x86-64 and ARM64 candidates are built on Debian 12 with a glibc 2.36 floor. Their publication and installed-CLI qualification are tracked separately from the current macOS artifacts. See [the native maintainer workflow](native/LINUX.md). Do not use a candidate hash as a public download until its exact archive has been published.

Connections cannot attach other databases, run PRAGMAs, or execute `VACUUM`/`VACUUM INTO`. Temporary storage is memory-only. A file-backed connection may create SQLite journal, WAL and shared-memory sidecars beside its explicitly opened database. Extension loading is disabled. `queryScalar` requires exactly one non-null scalar row. Its preparation authorizer permits SELECT/read/function/recursive-query actions and rejects transactions, savepoints, and writes before execution. It cannot change connection settings through a rejected query. Native calls serialize each connection's authorization, preparation, execution and finalization.

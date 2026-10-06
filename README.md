# August SQLite

In-memory databases, parameterized SQL and scalar queries backed by SQLite.

**Unreleased 0.2.0 candidate.** This source requires August 1.0.0; compiler qualification and publication are pending. With the published August 0.23.0 compiler, use package v0.1.5.

Supported artifacts target macOS 14+ ARM64 and GNU/Linux x86-64 or ARM64 with glibc 2.36+. Consumers need Node 24+ and August. The CLI obtains prebuilt libraries and the compiler pack; no separate native compiler is required.

## Use it after publication

```sh
aug init native-example
cd native-example
aug add https://github.com/GreenPandaStudios/aug-sqlite#v0.2.0 --as sqlite
```

Save this as main.aug:

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

Run aug run. Expected output:

```text
August
```

Commit aug.lock.json. A frozen offline run uses the locked source and verified cached artifacts. Deploy the executable with its neighboring lib and share directories. An unsupported target or missing artifact stops installation; source builds require an explicit maintainer action.

## Ownership and failures

A Database is an owned native resource. execute requires mutable access to that database; queryScalar reads through a call-duration loan. SqliteError carries checked failures. Keep the database on its creating heap; scope exit releases it, including on failure.

## Maintain the package

Start at src/export.aug and the adjacent compiled specifications. native.abi.json declares symbols, input bounds, ownership, release functions and checked failures. Update native declarations and their descriptor together. Run aug check, aug test and aug spec with the required compiler.

A native build uses Clang and the Apple SDK on macOS, or the pinned Debian 12 maintainer image on Linux; its exact requirements and upstream inputs are in aug-package.json and native/sources.lock.json. Run node native/build.mjs, review the candidate archive and manifest, and publish the exact measured bytes. The archive contains dependency, license and provenance records. Installation never runs the recipe.

The 0.2.0 candidate preserves the previous native and August bindings and reuses their checksum-pinned archives. The compatibility publisher checks those inputs against the original release. A binding or adapter change requires new native qualification. Repeat real LLVM operations, cleanup and clean-consumer tests for the new compiler before publishing its support claim.

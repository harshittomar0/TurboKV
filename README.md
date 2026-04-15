# In-Memory Key-Value Store (C++)

A simple Redis-like key-value store built in C++.

## Features

* GET, SET, DEL
* TTL (key expiration)
* Sorted Sets (ZADD, ZREM, ZSCORE)
* TCP server (poll-based)
* CLI client

## Build

```bash
make
```

## Run

```bash
./kvstore
./client
```

## Example

```
set a 10
get a
del a
```

`The cache_bench is come from the rocksdb. And it's a good example for us to take a deep knowladge of the cache algorithm's implementation with high concurrent in lookup/insert/erase.`

# cache_bench
- lru cache
- clock cache
- leveldb lru cache

# Build
> The make file's lib is for mac, if you want to build the cache_bench ,it't better to change the dylib to .so.

- `make cache_bench`
  It's a tool to test the different cache with their options.
- `make cache_lib`
  Compile a cache-lib, and user could use the `lru` or `clock` cache easily. Just like the example of the example directroy.

# Performance
`./cache_bench`
```shell
Ops per thread      : 1200000
Cache size          : 8388608
Num shard bits      : 4
Max key             : 1073741824
Populate cache      : 0
Insert percentage   : 40%
Lookup percentage   : 50%
Erase percentage    : 10%
Test count          : 1
----------------------------
1 Test: complete in 4.448 s; QPS = 4316274
```


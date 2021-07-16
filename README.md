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
 $ ./cache_bench -threads=32 -lookup_percent=100 -erase_percent=0 -insert_percent=0 -num_shard_bits=8
Number of threads   : 32
Ops per thread      : 1200000
Cache size          : 8388608
Num shard bits      : 8
Max key             : 1073741824
Populate cache      : 0
Insert percentage   : 0%
Lookup percentage   : 100%
Erase percentage    : 0%
Test count          : 1
----------------------------
1 Test: complete in 0.767 s; QPS = 50052072

 $ ./cache_bench -threads=32 -lookup_percent=100 -erase_percent=0 -insert_percent=0 -num_shard_bits=8 -use_clock_cache=true
Number of threads   : 32
Ops per thread      : 1200000
Cache size          : 8388608
Num shard bits      : 8
Max key             : 1073741824
Populate cache      : 0
Insert percentage   : 0%
Lookup percentage   : 100%
Erase percentage    : 0%
Test count          : 1
----------------------------
1 Test: complete in 1.216 s; QPS = 31567213
```


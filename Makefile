BASH_EXISTS := $(shell which bash)
SHELL := $(shell which bash)

CFLAGS = $(shell which gcc)
CXXFLAGS = $(shell which g++)
INCLUDE = -Iinclude -Isrc/cache -Ithird-party/gflags/build/include \
		  -Ithird-party/oneTBB/include/oneapi \
		  -Ithird-party/oneTBB/include/ \
		  -Ithird-party/threadpool/include \

LIB = -std=c++11
# LIB = -std=c++11 -ltbb -lgflags
THIRD_PARTY_LIB = third-party/gflags/c_build/lib/libgflags.dylib \
	  third-party/oneTBB/build/appleclang_12.0_cxx11_64_release/libtbb.dylib \
		third-party/threadpool/libthreadpool.dylib
TARGET_LIB=libcache.dylib

SRC_SORCE = \
		src/cache/clock_cache.cc \
        src/cache/sharded_cache.cc \
        src/cache/lru_cache.cc \
        src/hash.cc \
        src/cache_bench.cc \
        src/port.cc \
        src/slice.cc \

cache_bench: $(SRC_SORCE)
	$(CXXFLAGS) $(INCLUDE) $(SRC_SORCE) -o cache_bench $(LIB) $(THIRD_PARTY_LIB)

cache_lib: $(SRC_SORCE)
	$(CXXFLAGS) -shared $(INCLUDE) $(SRC_SORCE) -o $(TARGET_LIB) $(LIB) $(THIRD_PARTY_LIB)

clean:
	rm -f cache_bench *.so *.dylib
	rm -rf third-party/threadpool/*.dylib
	rm -rf third-party/threadpool/*.a
	rm -rf third-party/threadpool/*.so

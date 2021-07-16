BASH_EXISTS := $(shell which bash)
SHELL := $(shell which bash)

CFLAGS = $(shell which gcc)
CXXFLAGS = $(shell which g++)
INCLUDE = -I../include -I../src/cache -I../third-party/gflags/build/include

LIB = -std=c++11
SRC_SORCE = ./clock_cache_test.cc
CACHE_LIB=../libcache.dylib ../third-party/gflags/c_build/lib/libgflags.dylib \
					../third-party/threadpool/libthreadpool.dylib

test: $(SRC_SORCE)
	$(CXXFLAGS) $(INCLUDE) $(SRC_SORCE) -o clock_cache_test $(LIB) $(CACHE_LIB) -g

clean:
	rm -f clock_cache_test

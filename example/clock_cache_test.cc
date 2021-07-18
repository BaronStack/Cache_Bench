//
// Created by zhanghuigui on 2021/7/16.
//

#include "cache.h"
#include "sharded_cache.h"
#include "gflags/gflags.h"

#include <iostream>
#include <memory>

DEFINE_int32(clock_cache_size, 1024, "cache size");
DEFINE_int32(cache_bit, 1, "cache shared bits ");
DEFINE_int32(key_nums, 5, "cache shared bits ");
DEFINE_bool(use_block_cache, true, "cache shared bits ");

using namespace std;

std::shared_ptr<Cache> cache = nullptr;
void CreateCache() {
	if (FLAGS_use_block_cache) {
		cache = NewClockCache(FLAGS_clock_cache_size, FLAGS_cache_bit);
	} else {
		cache = NewLRUCache(FLAGS_clock_cache_size, FLAGS_cache_bit);
	}
	if (!cache) {
		cout << "Create cache failed. " << endl;
		exit(-1);
	}
}

void deleter(const Slice& /*key*/, void* value) {
	delete reinterpret_cast<char *>(value);
}

void PrintCacheStats() {
	cache->PrintCacheInfo();
}

uint64_t RandNum(uint64_t key_range) {
	return uint64_t(rand()%(key_range-0) + 1);
}
void CacheInsert(std::string key, void *value) {
	uint64_t charge = key.size() + strlen(static_cast<char*>(value));
	cache->Insert(key, value, charge, &deleter);
  // cout << "Insert " << key << " success " << endl;
}

void CacheLookup(std::string key) {
	auto handle= cache->Lookup(key);
	if (handle) {
		// cout << "Lookup " << key << " success " << endl;
		cache->Release(handle);
	}
}

void CacheErase(std::string key) {
	cache->Erase(key);
	// cout << "Erase " << key << " success " << endl;
}

int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	CreateCache();
	cout << "Insert " << endl;
	for (uint64_t i = 1; i < FLAGS_key_nums; i ++) {
		CacheInsert("key" + std::to_string(RandNum(FLAGS_key_nums)), new char[10]);
	}
	PrintCacheStats();

	cout << "Lookup " << endl;
	for (uint64_t i = 1; i < FLAGS_key_nums; i ++) {
		CacheLookup("key" + std::to_string(RandNum(FLAGS_key_nums)));
	}
	PrintCacheStats();

	cout << "Erase " << endl;
	for (uint64_t i = 1; i < FLAGS_key_nums; i ++) {
		CacheErase("key" + std::to_string(RandNum(FLAGS_key_nums)));
	}
	PrintCacheStats();
}

//
// Created by zhanghuigui on 2021/7/12.
//

#pragma once
#include <stddef.h>
#include <stdint.h>

#include "slice.h"

#if defined(__x86_64__)
#define MURMUR_HASH MurmurHash64A
uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHash64A
typedef uint64_t murmur_t;

#elif defined(__i386__)
#define MURMUR_HASH MurmurHash2
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHash2
typedef unsigned int murmur_t;

#else
#define MURMUR_HASH MurmurHashNeutral2
unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed );
#define MurmurHash MurmurHashNeutral2
typedef unsigned int murmur_t;
#endif


#ifndef FALLTHROUGH_INTENDED
#if defined(__clang__)
#define FALLTHROUGH_INTENDED [[clang::fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH_INTENDED [[gnu::fallthrough]]
#else
#define FALLTHROUGH_INTENDED do {} while (0)
#endif
#endif

// Allow slice to be hashable by murmur hash.
struct murmur_hash {
	size_t operator()(const Slice& slice) const {
		return MurmurHash(slice.data(), static_cast<int>(slice.size()), 0);
	}
};

// Non-persistent hash. Only used for in-memory data structure
// The hash results are applicable to change.
extern uint64_t NPHash64(const char* data, size_t n, uint32_t seed);

extern uint32_t Hash(const char* data, size_t n, uint32_t seed);

inline uint32_t BloomHash(const Slice& key) {
	return Hash(key.data(), key.size(), 0xbc9f1d34);
}

inline uint64_t GetSliceNPHash64(const Slice& s) {
	return NPHash64(s.data(), s.size(), 0);
}

inline uint32_t GetSliceHash(const Slice& s) {
	return Hash(s.data(), s.size(), 397);
}

inline uint64_t NPHash64(const char* data, size_t n, uint32_t seed) {
	// Right now murmurhash2B is used. It should able to be freely
	// changed to a better hash, without worrying about backward
	// compatibility issue.
	return MURMUR_HASH(data, static_cast<int>(n),
										 static_cast<unsigned int>(seed));
}

// std::hash compatible interface.
struct SliceHasher {
	uint32_t operator()(const Slice& s) const { return GetSliceHash(s); }
};


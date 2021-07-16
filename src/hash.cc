//
// Created by zhanghuigui on 2021/7/12.
//

#include <string.h>
#include "hash.h"
#include "port.h"

inline uint32_t DecodeFixed32(const char* ptr) {
	if (port::kLittleEndian) {
		// Load the raw bytes
		uint32_t result;
		memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
		return result;
	} else {
		return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
						| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
						| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
						| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
	}
}

uint32_t Hash(const char* data, size_t n, uint32_t seed) {
	// Similar to murmur hash
	const uint32_t m = 0xc6a4a793;
	const uint32_t r = 24;
	const char* limit = data + n;
	uint32_t h = static_cast<uint32_t>(seed ^ (n * m));

	// Pick up four bytes at a time
	while (data + 4 <= limit) {
		uint32_t w = DecodeFixed32(data);
		data += 4;
		h += w;
		h *= m;
		h ^= (h >> 16);
	}

	// Pick up remaining bytes
	switch (limit - data) {
		// Note: The original hash implementation used data[i] << shift, which
		// promotes the char to int and then performs the shift. If the char is
		// negative, the shift is undefined behavior in C++. The hash algorithm is
		// part of the format definition, so we cannot change it; to obtain the same
		// behavior in a legal way we just cast to uint32_t, which will do
		// sign-extension. To guarantee compatibility with architectures where chars
		// are unsigned we first cast the char to int8_t.
		case 3:
			h += static_cast<uint32_t>(static_cast<int8_t>(data[2])) << 16;
			FALLTHROUGH_INTENDED;
		case 2:
			h += static_cast<uint32_t>(static_cast<int8_t>(data[1])) << 8;
			FALLTHROUGH_INTENDED;
		case 1:
			h += static_cast<uint32_t>(static_cast<int8_t>(data[0]));
			h *= m;
			h ^= (h >> r);
			break;
	}
	return h;
}

//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
/*
  Murmurhash from http://sites.google.com/site/murmurhash/

  All code is released to the public domain. For business purposes, Murmurhash
  is under the MIT license.
*/
#if defined(__x86_64__)

// -------------------------------------------------------------------
//
// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.
//
// 64-bit hash for 64-bit platforms

#ifdef ROCKSDB_UBSAN_RUN
#if defined(__clang__)
__attribute__((__no_sanitize__("alignment")))
#elif defined(__GNUC__)
__attribute__((__no_sanitize_undefined__))
#endif
#endif
uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed )
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end = data + (len/8);

	while(data != end)
	{
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7)
	{
		case 7: h ^= ((uint64_t)data2[6]) << 48; FALLTHROUGH_INTENDED;
		case 6: h ^= ((uint64_t)data2[5]) << 40; FALLTHROUGH_INTENDED;
		case 5: h ^= ((uint64_t)data2[4]) << 32; FALLTHROUGH_INTENDED;
		case 4: h ^= ((uint64_t)data2[3]) << 24; FALLTHROUGH_INTENDED;
		case 3: h ^= ((uint64_t)data2[2]) << 16; FALLTHROUGH_INTENDED;
		case 2: h ^= ((uint64_t)data2[1]) << 8;  FALLTHROUGH_INTENDED;
		case 1: h ^= ((uint64_t)data2[0]);
			h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

#elif defined(__i386__)

// -------------------------------------------------------------------
//
// Note - This code makes a few assumptions about how your machine behaves -
//
// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4
//
// And it has a few limitations -
//
// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch(len)
    {
    case 3: h ^= data[2] << 16; FALLTHROUGH_INTENDED;
    case 2: h ^= data[1] << 8;  FALLTHROUGH_INTENDED;
    case 1: h ^= data[0];
        h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#else

// -------------------------------------------------------------------
//
// Same as MurmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.

unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed )
{
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    unsigned int h = seed ^ len;

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k;

        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len)
    {
    case 3: h ^= data[2] << 16; FALLTHROUGH_INTENDED;
    case 2: h ^= data[1] << 8;  FALLTHROUGH_INTENDED;
    case 1: h ^= data[0];
        h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#endif


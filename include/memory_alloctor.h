//
// Created by zhanghuigui on 2021/7/7.
//
#pragma once

#include <memory>
#include <atomic>

// MemoryAllocator is an interface that a client can implement to supply custom
// memory allocation and deallocation methods. See rocksdb/cache.h for more
// information.
// All methods should be thread-safe.
class MemoryAllocator {
public:
	virtual ~MemoryAllocator() = default;

	// Name of the cache allocator, printed in the log
	virtual const char* Name() const = 0;

	// Allocate a block of at least size. Has to be thread-safe.
	virtual void* Allocate(size_t size) = 0;

	// Deallocate previously allocated block. Has to be thread-safe.
	virtual void Deallocate(void* p) = 0;

	// Returns the memory size of the block allocated at p. The default
	// implementation that just returns the original allocation_size is fine.
	virtual size_t UsableSize(void* /*p*/, size_t allocation_size) const {
		// default implementation just returns the allocation size
		return allocation_size;
	}
};

class CustomMemoryAllocator : public MemoryAllocator {
public:
	const char* Name() const override { return "CustomMemoryAllocator"; }

	void* Allocate(size_t size) override {
		++numAllocations;
		auto ptr = new char[size + 16];
		memcpy(ptr, "memory_allocator_", 16);  // mangle first 16 bytes
		return reinterpret_cast<void*>(ptr + 16);
	}
	void Deallocate(void* p) override {
		++numDeallocations;
		char* ptr = reinterpret_cast<char*>(p) - 16;
		delete[] ptr;
	}

	std::atomic<int> numAllocations;
	std::atomic<int> numDeallocations;
};
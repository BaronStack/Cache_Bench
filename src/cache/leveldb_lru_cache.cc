//
// Created by zhanghuigui on 2021/7/13.
//

#include "leveldb_lru_cache.h"

// LRU cache implementation
//
// Cache entries have an "in_cache" boolean indicating whether the cache has a
// reference on the entry.  The only ways that this can become false without the
// entry being passed to its "deleter" are via Erase(), via Insert() when
// an element with a duplicate key is inserted, or on destruction of the cache.
//
// The cache keeps two linked lists of items in the cache.  All items in the
// cache are in one list or the other, and never both.  Items still referenced
// by clients but erased from the cache are in neither list.  The lists are:
// - in-use:  contains the items currently referenced by clients, in no
//   particular order.  (This list is used for invariant checking.  If we
//   removed the check, elements that would otherwise be on this list could be
//   left as disconnected singleton lists.)
// - LRU:  contains the items not currently referenced by clients, in LRU order
// Elements are moved between these lists by the Ref() and Unref() methods,
// when they detect an element in the cache acquiring or losing its only
// external reference.

// An entry is a variable length heap-allocated structure.  Entries
// are kept in a circular doubly linked list ordered by access time.
struct LRUHandle {
	void* value;
	void (*deleter)(const Slice&, void* value);
	LRUHandle* next_hash;
	LRUHandle* next;
	LRUHandle* prev;
	size_t charge;  // TODO(opt): Only allow uint32_t?
	size_t key_length;
	bool in_cache;     // Whether entry is in the cache.
	uint32_t refs;     // References, including cache reference, if present.
	uint32_t hash;     // Hash of key(); used for fast sharding and comparisons
	char key_data[1];  // Beginning of key

	Slice key() const {
		// next_ is only equal to this if the LRU handle is the list head of an
		// empty list. List heads never have meaningful keys.
		assert(next != this);

		return Slice(key_data, key_length);
	}
};

// We provide our own simple hash table since it removes a whole bunch
// of porting hacks and is also faster than some of the built-in hash
// table implementations in some of the compiler/runtime combinations
// we have tested.  E.g., readrandom speeds up by ~5% over the g++
// 4.4.3's builtin hashtable.
class HandleTable {
public:
	HandleTable() : length_(0), elems_(0), list_(nullptr) { Resize(); }
	~HandleTable() { delete[] list_; }

	LRUHandle* Lookup(const Slice& key, uint32_t hash) {
		return *FindPointer(key, hash);
	}

	LRUHandle* Insert(LRUHandle* h) {
		LRUHandle** ptr = FindPointer(h->key(), h->hash);
		LRUHandle* old = *ptr;
		h->next_hash = (old == nullptr ? nullptr : old->next_hash);
		*ptr = h;
		if (old == nullptr) {
			++elems_;
			if (elems_ > length_) {
				// Since each cache entry is fairly large, we aim for a small
				// average linked list length (<= 1).
				Resize();
			}
		}
		return old;
	}

	LRUHandle* Remove(const Slice& key, uint32_t hash) {
		LRUHandle** ptr = FindPointer(key, hash);
		LRUHandle* result = *ptr;
		if (result != nullptr) {
			*ptr = result->next_hash;
			--elems_;
		}
		return result;
	}

private:
	// The table consists of an array of buckets where each bucket is
	// a linked list of cache entries that hash into the bucket.
	uint32_t length_;
	uint32_t elems_;
	LRUHandle** list_;

	// Return a pointer to slot that points to a cache entry that
	// matches key/hash.  If there is no such cache entry, return a
	// pointer to the trailing slot in the corresponding linked list.
	LRUHandle** FindPointer(const Slice& key, uint32_t hash) {
		LRUHandle** ptr = &list_[hash & (length_ - 1)];
		while (*ptr != nullptr && ((*ptr)->hash != hash || key.compare((*ptr)->key()) !=0 )) {
			ptr = &(*ptr)->next_hash;
		}
		return ptr;
	}

	void Resize() {
		uint32_t new_length = 4;
		while (new_length < elems_) {
			new_length *= 2;
		}
		LRUHandle** new_list = new LRUHandle*[new_length];
		memset(new_list, 0, sizeof(new_list[0]) * new_length);
		uint32_t count = 0;
		for (uint32_t i = 0; i < length_; i++) {
			LRUHandle* h = list_[i];
			while (h != nullptr) {
				LRUHandle* next = h->next_hash;
				uint32_t hash = h->hash;
				LRUHandle** ptr = &new_list[hash & (new_length - 1)];
				h->next_hash = *ptr;
				*ptr = h;
				h = next;
				count++;
			}
		}
		assert(elems_ == count);
		delete[] list_;
		list_ = new_list;
		length_ = new_length;
	}
};

// A single shard of sharded cache.
class LeveldbLRUCache final : CacheShard {
public:
	LeveldbLRUCache();
	~LeveldbLRUCache();

	// Separate from constructor so caller can easily make an array of LeveldbLRUCache
	void SetCapacity(size_t capacity) { capacity_ = capacity; }

	// Like Cache methods, but with an extra "hash" parameter.
	bool Insert(const Slice& key, uint32_t hash, void* value,
												size_t charge,
												void (*deleter)(const Slice& key, void* value),
												Cache::Handle** handle, Cache::Priority priority) override ;
	Cache::Handle* Lookup(const Slice& key, uint32_t hash);
	void Release(Cache::Handle* handle);
	void Erase(const Slice& key, uint32_t hash);
	bool Ref(LRUHandle* e);
	void Prune();
	size_t TotalCharge() const {
		MutexLock l(&mutex_);
		return usage_;
	}

private:
	void LRU_Remove(LRUHandle* e);
	void LRU_Append(LRUHandle* list, LRUHandle* e);
	void Unref(LRUHandle* e);
	bool FinishErase(LRUHandle* e);

	// Initialized before use.
	size_t capacity_;

	// mutex_ protects the following state.
	mutable port::Mutex mutex_;
	size_t usage_;

	// Dummy head of LRU list.
	// lru.prev is newest entry, lru.next is oldest entry.
	// Entries have refs==1 and in_cache==true.
	LRUHandle lru_;

	// Dummy head of in-use list.
	// Entries are in use by clients, and have refs >= 2 and in_cache==true.
	LRUHandle in_use_;

	HandleTable table_;
};

LeveldbLRUCache::LeveldbLRUCache() : capacity_(0), usage_(0) {
	// Make empty circular linked lists.
	lru_.next = &lru_;
	lru_.prev = &lru_;
	in_use_.next = &in_use_;
	in_use_.prev = &in_use_;
}

LeveldbLRUCache::~LeveldbLRUCache() {
	assert(in_use_.next == &in_use_);  // Error if caller has an unreleased handle
	for (LRUHandle* e = lru_.next; e != &lru_;) {
		LRUHandle* next = e->next;
		assert(e->in_cache);
		e->in_cache = false;
		assert(e->refs == 1);  // Invariant of lru_ list.
		Unref(e);
		e = next;
	}
}

bool LeveldbLRUCache::Ref(LRUHandle* e) {
	if (e->refs == 1 && e->in_cache) {  // If on lru_ list, move to in_use_ list.
		LRU_Remove(e);
		LRU_Append(&in_use_, e);
	}
	e->refs++;
	return true;
}

void LeveldbLRUCache::Unref(LRUHandle* e) {
	assert(e->refs > 0);
	e->refs--;
	if (e->refs == 0) {  // Deallocate.
		assert(!e->in_cache);
		(*e->deleter)(e->key(), e->value);
		free(e);
	} else if (e->in_cache && e->refs == 1) {
		// No longer in use; move to lru_ list.
		LRU_Remove(e);
		LRU_Append(&lru_, e);
	}
}

void LeveldbLRUCache::LRU_Remove(LRUHandle* e) {
	e->next->prev = e->prev;
	e->prev->next = e->next;
}

void LeveldbLRUCache::LRU_Append(LRUHandle* list, LRUHandle* e) {
	// Make "e" newest entry by inserting just before *list
	e->next = list;
	e->prev = list->prev;
	e->prev->next = e;
	e->next->prev = e;
}

Cache::Handle* LeveldbLRUCache::Lookup(const Slice& key, uint32_t hash) {
	MutexLock l(&mutex_);
	LRUHandle* e = table_.Lookup(key, hash);
	if (e != nullptr) {
		Ref(e);
	}
	return reinterpret_cast<Cache::Handle*>(e);
}

void LeveldbLRUCache::Release(Cache::Handle* handle) {
	MutexLock l(&mutex_);
	Unref(reinterpret_cast<LRUHandle*>(handle));
}

bool LeveldbLRUCache::Insert(const Slice& key, uint32_t hash, void* value,
																size_t charge,
																void (*deleter)(const Slice& key,
																								void* value),
																Cache::Handle** handle,
																Cache::Priority /*priority*/) {
	MutexLock l(&mutex_);

	LRUHandle* e =
	reinterpret_cast<LRUHandle*>(malloc(sizeof(LRUHandle) - 1 + key.size()));
	e->value = value;
	e->deleter = deleter;
	e->charge = charge;
	e->key_length = key.size();
	e->hash = hash;
	e->in_cache = false;
	e->refs = 1;  // for the returned handle.
	memcpy(e->key_data, key.data(), key.size());

	if (capacity_ > 0) {
		e->refs++;  // for the cache's reference.
		e->in_cache = true;
		LRU_Append(&in_use_, e);
		usage_ += charge;
		FinishErase(table_.Insert(e));
	} else {  // don't cache. (capacity_==0 is supported and turns off caching.)
		// next is read by key() in an assert, so it must be initialized
		e->next = nullptr;
	}
	while (usage_ > capacity_ && lru_.next != &lru_) {
		LRUHandle* old = lru_.next;
		assert(old->refs == 1);
		bool erased = FinishErase(table_.Remove(old->key(), old->hash));
		if (!erased) {  // to avoid unused variable when compiled NDEBUG
			assert(erased);
		}
	}

	*handle = reinterpret_cast<Cache::Handle*>(e);
	return true;
}

// If e != nullptr, finish removing *e from the cache; it has already been
// removed from the hash table.  Return whether e != nullptr.
bool LeveldbLRUCache::FinishErase(LRUHandle* e) {
	MutexLock l(&mutex_);
	if (e != nullptr) {
		assert(e->in_cache);
		LRU_Remove(e);
		e->in_cache = false;
		usage_ -= e->charge;
		Unref(e);
	}
	return e != nullptr;
}

void LeveldbLRUCache::Erase(const Slice& key, uint32_t hash) {
	MutexLock l(&mutex_);
	FinishErase(table_.Remove(key, hash));
}

void LeveldbLRUCache::Prune() {
	MutexLock l(&mutex_);
	while (lru_.next != &lru_) {
		LRUHandle* e = lru_.next;
		assert(e->refs == 1);
		bool erased = FinishErase(table_.Remove(e->key(), e->hash));
		if (!erased) {  // to avoid unused variable when compiled NDEBUG
			assert(erased);
		}
	}
}

// class LevelDBLRU final : public ShardedCache {
// public:
// 	LevelDBLRU(size_t capacity, int num_shard_bits, bool strict_capacity_limit)
// 	: ShardedCache(capacity, num_shard_bits, strict_capacity_limit) {
// 		int num_shards = 1 << num_shard_bits;
// 		shards_ = new LeveldbLRUCache[num_shards];
// 		SetCapacity(capacity);
// 		SetStrictCapacityLimit(strict_capacity_limit);
// 	}
//
// 	~LevelDBLRU() override { delete[] shards_; }
//
// 	const char* Name() const override { return "LevelDBLRUCache"; }
//
// 	CacheShard* GetShard(int shard) override {
// 		return reinterpret_cast<CacheShard*>(&shards_[shard]);
// 	}
//
// 	const CacheShard* GetShard(int shard) const override {
// 		return reinterpret_cast<CacheShard*>(&shards_[shard]);
// 	}
//
// 	void* Value(Handle* handle) override {
// 		return reinterpret_cast<const LRUHandle*>(handle)->value;
// 	}
//
// 	size_t GetCharge(Handle* handle) const override {
// 		return reinterpret_cast<const LRUHandle*>(handle)->charge;
// 	}
//
// 	uint32_t GetHash(Handle* handle) const override {
// 		return reinterpret_cast<const LRUHandle*>(handle)->hash;
// 	}
//
// 	void DisownData() override { shards_ = nullptr; }
//
// private:
// 	LeveldbLRUCache* shards_;
// };


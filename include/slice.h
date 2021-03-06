//
// Created by zhanghuigui on 2021/7/7.
//
#pragma once

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <cstdio>
#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

class Slice {
public:
	// Create an empty slice.
	Slice() : data_(""), size_(0) {}

	// Create a slice that refers to d[0,n-1].
	Slice(const char* d, size_t n) : data_(d), size_(n) {}

	// Create a slice that refers to the contents of "s"
	/* implicit */
	Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

#ifdef __cpp_lib_string_view
	// Create a slice that refers to the same contents as "sv"
/* implicit */
Slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {}
#endif

	// Create a slice that refers to s[0,strlen(s)-1]
	/* implicit */
	Slice(const char* s) : data_(s) { size_ = (s == nullptr) ? 0 : strlen(s); }

	// Return a pointer to the beginning of the referenced data
	const char* data() const { return data_; }

	// Return the length (in bytes) of the referenced data
	size_t size() const { return size_; }

	// Return true iff the length of the referenced data is zero
	bool empty() const { return size_ == 0; }

	// Return the ith byte in the referenced data.
	// REQUIRES: n < size()
	char operator[](size_t n) const {
		assert(n < size());
		return data_[n];
	}

	// Change this slice to refer to an empty array
	void clear() {
		data_ = "";
		size_ = 0;
	}

	// Drop the first "n" bytes from this slice.
	void remove_prefix(size_t n) {
		assert(n <= size());
		data_ += n;
		size_ -= n;
	}

	void remove_suffix(size_t n) {
		assert(n <= size());
		size_ -= n;
	}

	// Return a string that contains the copy of the referenced data.
	// when hex is true, returns a string of twice the length hex encoded (0-9A-F)
	std::string ToString(bool hex = false) const;

#ifdef __cpp_lib_string_view
	// Return a string_view that references the same data as this slice.
std::string_view ToStringView() const {
  return std::string_view(data_, size_);
}
#endif

	// Decodes the current slice interpreted as an hexadecimal string into result,
	// if successful returns true, if this isn't a valid hex string
	// (e.g not coming from Slice::ToString(true)) DecodeHex returns false.
	// This slice is expected to have an even number of 0-9A-F characters
	// also accepts lowercase (a-f)
	bool DecodeHex(std::string* result) const;

	// Three-way comparison.  Returns value:
	//   <  0 iff "*this" <  "b",
	//   == 0 iff "*this" == "b",
	//   >  0 iff "*this" >  "b"
	int compare(const Slice& b) const;

	// Return true iff "x" is a prefix of "*this"
	bool starts_with(const Slice& x) const {
		return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
	}

	bool ends_with(const Slice& x) const {
		return ((size_ >= x.size_) &&
						(memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
	}

	// Compare two slices and returns the first byte where they differ
	size_t difference_offset(const Slice& b) const;

	// private: make these public for rocksdbjni access
	const char* data_;
	size_t size_;

	// Intentionally copyable
};

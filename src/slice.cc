//
// Created by zhanghuigui on 2021/7/7.
//

#include "slice.h"
// 2 small internal utility functions, for efficient hex conversions
// and no need for snprintf, toupper etc...
// Originally from wdt/util/EncryptionUtils.cpp - for ToString(true)/DecodeHex:
char toHex(unsigned char v) {
	if (v <= 9) {
		return '0' + v;
	}
	return 'A' + v - 10;
}
// most of the code is for validation/error check
int fromHex(char c) {
	// toupper:
	if (c >= 'a' && c <= 'f') {
		c -= ('a' - 'A');  // aka 0x20
	}
	// validation
	if (c < '0' || (c > '9' && (c < 'A' || c > 'F'))) {
		return -1;  // invalid not 0-9A-F hex char
	}
	if (c <= '9') {
		return c - '0';
	}
	return c - 'A' + 10;
}

// Return a string that contains the copy of the referenced data.
std::string Slice::ToString(bool hex) const {
	std::string result;  // RVO/NRVO/move
	if (hex) {
		result.reserve(2 * size_);
		for (size_t i = 0; i < size_; ++i) {
			unsigned char c = data_[i];
			result.push_back(toHex(c >> 4));
			result.push_back(toHex(c & 0xf));
		}
		return result;
	} else {
		result.assign(data_, size_);
		return result;
	}
}

// Originally from rocksdb/utilities/ldb_cmd.h
bool Slice::DecodeHex(std::string* result) const {
	std::string::size_type len = size_;
	if (len % 2) {
		// Hex string must be even number of hex digits to get complete bytes back
		return false;
	}
	if (!result) {
		return false;
	}
	result->clear();
	result->reserve(len / 2);

	for (size_t i = 0; i < len;) {
		int h1 = fromHex(data_[i++]);
		if (h1 < 0) {
			return false;
		}
		int h2 = fromHex(data_[i++]);
		if (h2 < 0) {
			return false;
		}
		result->push_back(static_cast<char>((h1 << 4) | h2));
	}
	return true;
}

inline bool operator==(const Slice& x, const Slice& y) {
	return ((x.size() == y.size()) &&
					(memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) { return !(x == y); }

int Slice::compare(const Slice& b) const {
	assert(data_ != nullptr && b.data_ != nullptr);
	const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
	int r = memcmp(data_, b.data_, min_len);
	if (r == 0) {
		if (size_ < b.size_)
			r = -1;
		else if (size_ > b.size_)
			r = +1;
	}
	return r;
}

inline size_t Slice::difference_offset(const Slice& b) const {
	size_t off = 0;
	const size_t len = (size_ < b.size_) ? size_ : b.size_;
	for (; off < len; off++) {
		if (data_[off] != b.data_[off]) break;
	}
	return off;
}
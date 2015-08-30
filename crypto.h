
#pragma once

#include "types.h"

class digest : comparable<digest>
{
public:
	// Make a 'zero' digest
	digest();
	// Make a hash from a string
	digest(const string& str);
	// Makes a hash of the concatenation of two hashes
	digest(const digest& d1, const digest& d2);
	// Compare hashes
	bool operator<(const digest& rhs) const;
	bool operator==(const digest& rhs) const;
	// Find the first bit that differs between two hashed
	uint32_t first_diff(const digest& rhs) const;
	// Returns 0 or 1 based on the bit at position 'which'
	uint32_t get_bit(uint32_t which) const;
	// Pretty print
	string as_string() const;
private:
	uint8_t m_digest[32];
};


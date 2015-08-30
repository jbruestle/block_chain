
#include "crypto.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <openssl/sha.h>

digest::digest()
{
	memset(m_digest, 0, 32);
}

digest::digest(const string& str)
{
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.data(), str.size());
    SHA256_Final(m_digest, &sha256);
}

digest::digest(const digest& d1, const digest& d2)
{
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, d1.m_digest, 32);
    SHA256_Update(&sha256, d2.m_digest, 32);
    SHA256_Final(m_digest, &sha256);
}

bool digest::operator<(const digest& rhs) const
{
	return memcmp(m_digest, rhs.m_digest, 32) < 0;
}

bool digest::operator==(const digest& rhs) const
{
	return memcmp(m_digest, rhs.m_digest, 32) == 0;
}

uint32_t digest::first_diff(const digest& rhs) const
{
	// First find the byte that differs
	for(uint32_t i = 0; i < 32; i++) {
		if (m_digest[i] != rhs.m_digest[i]) {
			// Find the first bit that differs
			uint8_t x = m_digest[i];
			uint8_t y = rhs.m_digest[i];
			for(int32_t j = 0; j < 8; j++) {
				uint32_t mask = ~((1 << (7-j)) - 1);
				if ((x & mask) != (y & mask)) {
					return (i*8) + j;
				}
			}	
			// If the byte differed, no bits differing is impossible
			assert(false);
		}
	}
	// Looks like a perfect match
	return 32*8;
}

uint32_t digest::get_bit(uint32_t i) const
{
	assert(i < 32*8);
	return (m_digest[i/8] & (1 << (7 - (i % 8)))) ? 1 : 0;
}

static char hex_digits[] = "0123456789ABCDEF";

string digest::as_string() const
{
	string s;
	for(size_t i = 0; i < 4; i++) {	
		s += hex_digits[m_digest[i] / 16];
		s += hex_digits[m_digest[i] % 16];
	}
	return s;
}


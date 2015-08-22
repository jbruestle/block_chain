
#include "merkle_cow.h"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <openssl/sha.h>
#include <arpa/inet.h>

static void hash_kvp(hash_t& out, const merkle_cow::key_type& key, const merkle_cow::mapped_type& value)
{
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	uint32_t klen = htonl(uint32_t(key->size()));
	SHA256_Update(&ctx, (const char *) &klen, sizeof(uint32_t));
	SHA256_Update(&ctx, key->data(), key->size());
	SHA256_Update(&ctx, value->data(), value->size());
	SHA256_Final((unsigned char*) out.data(), &ctx);
}

merkle_cow::policy::value_t merkle_cow::policy::compute_total(const value_t* vals, size_t count)
{
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	for(size_t i = 0; i < count; i++) {
		SHA256_Update(&ctx, (const char *) vals[i].second.data(), vals[i].second.size()); 
	}
	value_t r;
	SHA256_Final((unsigned char*) r.second.data(), &ctx);
	return r;
}

bool merkle_cow::policy::less(const key_t& a, const key_t& b)
{
	return *a < *b;
}


merkle_cow::mapped_type merkle_cow::put(const key_type& key, const mapped_type& value) {
	bool new_exists;
	bnode_t::value_t new_val;
	if (value) {
		new_exists = true;
		new_val.first = value;
		hash_kvp(new_val.second, key, value);
	} else {
		new_exists = false;
	}
	mapped_type r;
	m_tree.update(key, [&](bnode_t::value_t& val, bool& exists) -> bool {
		if (exists) {
			r = val.first;
		}
		if (!exists && !new_exists) { return false; }	
		if (exists && new_exists && *val.first == *new_val.first) { return false; }
		exists = new_exists;
		if (new_exists) {
			val = new_val;
		}
		return true;
	});
	return r;
}

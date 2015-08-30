
#pragma once

#include "btree.h"
#include "biter.h"
#include <boost/iterator/iterator_facade.hpp>

typedef array<char, 32> hash_t;

// Don't support mutable iterators because proxies annoy me
class merkle_cow
{
private:
	struct policy {
		static const size_t min_size = 8;
		static const size_t max_size = 16;
		typedef shared_ptr<string> string_ptr_t;
		typedef string_ptr_t key_t;
		typedef pair<string_ptr_t, hash_t> value_t;
		static value_t compute_total(const value_t* vals, size_t count);
		static bool less(const key_t& a, const key_t& b);
		static void serialize(writable& out, const key_t& a, const value_t& b);
        };
	typedef btree<policy> btree_t;
	typedef bnode<policy> bnode_t;
	typedef biter<policy> biter_t;
public:
	// Map like typedefs, add as needed
	typedef shared_ptr<string> key_type;
	typedef shared_ptr<string> mapped_type;
        typedef pair<key_type, mapped_type> value_type;

	// I only support const_iterator, and they keep the tree forever
	// This is technically not to spec, but I don't care
	class const_iterator : public boost::iterator_facade<
                const_iterator,
		const value_type,
                boost::bidirectional_traversal_tag>
        {
                friend class boost::iterator_core_access;
		friend class merkle_cow;
	public:
		const_iterator() {}
		void increment() { m_iter.increment(); update(); }
                void decrement() { m_iter.decrement(); update(); }
		bool equal(const_iterator const& rhs) const { return m_iter == rhs.m_iter; }
		const value_type& dereference() const { return m_pair; }
	private:
		const_iterator(const btree_t& tree)
			: m_iter(tree.root(), tree.height())
		{}

		void update() {
			if (m_iter.is_end()) {
				m_pair = value_type();
			} else {
				m_pair.first = m_iter.get_key();
				m_pair.second = m_iter.get_value().first;
			}
		}
	
		biter_t m_iter;
		value_type m_pair;	
	};
	
	// Read only iterator style access
	const_iterator begin() const { 
		const_iterator it(m_tree); it.m_iter.set_begin(); it.update(); return it; 
	}
	const_iterator find(const key_type& key) const { 
		const_iterator it(m_tree); it.m_iter.set_find(key); it.update(); return it; 
	}
	const_iterator lower_bound(const key_type& key) const { 
		const_iterator it(m_tree); it.m_iter.set_lower_bound(key); it.update(); return it; 
	}
	const_iterator upper_bound(const key_type& key) const { 
		const_iterator it(m_tree); it.m_iter.set_upper_bound(key); it.update(); return it; 
	}
	const_iterator end() const { const_iterator it(m_tree); return it; }

	// Set key to value, overwrite as needed, return previous value
	// Value of emptry string represents 'no-value'
	mapped_type put(const key_type& key, const mapped_type& value);

	// Get value, empty string means not found
	string get(const key_type& key) const;
	
private:
	btree_t m_tree;
};

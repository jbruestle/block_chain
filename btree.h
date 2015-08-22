
#pragma once

#include "bnode.h"

template<class Policy>
class btree
{
	typedef bnode<Policy> node_t;
	typedef typename node_t::ptr_t ptr_t;
	typedef typename node_t::wptr_t wptr_t;
	typedef typename node_t::key_t key_t;
	typedef typename node_t::value_t value_t;
public:
	// Make an empty tree
	btree() 
		: m_height(0)
		, m_size(0)
	{}
		
	template<class Updater>
	bool update(const key_t& k, const Updater& updater)
	{
		// If root is null, see if an insert works
		if (m_height == 0) {
			value_t v;
			bool exists = false;
			// Try the insert
			bool changed = updater(v, exists);
			// If nothing changed, return false
			if (!changed || !exists)
				return false;
			// Otherwise, create the initial node
			m_root = std::make_shared<node_t>(k, v);
			m_height++;
			m_size++;
			return true;
		}

		// Let's try running the update
		wptr_t w_root = m_root->copy();
		wptr_t overflow;
		ptr_t peer;
		auto r = w_root->update(k, peer, overflow, updater, m_height - 1);

		if (r == node_t::ur_nop) {
			// If nothing happend, return false
			return false;
		}
		else if (r == node_t::ur_modify) {
			m_root = w_root;
		} else if (r == node_t::ur_insert) {
			m_size++;
			m_root = w_root;
		} else if (r == node_t::ur_erase) {
			m_size--;
			m_root = w_root;
		}
		else if (r == node_t::ur_split)
		{
			// Root just split, make new root
			m_root = std::make_shared<node_t>(w_root, overflow);
			m_height++;  
			m_size++;
		} 
		else if (r == node_t::ur_singular)
		{
			// If the node is down to one element get it's inner bit and throw it away
			m_root = w_root->ptr(0);
			m_height--;
			m_size--;
		}
		else if (r == node_t::ur_empty)
		{
			// Tree is totally empty
			m_root = ptr_t();
			m_height = 0;
			m_size = 0;
		}
		return true;    
	}

	size_t size() const { return m_size; }
	size_t height() const { return m_height; }
	ptr_t root() const { return m_root; }

	void serialize(writable& out) {
		// TODO: Serialize size
		uint8_t height = uint8_t(m_height);
		out.write((const char *) &height, 1);
		if (height) {
			m_root->serialize(out, m_height - 1);
		}
	}

private:
	ptr_t m_root;
	size_t m_height;
	size_t m_size;
};


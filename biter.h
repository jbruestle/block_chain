
#pragma once

#include "bnode.h"

template<class Policy>
class biter
{
public:
	typedef typename Policy::key_t key_t;
	typedef typename Policy::value_t value_t;
	typedef typename bnode<Policy>::ptr_t ptr_t;

	// Construct a totally empty iterator
	biter()
		: m_height(0)
	{}

	// Construct an empty iterator to a tree snapshot
	biter(const ptr_t& root, size_t height) 
		: m_height(height)
		, m_nodes(height)
		, m_iters(height)
	{
		if (m_height)
		{
			assert(root != ptr_t());
			m_nodes[0] = root;
			m_iters[0] = root->size();
		}
	}

	bool operator==(const biter& other) const
	{
		// Ignore tree of different size
		if (m_height != other.m_height)
			return false;

		// Empty trees are all the same
		if (m_height == 0)
			return true;

		// Iterators from different trees differ
		if (m_nodes[0] != other.m_nodes[0])
			return false;

		// Handle special cases for 'end'
		if (is_end())
			return other.is_end();
		else if (other.is_end())
			return false;
		
		// Now compare the iterators in depth
		return m_iters == other.m_iters;
	}

	// Set this iterator to the tree begin
	void set_begin()
	{
		if (m_height == 0) return;
		for(size_t i = 0; i + 1 < m_height; i++)
		{
			m_iters[i] = 0;
			m_nodes[i+1] = m_nodes[i]->ptr(m_iters[i]);	
		}
		m_iters[m_height - 1] = 0;
	}

	void set_rbegin()
	{
		if (m_height == 0) return;
		for(size_t i = 0; i + 1 < m_height; i++)
		{
			m_iters[i] = m_nodes[i]->size() - 1;
			m_nodes[i+1] = m_nodes[i]->ptr(m_iters[i]);	
		}
		m_iters[m_height - 1] = m_nodes[m_height - 1]->size() - 1;
	}

	// Set this iterator to the tree end
	void set_end()
	{
		// Handle empty case
		if (m_height == 0)
			return;
		// Set the top of the stack to end
		m_iters[0] = m_nodes[0]->size();
	}

	void set_find(const key_t& k)
	{
		if (m_height == 0)
			return;
		set_lower_bound(k);
		if (is_end()) return;
		if (Policy::less(k, get_key()))
			set_end();
	}

	void set_lower_bound(const key_t& k)
	{
		if (m_height == 0)
			return;
		if (!Policy::less(m_nodes[0]->key(0), k))
		{
			set_begin();
			return;
		}
		for(size_t i = 0; i + 1 < m_height;i++)
		{
			m_iters[i] = m_nodes[i]->lower_bound(k) - 1;
			m_nodes[i+1] = m_nodes[i]->ptr(m_iters[i]);
		}
		m_iters[m_height - 1] = m_nodes[m_height-1]->lower_bound(k);
		m_iters[m_height - 1]--;
		increment();
	}

	void set_upper_bound(const key_t& k)
	{
		if (m_height == 0)
			return;
		if (Policy::less(k, m_nodes[0]->key(0)))
		{
			set_begin();
			return;
		}
		for(size_t i = 0; i + 1 < m_height;i++)
		{
			m_iters[i] = m_nodes[i]->upper_bound(k) - 1;
			m_nodes[i+1] = m_nodes[i]->ptr(m_iters[i]);
		}
		m_iters[m_height - 1] = m_nodes[m_height-1]->upper_bound(k);
		m_iters[m_height - 1]--;
		increment();
	}

	void increment()
	{
		// Make sure we are not at end
		assert(!is_end());
	
		// Move the lowest level forward one	
		int cur = m_height - 1;
		m_iters[cur]++;
		// If we hit the end at our current level, move up and increment
		while(m_iters[cur] == m_nodes[cur]->size())
		{
			cur--;
			if (cur < 0)
				return;  // If we hit the top, we are done
			m_iters[cur]++;
		}

		cur++;
		while(cur < (int) m_height)
		{
			m_nodes[cur] = m_nodes[cur -1]->ptr(m_iters[cur-1]);	
			m_iters[cur] = 0;
			cur++;
		}
	}

	void decrement()
	{
		// Handle special 'end' case
		if (is_end())
		{
			set_rbegin();
			return;
		}
	
		// Find the lowest level that can move back one
		int cur = m_height - 1;
		while(m_iters[cur] == 0)
		{
			cur--;
			if (cur < 0) 
				return;  // begin()-- is undefined, make it a no-op
		}
		// Move back
		m_iters[cur]--;
		// Set things to 'rbegin' the rest of the way down
		cur++;
		while(cur < (int) m_height)
		{
			m_nodes[cur] = m_nodes[cur-1]->ptr(m_iters[cur-1]);	
			m_iters[cur] = m_nodes[cur]->size();
			m_iters[cur]--;
			cur++;
		}
	}

	bool is_end() const { return m_height == 0 || m_iters[0] == m_nodes[0]->size(); }
	const key_t& get_key() const { assert(!is_end()); return m_nodes[m_height-1]->key(m_iters[m_height-1]); }
	const value_t& get_value() const { assert(!is_end()); return m_nodes[m_height-1]->val(m_iters[m_height-1]); }
	ptr_t get_root() const { ptr_t r;  if (m_height != 0) r = m_nodes[0]; return r; }
	size_t get_height() const { return m_height; }

private:
	// The height of the tree we are attached to
	size_t m_height;  
	// The nodes in the tree
	// nodes[0] = root
	// nodes[i+1] = nodes[i]->ptr(iters[i])
	vector<ptr_t> m_nodes;
	// The iterators within each node
	vector<size_t> m_iters; 
};



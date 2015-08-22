
#pragma once

#include <cstddef>
#include <memory>
#include <algorithm>

class writable
{
public:
	virtual void write(const char* buf, size_t len) = 0;
};

class readable 
{
public:
	virtual void read(char* buf, size_t len) = 0;
};

template<class Policy>
class bnode 
{
public:
	const static size_t min_size = Policy::min_size;
	const static size_t max_size = Policy::max_size;
	typedef typename Policy::key_t key_t;
	typedef typename Policy::value_t value_t;
	typedef typename std::shared_ptr<const bnode> ptr_t;
	typedef typename std::shared_ptr<bnode> wptr_t;

	// Create a new 'tree' with one element
	bnode(const key_t& k, const value_t& v)
		: m_size(1)
	{
		recompute_total();
		m_keys[0] = k;
		m_vals[0] = v;
	}

	// Make a new root node based on two nodes (which must be identical height)
	bnode(const ptr_t& n1, const ptr_t& n2)
		: m_size(2)
	{
		assign(0, n1);
		assign(1, n2);
		recompute_total();
	}
	
	void serialize(writable& out, size_t height) const
	{
		uint8_t size = uint8_t(m_size);
		out.write((const char*) &size, 1);
		for(size_t i = 0; i < m_size; i++)
		{
			if (height == 0) {
				Policy::serialize(out, m_keys[i], m_vals[i]);
				continue;
			}
			m_ptrs[i]->serialize(out, height - 1);
		}
	}

	void deserialize(readable& in, size_t height)
	{
		uint8_t size;
		in.read((char*) &size, 1);
		for(size_t i = 0; i < m_size; i++)
		{
			if (height == 0) {
				Policy::deserialize(in, m_keys[i], m_vals[i]);
				continue;
			}
			wptr_t ptr = std::make_shared<bnode>(m_size);
			ptr->deserialize(in, height - 1);
			m_keys[i] = ptr->m_keys[0];
			m_vals[i] = ptr->m_total;
			m_ptrs[i] = ptr;
		}
		recompute_total();
	}

	// Return a writable version of this node
	wptr_t copy() const
	{
		// Make a copy of a node	
		wptr_t copy = std::make_shared<bnode>(m_size);
		copy->m_total = m_total;
		for(size_t i = 0; i < m_size; i++)
		{
			copy->m_keys[i] = m_keys[i];
			copy->m_vals[i] = m_vals[i];
			copy->m_ptrs[i] = m_ptrs[i];
		}

		// Return the new node
		return copy;
	}

	// What happened during the update
	enum update_result 
	{
		ur_nop,  // Didn't make any changes
		ur_modify, // Just altered the entry
		ur_insert, // Inserted an entry, but no need for split
		ur_erase, // Erased an entry, but no effect on peer
		ur_split,  // Had to split the node
		ur_steal, // Erased and stole and entry from peer
		ur_merge, // Erased and merged with peer
		ur_singular, // Erased, had no peer, and down to a single down pointer
		ur_empty, // Node is both empty and has height of 0
	};

	template<class Updater> 
	update_result update(const key_t& k, ptr_t& peer, wptr_t& split, const Updater& updater, int height)
	{
		if (height == 0)
		{
			// This node is a leaf
			// Declare variables for updater
			bool exists = true;
			value_t v;

			// Try to find existing entry
			size_t i = find(k);
			if (i == m_size)
				exists = false;
			else
				v = m_vals[i];

			// Run the updater
			bool did_exist = exists;
			bool changed = updater(v, exists);

			// Return if no actual change
			if (changed == false || (!did_exist && !exists))
				return ur_nop;

			if (did_exist && !exists)
			{
				// Erase case, remove element
				erase(i);
				// Run fixup
				return erase_fixup(peer, height);
			}
			else if (!did_exist && exists)
			{
				// Insert case, do insert
				insert(k, v, ptr_t()); 
				// Maybe do a split
				split = maybe_split(); 
				return split ? ur_split : ur_insert;
			}
			else 
			{
				// Modify case
				m_vals[i] = v; 
				recompute_total();
				return ur_modify;
			}
		}
		// Find the child node the update goes to
		size_t i = find_by_key(k);  
		// Find a peer, try next node over
		size_t pi = (i == m_size-1 ? i - 1 : i+1);

		// Prepare node and it's peer for modification
		wptr_t new_node = m_ptrs[i]->copy();
		wptr_t overflow;

		// Run the recursive update 
		update_result r = new_node->update(k, m_ptrs[pi], overflow, updater, height - 1);
		if (r == ur_nop)
		{
			// Nothing happened, undo everything
			return ur_nop;
		}
		if (r == ur_modify || r == ur_erase || r == ur_insert)
		{
			// Easy case, keep new node, peer is untouched
			assign(i, new_node);
			recompute_total();  // Recompute self
			return r;  // Send status up
		}
		if (r == ur_split)
		{
			// We had a split of the lower node
			// First keep new_node
			assign(i, new_node);
			// Then add overflow node
			insert(overflow);
			// Check for yet another split
			split = maybe_split();
			return split ? ur_split : ur_insert;
		}
		// We modified peer, update info
		m_keys[pi] = m_ptrs[pi]->m_keys[0];
		m_vals[pi] = m_ptrs[pi]->m_total;
		if (r == ur_steal)
		{
			// Keep new node
			assign(i, new_node);
			recompute_total();  // Recompute self
			return ur_erase;  // Send up status
		}
		// r == ur_merge
		// Keep only peer (main down ptr is deleted)
		erase(i);  // Delete erased down ptr
		return erase_fixup(peer, height);  // Do an erase fixup
	}

public:
	size_t size() const { return m_size; }
	const key_t& key(size_t i) const { return m_keys[i]; }
	const value_t& val(size_t i) const { return m_vals[i]; }
	const value_t& total() const { return m_total; }
	const ptr_t& ptr(size_t i) const { return m_ptrs[i]; } 

	size_t lower_bound(const key_t& k) const 
	{ return std::lower_bound(m_keys, m_keys + m_size, k, 
		[this](const key_t& a, const key_t& b) -> bool { return Policy::less(a, b); }) - m_keys; }
	size_t upper_bound(const key_t& k) const 
	{ return std::upper_bound(m_keys, m_keys + m_size, k,
		[this](const key_t& a, const key_t& b) -> bool { return Policy::less(a, b); }) - m_keys; }
	size_t find(const key_t& k) const
	{
		size_t i = lower_bound(k);
		if (i != m_size && !Policy::less(k, m_keys[i])) return i;
		return m_size;
	}

	// Constructor for an empty bnode
	// Used during deserialization
	bnode(size_t size) 
		: m_size(size)
	{}

private:
	// Find the entry for a key
	size_t find_by_key(const key_t& k)
	{
		size_t i = upper_bound(k);
		if (i != 0) i--;
		return i;
	}

	void assign(size_t i, const ptr_t& newval) { 
		m_keys[i] = newval->m_keys[0];
		m_vals[i] = newval->m_total;
		m_ptrs[i] = newval;
	}

	void copy_entry(size_t i, size_t j)
	{
		m_keys[i] = m_keys[j];
		m_vals[i] = m_vals[j];
		m_ptrs[i] = m_ptrs[j];
	}

	void insert(const key_t& k, const value_t& v, const ptr_t& down)
	{
		int loc = (int) lower_bound(k);
		for(int i = m_size; i > loc; i--)
			copy_entry(i, i-1);
		m_keys[loc] = k;
		m_vals[loc] = v;
		m_ptrs[loc] = down;
		m_size++;
	}
	
	void insert(const ptr_t& down)
	{
		insert(down->m_keys[0], down->m_total, down);
	}

	void erase(size_t begin, size_t end)
	{
		int diff = end - begin;
		for(int i = begin; i + diff < (int) m_size; i++)
			copy_entry(i, i+diff);
		for(int i = m_size - diff; i < (int) m_size; i++)
		{
			m_keys[i] = key_t();
			m_vals[i] = value_t();
			m_ptrs[i] = ptr_t();
		}
			
		m_size -= diff;
	}
	void erase(size_t loc) { erase(loc, loc+1); }

	void recompute_total() 
	{
		m_total = Policy::compute_total(&m_vals[0], m_size);
	}

	wptr_t maybe_split()
	{
		// If the node isn't too big, and fix up total
		if (m_size <= max_size)
		{
			recompute_total();
			return NULL;
		}

		// Compute the size of half (rounded dVown) to keep
		int keep_size = m_size / 2;

		// Create a new bnode with the same height as me
		wptr_t r = std::make_shared<bnode>(m_size - keep_size);

		// Copy second of the entries into the new node
		for(size_t i = 0; i < m_size - keep_size; i++)
		{
			r->m_keys[i] = m_keys[i + keep_size];
			r->m_vals[i] = m_vals[i + keep_size];
			r->m_ptrs[i] = m_ptrs[i + keep_size];
		}
		// Erase them from me
		for(size_t i = keep_size; i < m_size; i++)
		{
			m_keys[i] = key_t();
			m_vals[i] = value_t();
			m_ptrs[i] = ptr_t();
		}
	
		m_size = keep_size;

		recompute_total();
		r->recompute_total();
		// Return new node
		return r;
	}

	update_result erase_fixup(ptr_t& peer_ptr, int height)
	{
		// If we still have a valid number of nodes, we're done
		if (m_size >= min_size)
		{
			// Still need to fix total
			recompute_total();
			// Return easy case
			return ur_erase; 
		}	
		// Handle the special root case
		if (peer_ptr == ptr_t())
		{
			// We should only ever run out of entries as the last
			// erase of the entire tree.
			if (m_size == 0)
				return ur_empty; 
			// No more recursion, compute totals
			recompute_total();
			// Figure out which enum to return
			if (height != 0 && size() == 1)
				return ur_singular; // Down to 1 entry at top of tree
			return ur_erase;
		}
		// We are going to modify peer, let's copy first
		wptr_t peer = peer_ptr->copy();
		// Now we try to steal from peer
		if (peer->m_size > min_size)
		{
			// Get the appropriate peer entry
			size_t pi = (Policy::less(peer->m_keys[0], m_keys[0])) 
					? peer->size() - 1 // Last of peer before me
					: 0 // First of peer after me
					;

			// 'Move' the entry over
			insert(peer->m_keys[pi], peer->m_vals[pi], peer->m_ptrs[pi]);
			peer->erase(pi);
			// Recompute self and peer's totals
			recompute_total();
			peer->recompute_total();
			// Set output
			peer_ptr = peer;
			// Return my state
			return ur_steal;
		}
		// Looks like we need to merge with peer
		// Add my entries into it, and recompute total
		// TODO: Make this not slow!
		for(size_t i = 0; i < m_size; i++)
			peer->insert(m_keys[i], m_vals[i], m_ptrs[i]);
		// Fix peers total
		peer->recompute_total();
		// Set output
		peer_ptr = peer;
		// Return the fact that I merged
		return ur_merge;
	}

	value_t m_total;  // Total of all down entries, cached
	size_t m_size;
	key_t m_keys[max_size + 1];  // All my keys
	value_t m_vals[max_size + 1];  // All my values
	ptr_t m_ptrs[max_size + 1];  // All my pointers
};


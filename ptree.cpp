
#include "ptree.h"

const digest k_empty;

ptree_branch::ptree_branch(uint32_t split_pos, const ptree_ptr& p1, const ptree_ptr& p2)
	: m_branches{ 
		p1->prefix() < p2->prefix() ? p1 : p2, 
		p1->prefix() < p2->prefix() ? p2 : p1 }
	, m_prefix(m_branches[0]->prefix())
	, m_split_pos(split_pos)
	, m_merkle(m_branches[0]->merkle(), m_branches[1]->merkle())
{}

const digest& ptree_branch::get(const digest& key) const
{
	// See if I match prefix
	uint32_t match_len = m_prefix.first_diff(key);
	if (match_len < m_split_pos) {
		return k_empty;  // Missed
	}
	// Recurse down proper branch
	return m_branches[key.get_bit(m_split_pos)]->get(key);
}

ptree_ptr ptree_branch::set(const digest& key, const digest& value) const
{
	// See if I match prefix
	uint32_t match_len = m_prefix.first_diff(key);
	if (match_len < m_split_pos) {
		// If it's an erase, not found, return unchanged
		if (value == k_empty) {
			return shared_from_this();
		}

		// Looks like a need to split further
		return make_shared<ptree_branch>(
			match_len,
			shared_from_this(), 
			make_shared<ptree_leaf>(key, value));
	}

	// Determine which way to go
	uint32_t dir = key.get_bit(m_split_pos);

	// Recurse on down
	ptree_ptr new_branch = m_branches[dir]->set(key, value);

	// If no change, return self
	if (new_branch.get() == m_branches[dir].get()) {
		return shared_from_this();
	}

	// If null, must have been a delete, return other branch
	if (!new_branch) {
		return m_branches[1-dir];
	}

	// Return new copy
	return make_shared<ptree_branch>(m_split_pos, m_branches[1-dir], new_branch);
}

ptree_leaf::ptree_leaf(const digest& key, const digest& value) 
	: m_key(key)
	, m_value(value)
	, m_merkle(key, value)
{}
 
const digest& ptree_leaf::get(const digest& key) const
{
	if (key != m_key) {
		return k_empty; // Missed, return empty
	}
	return m_value;
}

ptree_ptr ptree_leaf::set(const digest& key, const digest& value) const
{
	if (key == m_key) {
		// Found the node in question
		if (value == k_empty) {
			// It's an erase, return null
			return ptree_ptr();
		}
		// Return a newly created node
		return make_shared<ptree_leaf>(key, value);
	}
	// No match, if erase, return self
	if (value == k_empty) {
		return shared_from_this();
	}
	// Otherwise, split and return 
	uint32_t match_len = m_key.first_diff(key);
	return make_shared<ptree_branch>(
		match_len,
		shared_from_this(), 
		make_shared<ptree_leaf>(key, value));
}

const digest& ptree::merkle() const
{
	if (!m_root) {
		return k_empty;
	}
	return m_root->merkle();
}

const digest& ptree::get(const digest& key) const
{
	if (!m_root) {
		return k_empty;
	}
	return m_root->get(key);
}

void ptree::set(const digest& key, const digest& value)
{
	if (!m_root) {
		if (value != k_empty) {
			m_root = make_shared<ptree_leaf>(key, value);
		}
		return;
	}
	m_root = m_root->set(key, value);
}


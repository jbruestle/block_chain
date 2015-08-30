
#pragma once 

#include "types.h"
#include "crypto.h"

class ptree_node;
typedef shared_ptr<const ptree_node> ptree_ptr;
extern const digest k_empty;

class ptree_node : public enable_shared_from_this<ptree_node>
{
public:
	virtual ~ptree_node() {}
	virtual const digest& prefix() const = 0;
	virtual const digest& merkle() const = 0;
	virtual const digest& get(const digest& key) const = 0;
	virtual ptree_ptr set(const digest& key, const digest& value) const = 0;
};

class ptree_branch : public ptree_node
{
public:
	ptree_branch(uint32_t split_pos, const ptree_ptr& p1, const ptree_ptr& p2);
	const digest& prefix() const { return m_prefix; }
	const digest& merkle() const { return m_merkle; }
	const digest& get(const digest& key) const;
	ptree_ptr set(const digest& key, const digest& value) const;

private:
	ptree_ptr m_branches[2];
	uint32_t  m_split_pos;
	digest    m_prefix;
	digest    m_merkle;
};

class ptree_leaf : public ptree_node 
{
public:
	ptree_leaf(const digest& key, const digest& value);
	const digest& prefix() const { return m_key; }
	const digest& merkle() const { return m_merkle; }
	const digest& get(const digest& key) const;
	ptree_ptr set(const digest& key, const digest& value) const;

private:
	digest m_key;  
	digest m_value; 
	digest m_merkle;
};

class ptree 
{
public:
	// Constructors, etc, are default
	const digest& merkle() const;
	const digest& get(const digest& key) const;
	void set(const digest& key, const digest& value);
	
private:
	ptree_ptr m_root;
};


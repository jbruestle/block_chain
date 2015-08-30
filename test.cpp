
#include "btree.h"
#include "utils.h"
#include "merkle_cow.h"

typedef array<char, 32> hash_t;
shared_ptr<string> to_shared(const string& str) {
	return make_shared<string>(str);
}

class string_writer : public writable
{
public:
	void write(const char* str, size_t len) {
		m_value += string(str, len);
	}
	string value() { return m_value; }
private:
	string m_value;
};

int main() 
{
	merkle_cow mc;
	for(size_t i = 0; i < 100; i++) {
		mc.put(to_shared(to_string(i)), to_shared(to_string(i)));
	}
	for(size_t i = 0; i < 100; i+=2) {
		mc.put(to_shared(to_string(i)), shared_ptr<string>());
	}
	for(auto it : mc) {
		printf("%s->%s\n", it.first->c_str(), it.second->c_str());
	}
	string_writer sw;
	//cm.serialize(sw);
	//printf("%s", hexdump(sw.value()).c_str());
}


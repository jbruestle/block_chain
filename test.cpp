
#include "btree.h"
#include <array>
#include <string>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "utils.h"
#include "merkle_cow.h"

typedef std::array<char, 32> hash_t;
std::shared_ptr<std::string> to_shared(const std::string& str) {
	return std::make_shared<std::string>(str);
}

class string_writer : public writable
{
public:
	void write(const char* str, size_t len) {
		m_value += std::string(str, len);
	}
	std::string value() { return m_value; }
private:
	std::string m_value;
};

int main() 
{
	merkle_cow mc;
	for(size_t i = 0; i < 100; i++) {
		mc.put(to_shared(std::to_string(i)), to_shared(std::to_string(i)));
	}
	for(size_t i = 0; i < 100; i+=2) {
		mc.put(to_shared(std::to_string(i)), std::shared_ptr<std::string>());
	}
	for(auto it : mc) {
		printf("%s->%s\n", it.first->c_str(), it.second->c_str());
	}
	string_writer sw;
	cm.serialize(sw);
	//printf("%s", hexdump(sw.value()).c_str());
}


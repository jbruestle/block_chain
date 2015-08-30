
#include "types.h"
#include <sstream>
#include <iomanip>

string printstring(const char* format, ...)
{
        va_list vl;
        va_start(vl, format);
        
        char* buf;
        int len = vasprintf(&buf, format, vl);
        string r(buf, len);
        free(buf);
        va_end(vl);

        return r;
}

static const char* digits = "0123456789ABCDEF";

string cleanify(const string& input) {
	string out = input;
	for(char& c : out) {
		if (c < 32 || c > 127) {
			c = '.';
		}
	}
	return out;
}

string hexify(const string& input) {
	string out;
	for(uint8_t c : input) {
		out += digits[c >> 4];
		out += digits[c & 0xf];
	}
	return out;
}

// TODO: Where did I get this hexdump?
string hexdump(const string& input)
{
	 std::istringstream is(input);
	 std::stringstream ss;
	 size_t address = 0;

	 ss << std::hex << std::setfill('0');
	 while (is.good()) {
		  int nread;
		  char buf[16];

		  for (nread = 0; nread < 16 && is.get(buf[nread]); nread++);
		  if (nread == 0) {
			   break;
		  }

		  // Show the address
		  ss << std::setw(8) << address;

		  // Show the hex codes
		  for (int i = 0; i < 16; i++) {
			   if (i % 8 == 0) {
				    ss << ' ';
			   }
			   if (i < nread) {
				    ss << ' ' << std::setw(2) << ((unsigned)buf[i] & 0xff);
			   }
			   else {
				    ss << "   ";
			   }
		  }

		  // Show printable characters
		  ss << "  ";
		  for (int i = 0; i < nread; i++) {
			   if (buf[i] < 32) {
				    ss << '.';
			   }
			   else {
				    ss << buf[i];
			   }
		  }

		  ss << "\n";
		  address += 16;
	 }
	 return ss.str();
}



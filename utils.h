

// Helper to 'printf' to strings
string printstring(const char* format, ...);
// Makes a posibly binary string safe for logging
string cleanify(const string& input);
// Does inline hex dumps
string hexify(const string& input);
// Does long form hex dumps
string hexdump(const string& input);


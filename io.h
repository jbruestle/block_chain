
#pragma once

#include "types.h"

// Exception class for io abstraction
class io_exception : public runtime_error
{
public:
        io_exception(const string& message) : runtime_error(message) {}
};

// Something you can read from
class readable
{
public:
	virtual ~readable() {}

	// Always reads until len or EOF, returns total read, throws on err 
	virtual size_t read(char* buf, size_t len) = 0;
};

/* A helper class to wrap objects with unix read semantics (-1 on err, not full reads, etc) */
class read_wrapper : public readable
{
public:
	read_wrapper();
	~read_wrapper();
	size_t read(char* buf, size_t len);

private:
	virtual int base_read(char* buf, int len) = 0;
};

class writable
{
public:
	virtual ~writable() {}

	// Always does a full write, throws on errors, may do buffering prior to close
	virtual void write(const char* buf, size_t len) = 0;

	// Flush data before returning
	virtual void flush() {}

	// Since you can write, it must make sense to 'finish' somehow, throws on error
	virtual void close() {} 
};

/* A helper class to wrap objects with unix write semantics */
class write_wrapper : public writable
{
public:
	void write(const char* buf, size_t len);
	void flush();
	void close();

private:
	virtual int base_write(const char *buf, int len) = 0;
	virtual int base_close() { return 0; }
	virtual int base_flush() { return 0; }
};


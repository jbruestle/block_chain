
#include "io.h"

const static size_t k_bufsize = 16 * 1024;

size_t read_wrapper::read(char* buf, size_t len)
{
	size_t tot_read = 0;
	while(len)
	{
		int r = base_read(buf, len);
		if (r < 0)
			throw io_exception("IO error on read");
		if (r == 0)
			return tot_read;
		tot_read += r;
		buf += r;
		len -= r;
	}

	return tot_read;
}

void write_wrapper::write(const char* buf, size_t len)
{
	while(len > 0)
	{
		int r = base_write(buf, len);
		if (r <= 0)
			throw io_exception("IO error during write");
		len -= r;
		buf += r;
	}
}

void write_wrapper::flush()
{
	if (base_flush() < 0)
		throw io_exception("Error during flush");
}

void write_wrapper::close()
{
	if (base_close() < 0)
		throw io_exception("Error during close");
}


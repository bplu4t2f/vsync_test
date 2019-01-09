#include "sformat.h"


static thread_local char sformat_buf[4096];

static bool get_int(const char *str, int *value, const char **after);
static bool follows_closing_brace(const char *str, const char **after);

String sformat_core(const char *format, int num_args, IFormattable **args)
{
	const char *from = format;
	char * const to = sformat_buf;
	const int to_size = sizeof(sformat_buf) / sizeof(sformat_buf[0]);
	int to_cursor = 0;

	for (;; ++from)
	{
		char c = *from;
		if (c == 0 || to_cursor >= to_size - 1) {
			to[to_cursor] = 0;
			break;
		}
		if (c != '{') {
			goto __normal;
		}

		if (from[1] == '{') {
			from += 1;
			goto __normal;
		}
		int arg_index;
		const char *after;
		if (!get_int(from + 1, &arg_index, &after)) {
			goto __normal;
		}
		if (!follows_closing_brace(after, &after)) {
			goto __normal;
		}
		if (arg_index >= num_args) {
			goto __normal;
		}
		// Here: It's actually valid
		from = after;
		int remaining = to_size - (to_cursor);
		to_cursor += args[arg_index]->format(to + to_cursor, remaining);
		continue;

	__normal:
		to[to_cursor++] = c;
	}

	return String(sformat_buf, to_cursor);
}

static bool get_int(const char *str, int *value, const char **after)
{
	while (*str == ' ' || *str == '\t') {
		++str;
	}
	if (*str >= '0' && *str <= '9')
	{
		int tmp = *str - '0';
		str += 1;
		while (*str >= '0' && *str <= '9') {
			tmp *= 10;
			tmp += *str - '0';
		}
		*value = tmp;
		*after = str;
		return true;
	}
	else
	{
		return false;
	}
}

static bool follows_closing_brace(const char *str, const char **after)
{
	while (*str == ' ' || *str == '\t') {
		++str;
	}
	if (*str == '}') {
		*after = str;
		return true;
	}
	else {
		return false;
	}
}






#include <stdio.h>

int sappend(int value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%d", value);
}

int sappend(double value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%lf", value);
}

int sappend(float value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%f", value);
}

int sappend(unsigned int value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%u", value);
}

int sappend(long value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%ld", value);
}

int sappend(unsigned long value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%lu", value);
}

int sappend(long long value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%lld", value);
}

int sappend(unsigned long long value, char *buffer, int buffer_size)
{
	return sprintf_s(buffer, buffer_size, "%llu", value);
}

int sappend(char value, char *buffer, int buffer_size)
{
	if (buffer_size > 0) {
		*buffer = value;
		return 1;
	} else {
		return 0;
	}
}

int sappend(const char *value, char *buffer, int buffer_size)
{
	int i = 0;
	for (; value[i] != 0 && i < buffer_size; ++i)
	{
		buffer[i] = value[i];
	}
	return i;
}

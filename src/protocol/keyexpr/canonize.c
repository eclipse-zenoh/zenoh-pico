#include "stdbool.h"
#include "string.h"
#include "assert.h"

typedef enum _z_keyexpr_canon_status_t
{
	_Z_SUCCESS,						  // the key expression is canon
	_Z_EMPTY_CHUNK,					  // the key contains empty chunks
	_Z_LONE_DOLLAR_STAR,			  // the key contains a `$*` chunk, which must be replaced by `*`
	_Z_STARS_IN_CHUNK,				  // the key contains a `*` in a chunk without being escaped by a DSL, which is forbidden
	_Z_SINGLE_STAR_AFTER_DOUBLE_STAR, // the key contains `**/*`, which must be replaced by `*/**`
	_Z_DOUBLE_STAR_AFTER_DOUBLE_STAR, // the key contains `**/**`, which must be replaced by `**`
	_Z_DOLLAR_AFTER_DOLLAR_OR_STAR,	  // the key contains `$*$` or `$$`, which is forbidden
	_Z_CONTAINS_SHARP_OR_QMARK,		  // the key contains `#` or `?`, which is forbidden
	_Z_CONTAINS_UNBOUND_DOLLAR		  // the key contains a `$` which isn't bound to a DSL
} _z_keyexpr_canon_status_t;

_z_keyexpr_canon_status_t _zp_canon_prefix(const char *start, size_t *len)
{
	bool in_big_wild = false;
	char const *chunk_start = start;
	const char *end = start + *len;
	char const *chunk_end;
	do
	{
		chunk_end = memchr(chunk_start, '/', end - chunk_start);
		size_t chunk_len = (chunk_end ? chunk_end : end) - chunk_start;
		const char *end = chunk_start + chunk_len;
		switch (chunk_len)
		{
		case 0:
			return _Z_EMPTY_CHUNK;
		case 1:
			if (in_big_wild && *chunk_start == '*')
			{
				*len = chunk_start - start - 3;
				return _Z_SINGLE_STAR_AFTER_DOUBLE_STAR;
			}
			chunk_start = end + 1;
			continue;
		case 2:
			if (chunk_start[1] == '*')
			{
				switch (*chunk_start)
				{
				case '$':
					*len = chunk_start - start;
					return _Z_LONE_DOLLAR_STAR;
				case '*':
					if (in_big_wild)
					{
						*len = chunk_start - start - 3;
						return _Z_DOUBLE_STAR_AFTER_DOUBLE_STAR;
					}
					else
					{
						chunk_start = end + 1;
						in_big_wild = true;
						continue;
					}
				}
			}
			break;
		}
		unsigned char in_dollar = 0;
		for (char const *c = chunk_start; c < end; c++)
		{
			switch (*c)
			{
			case '#':
			case '?':
				return _Z_CONTAINS_SHARP_OR_QMARK;
			case '$':
				if (in_dollar)
				{
					return _Z_DOLLAR_AFTER_DOLLAR_OR_STAR;
				}
				in_dollar += 1;
				break;
			case '*':
				if (in_dollar != 1)
				{
					return _Z_STARS_IN_CHUNK;
				}
				in_dollar += 2;
				break;
			default:
				if (in_dollar == 1)
				{
					return _Z_CONTAINS_UNBOUND_DOLLAR;
				}
				in_dollar = 0;
			}
		}
		if (in_dollar == 1)
		{
			return _Z_CONTAINS_UNBOUND_DOLLAR;
		}
		chunk_start = end + 1;
		in_big_wild = false;
	} while (chunk_start < end);
	return _Z_SUCCESS;
}

_z_keyexpr_canon_status_t _zp_is_canon_ke(const char *start, size_t len)
{
	return _zp_canon_prefix(start, &len);
}

void _zp_write(char *to, size_t *to_len, const char *from, size_t len)
{
	memmove(to, from, len);
	*to_len += len;
	*to += len;
}
size_t _zp_starts_with(const char *s, const char *needle)
{
	size_t i = 0;
	for (; needle[i]; i++)
	{
		if (s[i] != needle[i])
		{
			return 0;
		}
	}
	return i;
}
void _zp_singleify(char *start, size_t *len, const char *needle)
{
	const char *end = start + *len;
	bool right_after_needle = false;
	char *reader = start;
	while (reader < end)
	{
		size_t len = _zp_starts_with(reader, needle);
		if (len)
		{
			if (right_after_needle)
			{
				break;
			}
			right_after_needle = true;
			reader += len;
		}
		else
		{
			right_after_needle = false;
			reader++;
		}
	}
	char *writer = reader;
	while (reader < end)
	{
		size_t len = _zp_starts_with(reader, needle);
		if (len)
		{
			if (!right_after_needle)
			{
				for (size_t i = 0; i < len; i++)
				{
					writer[i] = reader[i];
				}
				writer += len;
			}
			right_after_needle = true;
			reader += len;
		}
		else
		{
			right_after_needle = false;
			*writer++ = *reader++;
		}
	}
	*len = writer - start;
}
void _zp_ke_write_chunk(char **writer, const char *chunk, size_t len, const char *write_start)
{
	if (*writer != write_start)
	{
		*(*writer)++ = '/';
	}
	memcpy(*writer, chunk, len);
	*writer += len;
}
_z_keyexpr_canon_status_t _zp_canonize(char *start, size_t *len)
{
	_zp_singleify(start, len, "$*");
	size_t canon_len = *len;
	_z_keyexpr_canon_status_t prefix_canon_state = _zp_canon_prefix(start, &canon_len);
	switch (prefix_canon_state)
	{
	case _Z_LONE_DOLLAR_STAR:
	case _Z_SINGLE_STAR_AFTER_DOUBLE_STAR:
	case _Z_DOUBLE_STAR_AFTER_DOUBLE_STAR:
		break; // Canonization only handles these errors, so finding any other error (or a success) means immediate return
	default:
		return prefix_canon_state;
	}
	const char *end = start + *len;
	char *reader = start + canon_len;
	const char *write_start = reader;
	char *writer = reader;
	char *next_slash = memchr(reader, '/', end - reader);
	char const *chunk_end = next_slash ? next_slash : end;
	bool in_big_wild = false;
	if (chunk_end - reader == 2 && reader[1] == '*')
	{
		switch (*reader)
		{
		case '*':
			in_big_wild = true;
			break;
		case '$': // if the chunk is $*, replace it with *.
			*writer++ = '*';
			break;
		default:
			assert(false); // anything before "$*" or "**" must be part of the canon prefix
			break;
		}
	}
	else
	{
		assert(false); // anything before "$*" or "**" must be part of the canon prefix
	}
	while (next_slash)
	{
		reader = next_slash + 1;
		next_slash = memchr(reader, '/', end - reader);
		chunk_end = next_slash ? next_slash : end;
		switch (chunk_end - reader)
		{
		case 0:
			return _Z_EMPTY_CHUNK;
		case 1:
			_zp_ke_write_chunk(&writer, "*", 1, write_start);
			continue;
		case 2:
			if (reader[1] == '*')
			{
				switch (*reader)
				{
				case '$':
					_zp_ke_write_chunk(&writer, "*", 1, write_start);
					continue;
				case '*':
					in_big_wild = true;
					continue;
				}
			}
			break;
		}
		unsigned char in_dollar = 0;
		for (char const *c = reader; c < end; c++)
		{
			switch (*c)
			{
			case '#':
			case '?':
				return _Z_CONTAINS_SHARP_OR_QMARK;
			case '$':
				if (in_dollar)
				{
					return _Z_DOLLAR_AFTER_DOLLAR_OR_STAR;
				}
				in_dollar += 1;
				break;
			case '*':
				if (in_dollar != 1)
				{
					return _Z_STARS_IN_CHUNK;
				}
				in_dollar += 2;
				break;
			default:
				if (in_dollar == 1)
				{
					return _Z_CONTAINS_UNBOUND_DOLLAR;
				}
				in_dollar = 0;
			}
		}
		if (in_dollar == 1)
		{
			return _Z_CONTAINS_UNBOUND_DOLLAR;
		}
		if (in_big_wild)
		{
			_zp_ke_write_chunk(&writer, "**", 2, write_start);
		}
		_zp_ke_write_chunk(&writer, reader, chunk_end - reader, write_start);
		in_big_wild = false;
	}
	if (in_big_wild)
	{
		_zp_ke_write_chunk(&writer, "**", 2, write_start);
	}
	*len = writer - start;
	return _Z_SUCCESS;
}

_z_keyexpr_canon_status_t _zp_canonize_null_terminated(char *start)
{
	size_t len = strlen(start);
	size_t newlen = len;
	_z_keyexpr_canon_status_t result = _zp_canonize(start, &newlen);
	if (newlen < len)
	{
		start[newlen] = 0;
	}
	return result;
}
#include "stdio.h"

void _zp_singleify_null_terminated(char *start, const char *needle)
{
	size_t len = strlen(start);
	size_t newlen = len;
	_zp_singleify(start, &newlen, needle);
	if (newlen < len)
	{
		start[newlen] = 0;
	}
}

int main()
{
#define N 18
	const char *input[N] = {
		"greetings/hello/there",
		"greetings/good/*/morning",
		"greetings/*",
		"greetings/*/**",
		"greetings/$*",
		"greetings/**/*/morning",
		"greetings/**/*",
		"greetings/**/**",
		"greetings/**/*/**",
		"$*",
		"$*$*",
		"$*$*$*",
		"$*hi$*$*",
		"$*$*hi$*",
		"hi$*$*$*",
		"$*$*$*hi",
		"$*$*$*hi$*$*$*",
	};
	_z_keyexpr_canon_status_t expected[N] = {
		_Z_SUCCESS,
		_Z_SUCCESS,
		_Z_SUCCESS,
		_Z_SUCCESS,
		_Z_LONE_DOLLAR_STAR,
		_Z_SINGLE_STAR_AFTER_DOUBLE_STAR,
		_Z_SINGLE_STAR_AFTER_DOUBLE_STAR,
		_Z_DOUBLE_STAR_AFTER_DOUBLE_STAR,
		_Z_SINGLE_STAR_AFTER_DOUBLE_STAR,
	};
	const char *canonized[N] = {
		"greetings/hello/there",
		"greetings/good/*/morning",
		"greetings/*",
		"greetings/*/**",
		"greetings/*",
		"greetings/*/**/morning",
		"greetings/*/**",
		"greetings/**",
		"greetings/*/**",
		"*",
		"*",
		"*",
		"$*hi$*",
		"$*hi$*",
		"hi$*",
		"$*hi",
		"$*hi$*",
	};
	for (int i = 0; i < N; i++)
	{
		const char *ke = input[i];
		char canon[128];
		strcpy(canon, ke);
		_zp_canonize_null_terminated(canon);
		printf("%s expected %s got %s\n", ke, canonized[i], canon);
		assert(!strcmp(canonized[i], canon));
	}
	return 0;
}
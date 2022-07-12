#include "stdbool.h"
#include "string.h"

enum _z_keyexpr_canon_error_t
{
	_Z_SUCCESS,						  // the key expression is canon
	_Z_EMPTY_CHUNK,					  // the key contains empty chunks
	_Z_LONE_DOLLAR_STAR,			  // the key contains a `$*` chunk, which must be replaced by `*`
	_Z_STARS_IN_CHUNK,				  // the key contains a `*` in a chunk without being escaped by a DSL, which is forbidden
	_Z_SINGLE_STAR_AFTER_DOUBLE_STAR, // the key contains `**/*`, which must be replaced by `*/**`
	_Z_DOUBLE_STAR_AFTER_DOUBLE_STAR, // the key contains `**/**`, which must be replaced by `**`
	_Z_DOLLAR_AFTER_DOLLAR_OR_STAR,	  // the key contains `$*$` or `$$`, which is forbidden
	_Z_CONTAINS_POUND_SIGN,			  // the key contains `#`, which is forbidden
	_Z_CONTAINS_INTERROGATION,		  // the key contains `?`, which is forbidden
	_Z_CONTAINS_UNBOUND_DOLLAR		  // the key contains a `$` which isn't bound to a DSL
};

enum _z_keyexpr_canon_error_t
_z_is_canon_ke(const char *start, size_t len)
{
	bool in_big_wild = false;
	char const *chunk_start = start;
	char const *chunk_end;
	do
	{
		bool this_chunk_is_big_wild = false;
		chunk_end = memchr(chunk_start, '/', len);
		size_t chunk_len = chunk_end ? (size_t)(chunk_end - chunk_start) : len;
		switch (chunk_len)
		{
		case 0:
			return _Z_EMPTY_CHUNK;
		case 1:
			if (in_big_wild && *chunk_start == '*')
			{
				return _Z_SINGLE_STAR_AFTER_DOUBLE_STAR;
			}
			break;
		case 2:
			if (chunk_start[1] == '*')
			{
				switch (*chunk_start)
				{
				case '$':
					return _Z_LONE_DOLLAR_STAR;
				case '*':
					if (in_big_wild)
					{
						return _Z_DOUBLE_STAR_AFTER_DOUBLE_STAR;
					}
					else
					{
						this_chunk_is_big_wild = true;
					}
				}
			}
			break;
		}
		const char *end = chunk_start + chunk_len;
		unsigned char in_dollar = 0;
		for (char const *c = chunk_start; c < end; c++)
		{
			switch (*c)
			{
			case '#':
				return _Z_CONTAINS_POUND_SIGN;
			case '?':
				return _Z_CONTAINS_INTERROGATION;
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
		in_big_wild = this_chunk_is_big_wild;
	} while (chunk_end);
	return _Z_SUCCESS;
}
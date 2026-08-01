//
// Created by SyncShard on 7/27/26.
//

#ifndef KERNEL_PROJECT_CTYPE_H
#define KERNEL_PROJECT_CTYPE_H

#include "types.h"

#define _CTYPE static inline __attribute__((const, always_inline, used))

/* These do not explicitly comply with POSIX standards as they take u8's. ------ *
 * But this is only used inside the kernel anyway. As long as they do what ----- *
 * is expected of them and the compiler doesn't mangle them, it wont matter. --- *
 * And these functions are so small, it wouldn't hurt to add most of them. ----- *
 * Also, ascii only. ----------------------------------------------------------- *
 * ------------------------------------------------------------------------Sync- */


_CTYPE bool isupper(const u8 c) { return c >= 'A' && c <= 'Z'; }
_CTYPE bool islower(const u8 c) { return c >= 'a' && c <= 'z'; }
_CTYPE bool isdigit(const u8 c) { return c >= '0' && c <= '9'; }
_CTYPE bool isblank(const u8 c) { return c == ' ' || c == '\t'; }
_CTYPE bool isspace(const u8 c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
_CTYPE bool iscntrl(const u8 c) { return c < 0x20 || c == 0x7f; }


_CTYPE bool ispunct(const u8 c)
{
	return (c >= '!' && c <= '/') ||
	       (c >= ':' && c <= '@') ||
	       (c >= '[' && c <= '`') ||
	       (c >= '{' && c <= '~');
}



_CTYPE bool isalpha(const u8 c) { return islower(c) || isupper(c); }
_CTYPE bool isalnum(const u8 c) { return isalpha(c) || isdigit(c); }
_CTYPE bool isgraph(const u8 c) { return isalnum(c) || ispunct(c); }
_CTYPE bool isprint(const u8 c) { return isgraph(c) || c == ' '; }



// the reject child of being 8 characters long instead of 7.
_CTYPE bool isxdigit(const u8 c)
{
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

#endif //KERNEL_PROJECT_CTYPE_H

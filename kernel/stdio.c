// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <kctype.h>
#include <kstring.h>
#include <macros.h>
#include <stdarg.h>
#include <types.h>

#include "stdio.h"


static const char printf_digits[] = "0123456789ABCDEF";

enum strnum_flags : u8 {
	STR_SIGN     = 1 << 1,
        STR_PLUS     = 1 << 2,
	STR_LEFT     = 1 << 3,
	STR_ZERO_PAD = 1 << 4,
	STR_LOWER    = 1 << 5,
        STR_SPACE    = 1 << 6,
	STR_SPEC     = 1 << 7,
};

enum printf_bases : i8 {
        PRINTF_BASE_OCT = 8,
        PRINTF_BASE_DEC = 10,
        PRINTF_BASE_HEX = 16,
};

enum printf_exts : i8 {
        PRINTF_EXT_64 = 'l',
        PRINTF_EXT_32 = 0,
        PRINTF_EXT_16 = 'h',
};

static i64 atoi_bit(const char **str)
{
        i64 i = 0;
        while (isdigit(**str)) i = (i * 10) + (*((*str)++) - '0');
        return i;
}


static inline char reta(u64 *n, u64 base)
{
        u64 i = *n % base;
        *n /= base;
        return printf_digits[i];
}


static char *str_num(char *str, 
                     u64 magnitude, 
                     bool negative,
                     enum printf_bases base, 
                     i64 width, 
                     i64 precision, 
                     u32 flags)
{
        char tmp[128];
        isize i = {};

        char lower = (char)(flags & STR_LOWER);
        char sign = {};
        char pad = (flags & STR_ZERO_PAD) ? '0' : ' ';

        if (flags & STR_LEFT) flags &= ~STR_ZERO_PAD;
        if (flags & STR_SIGN) {
                if (negative) {
                        sign = '-';
                        magnitude = -magnitude;
                        width--;
                } else if (flags & STR_PLUS) {
                        sign = '+';
                        width--;
                } else if (flags & STR_SPACE) {
                        sign = ' ';
                        width--;
                }
        }

        if (flags & STR_SPEC) {
                if (base == PRINTF_BASE_HEX) width -= 2;
                else if (base == PRINTF_BASE_OCT) width--;
        }

        if (!(magnitude == 0 && precision == 0)) {
                do { tmp[i++] = (char)(reta(&magnitude, base) | lower); } while (magnitude);
        }

        if (i > precision) precision = i;
        width -= precision;

        if (!(flags & (STR_ZERO_PAD + STR_LEFT))) { while (width-- > 0) *str++ = ' '; }
        if (sign) *str++ = sign;

        if (flags & STR_SPEC) {
                if (base == PRINTF_BASE_OCT) { 
                        *str++ = '0';
                } else if (base == PRINTF_BASE_HEX) {
                        *str++ = '0';
                        *str++ = (char)('X' | lower);
                }
        }

        if (!(flags & STR_LEFT)) while (width-- > 0) *str++ = pad;
        while (i < precision--) *str++ = '0';
        while (i) *str++ = tmp[--i];
        while (width-- > 0) *str++ = ' ';

        return str;
}


static inline u32 fmt_setflags(const char **pfmt)
{
        u32 flags = {};

        for (;;) {
                switch (**pfmt) {
                case '+': flags |= STR_PLUS; break;
                case '-': flags |= STR_LEFT; break;
                case ' ': flags |= STR_SPACE; break;
                case '#': flags |= STR_SPEC; break;
                case '0': flags |= STR_ZERO_PAD; break;
                default: 
                          return flags;
                }

                (*pfmt)++;
        }
}

static inline enum printf_exts fmt_setext(const char **pfmt)
{
        const char *fmt = *pfmt;
        i64 ext = {};

        if (*fmt == 'h') { 
                ext = PRINTF_EXT_16;
                (*pfmt)++;
        } else if (*fmt == 'l') {
                ext = PRINTF_EXT_64;
                (*pfmt)++;
        }

        return ext;
}

static inline i64 fmt_setwidth(const char **pfmt, u32 *flags, va_list args)
{
        i64 width = {};
        const char *fmt = *pfmt;

        if (isdigit(*fmt)) {
                width = atoi_bit(&fmt);
        } else if (*fmt == '*') {
                width = va_arg(args, i32);
                if (width < 0) {
                        width = -width;
                        *flags |= STR_LEFT;
                }
                (*pfmt)++;
        }

        return width;
}

static inline i64 fmt_setprec(const char **pfmt, va_list args)
{
        const char *fmt = *pfmt;
        i64 precision = -1;

        if (*fmt == '.') {
                fmt++;
                if (isdigit(*fmt)) { 
                        precision = atoi_bit(&fmt);
                } else if (*fmt == '*') {
                        precision = va_arg(args, i32);
                }

                if (precision < 0) precision = 0;
        }

        return precision;
}

static inline u64 fmt_setnum(enum printf_exts ext, u32 flags, bool *negative, va_list args)
{
        u64 n = {};
        i64 t = {};
        *negative = false;

        if (ext == PRINTF_EXT_64) {
                t = va_arg(args, u64);
                n = t;
                if (flags & STR_SIGN) {
                        if (t < 0) *negative = true;
                        n = (i64)n;
                }
        } else if (ext == PRINTF_EXT_16) {
                t = (u16)va_arg(args, i32);
                n = t;
                if (flags & STR_SIGN) { 
                        if (t < 0) *negative = true;
                        n = (i16)n;
                }
        } else {
                if (flags & STR_SIGN) {
                        t = va_arg(args, i32);
                        if (t < 0) *negative = true;
                        n = t;
                } else {
                        n = va_arg(args, u32);
                }
        }

        return n;
}

_format_(printf, 2, 0)
isize vsprintf(char *buf, const char *fmt, va_list args)
{
        char *str = buf;

        for (; *fmt != 0; fmt++) {
                if (*fmt != '%') {
                        *str++ = *fmt;
                        continue;
                }

                u64 n = {};
                enum printf_bases base = PRINTF_BASE_DEC;

                fmt++;
                u32 flags = fmt_setflags(&fmt);
                i64 width = fmt_setwidth(&fmt, &flags, args);
                i64 precision = fmt_setprec(&fmt, args); 
                enum printf_exts ext = fmt_setext(&fmt);

                switch (*fmt) {
                case 'c':
                        if (!(flags & STR_LEFT)) while (--width) *str++ = ' ';
                        *str++ = (char)va_arg(args, int);
                        if (!(flags & STR_LEFT)) while (--width) *str++ = ' ';
                        continue;
                case 's':
                        char *strarg = va_arg(args, char *);
                        isize len = (isize)strnlen(strarg, precision);

                        if (!(flags & STR_LEFT)) while (len < width--) *str++ = ' ';
                        for (isize i = 0; i < len; i++) { *str++ = *strarg++; }
                        if (!(flags & STR_LEFT)) while (len < width--) *str++ = ' ';
                        break;
                case 'd':
                case 'i':
                        flags |= STR_SIGN;
                case 'u':
                        break;
                case 'o':
                        base = PRINTF_BASE_OCT;
                        break;
                case 'n':
                        if (ext == PRINTF_EXT_64) {
                                i64 *p = va_arg(args, i64 *);
                                *p = str - buf;
                        } else {
                                i32 *p = va_arg(args, i32 *);
                                *p = (i32)(str - buf);
                        }

                        continue;
                case 'x':
                        flags |= STR_LOWER;
                        _fallthrough_;
                case 'X':
                        base = PRINTF_BASE_HEX;
                        break;
                case 'p':
                        if (width == -1)  {
                                width = sizeof(void *) * 2;
                                flags |= STR_ZERO_PAD;
                        }

                        i64 sn = (i64)va_arg(args, void *);
                        base = PRINTF_BASE_HEX;

                        str = str_num(str, sn, false, base, width, precision, flags);
                        continue;
                case '%':
                        *str++ = '%';
                        continue;
                default:
                        *str++ = '%';
                        if (*fmt) *str++ = *fmt;
                        else fmt++; 
			continue;
		}
                bool negative = false;

                n = fmt_setnum(ext, flags, &negative, args);
                str = str_num(str, n, negative, base, width, precision, flags);
        }

        *str = '\0';
        return str - buf;
}

_format_(printf, 2, 3)
isize sprintf(char *buf, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);

        isize ret = vsprintf(buf, fmt, args);

        va_end(args);
        return ret;
}


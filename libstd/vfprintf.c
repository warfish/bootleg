#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

static void _debugcon_putc(char c)
{
    __asm__ volatile("out %%al, %%dx" ::"a"(c), "d"(0x402):);
}

/**
 * Printf-like format string parsing
 */

/** Flag characters */
enum {
    FMT_FLAG_ALTERNATE = (1u << 1),
    FMT_FLAG_ZERO_PADDING = (1u << 2),
    FMT_FLAG_LEFT_ADJUST = (1u << 3),
    FMT_FLAG_BLANK = (1u << 4),
    FMT_FLAG_SIGN = (1u << 5)
};

/** Length modifiers */
enum {
    FMT_LENGTH_CHAR,        /* hh */
    FMT_LENGTH_SHORT,       /* h */
    FMT_LENGTH_INT,         /* <empty> */
    FMT_LENGTH_LONG,        /* l */
    FMT_LENGTH_LONGLONG,    /* ll */
    FMT_LENGTH_SIZET,       /* z */
};

struct fmt_parse {
    unsigned flags;
    unsigned width;
    unsigned length;
    bool upcase;

};

static int format_char(struct fmt_parse* fmt, FILE* filp, char c)
{
    filp(c);
    return 1;
}

static int format_str(struct fmt_parse* fmt, FILE* filp, const char* str)
{
    size_t len = strlen(str);
    char pad_char = (fmt->flags & FMT_FLAG_ZERO_PADDING ? '0' : ' ');

    /* Left width padding */
    if (!(fmt->flags & FMT_FLAG_LEFT_ADJUST)) {
        for (size_t i = len; i < fmt->width; ++i) {
            filp(pad_char);
        }
    }

    for (size_t i = 0; i < len; ++i) {
        filp(str[i]);
    }

    /* Right width padding */
    if (fmt->flags & FMT_FLAG_LEFT_ADJUST) {
        for (size_t i = len; i < fmt->width; ++i) {
            filp(pad_char);
        }
    }

    return len + (fmt->width > len ? fmt->width - len : 0);
}

static int format_number(struct fmt_parse* fmt, FILE* filp, va_list* ap, bool is_signed, unsigned base)
{
    assert(base == 8 || base == 10 || base == 16);

    signed long long sll;
    unsigned long long ull; /* If number is signed, will hold absolute value */
    bool neg;

    if (base != 10 || !is_signed) {
        switch (fmt->length) {
        case FMT_LENGTH_CHAR: ull = va_arg(*ap, unsigned char); break;
        case FMT_LENGTH_SHORT: ull = va_arg(*ap, unsigned short); break;
        case FMT_LENGTH_INT: ull = va_arg(*ap, unsigned int); break;
        case FMT_LENGTH_LONG: ull = va_arg(*ap, unsigned long); break;
        case FMT_LENGTH_LONGLONG: ull = va_arg(*ap, unsigned long long); break;
        case FMT_LENGTH_SIZET: ull = va_arg(*ap, size_t); break;
        default: assert(0);
        }

        neg = false;
    } else if (is_signed) {
        switch (fmt->length) {
        case FMT_LENGTH_CHAR: sll = va_arg(*ap, signed char); break;
        case FMT_LENGTH_SHORT: sll = va_arg(*ap, signed short); break;
        case FMT_LENGTH_INT: sll = va_arg(*ap, signed int); break;
        case FMT_LENGTH_LONG: sll = va_arg(*ap, signed long); break;
        case FMT_LENGTH_LONGLONG: sll = va_arg(*ap, signed long long); break;
        case FMT_LENGTH_SIZET: sll = va_arg(*ap, ssize_t); break;
        default: assert(0);
        }

        neg = sll < 0;
        ull = neg ? -sll : sll;
    }

    const char* alpha = (fmt->upcase ? "0123456789ABCDEF" : "0123456789abcdef");

    /* Enough to hold base-8 number of maximum length */
    char buffer[32];
    buffer[31] = '\0';
    char* pbuf = &buffer[30];

    do {
        *pbuf-- = alpha[ull % base];
        ull /= base;
    } while (ull);

    if (neg || (fmt->flags & FMT_FLAG_SIGN)) {
        *pbuf-- = neg ? '-' : '+';
    }

    if (fmt->flags & FMT_FLAG_ALTERNATE) {
        if (base == 8) {
            filp('0');
        } else if (base == 16) {
            filp('0');
            filp('x');
        }
    }

    return format_str(fmt, filp, ++pbuf);
}

static int _vfprintf(FILE* filp, const char* s, va_list ap)
{
    int res = 0;

    char c;
    struct fmt_parse fmt;
    while ((c = *s++) != '\0') {
        if (c != '%') {
            filp(c);
            ++res;
            continue;
        }

        /* Complete conversion specification is '%%', otherwise invalid */
        if (*s == '%') {
            filp('%');
            ++s;
            ++res;
            continue;
        }

        memset(&fmt, 0, sizeof(fmt));

        /* Character % is followed by zero or more flag characters */
        while (*s != '\0') {
            if (*s == '#')      fmt.flags |= FMT_FLAG_ALTERNATE;
            else if (*s == '0') fmt.flags |= FMT_FLAG_ZERO_PADDING;
            else if (*s == '-') fmt.flags |= FMT_FLAG_LEFT_ADJUST;
            else if (*s == ' ') fmt.flags |= FMT_FLAG_BLANK;
            else if (*s == '+') fmt.flags |= FMT_FLAG_SIGN;
            else                break;
            ++s;
        }

        /* Flags characters are followed by optional field width specifier */
        while (*s != '\0') {
            if (*s >= '0' && *s <= '9') fmt.width = fmt.width * 10 + *s - '0';
            else break;
            ++s;
        }

        /* Width is followed by optional precision, which we don't support */

        /* Optional length modifier */
        if (*s == 'h') {
            ++s;
            fmt.length = FMT_LENGTH_SHORT;
            if (*s == 'h') {
                ++s;
                fmt.length = FMT_LENGTH_CHAR;
            }
        } else if (*s == 'l') {
            ++s;
            fmt.length = FMT_LENGTH_LONG;
            if (*s == 'l') {
                ++s;
                fmt.length = FMT_LENGTH_LONGLONG;
            }
        } else if (*s == 'z') {
            ++s;
            fmt.length = FMT_LENGTH_SIZET;
        } else {
            fmt.length = FMT_LENGTH_INT;
        }
    
        /* Required conversion specifier */
        switch (*s++) {
        case 'd':
        case 'i': res += format_number(&fmt, filp, &ap, true, 10); break;
        case 'u': res += format_number(&fmt, filp, &ap, false, 10); break;
        case 'o': res += format_number(&fmt, filp, &ap, false, 8); break;
        case 'x': res += format_number(&fmt, filp, &ap, false, 16); break;
        case 'X': fmt.upcase = true;
                  res += format_number(&fmt, filp, &ap, false, 16); break;
        case 'p': res += format_number(&fmt, filp, &ap, false, 16); break;
        case 'c': res += format_char(&fmt, filp, va_arg(ap, char)); break;
        case 's': res += format_str(&fmt, filp, va_arg(ap, const char*)); break;
        default: /* Invalid, ignore the whole thing */ break;
        }
    }

    return res;
}

////////////////////////////////////////////////////////////////////////////////

FILE* stdout = _debugcon_putc;
FILE* stderr = _debugcon_putc;

int vfprintf(FILE* filp, const char* format, va_list ap)
{
    return _vfprintf(filp, format, ap);
}

int fprintf(FILE* filp, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int res = _vfprintf(filp, format, ap);
    va_end(ap);

    return res;
}

int vprintf(const char* format, va_list ap)
{
    return _vfprintf(stdout, format, ap);
}

int printf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int res = _vfprintf(stdout, format, ap);
    va_end(ap);

    return res;
}

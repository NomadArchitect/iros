#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int printf_internal(bool (*print)(void *obj, const char *s, size_t len), void *obj, const char *__restrict format, va_list parameters);

#ifdef __is_libk
#define stdout NULL

bool kprint(const char*, size_t);
bool fprint(void *stream, const char *s, size_t len) {
	(void) stream;
	return kprint(s, len);
}

#else
#include <unistd.h>

bool fprint(void *stream, const char *s, size_t n) {
	return n == fwrite(s, 1, n, stream);
}

#endif /* __is_libk */

struct snprintf_info {
	char *s;
	size_t max_len;
	size_t pos;
};

bool sprint(void *_info, const char *s, size_t len) {
	struct snprintf_info *info = _info;
	for (size_t i = 0; i < len && info->pos < info->max_len; i++) {
		info->s[info->pos++] = s[i];
	}

	return true;
}

static int parseInt(const char* num, size_t length) {
	int n = 0;
	for (size_t i = 0; i < length; i++) {
		int digit = num[i] - '0';
		for (unsigned char t = 1; t < length - i; t++) {
			digit *= 10;
		}
		n += digit;
	}
	return n;
}

int snprintf(char *__restrict s, size_t len, const char *__restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = vsnprintf(s, len, format, parameters);

	va_end(parameters);
	return written;
}

int vsnprintf(char *__restrict s, size_t len, const char *__restrict format, va_list parameters) {
	struct snprintf_info info = { s, len, 0 };
	return printf_internal(sprint, &info, format, parameters);
}

int printf(const char *__restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = vprintf(format, parameters);

	va_end(parameters);
	return written;
}

int vprintf(const char *__restrict format, va_list parameters) {
	return vfprintf(stdout, format, parameters);
}

int fprintf(FILE *stream, const char *__restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = vfprintf(stream, format, parameters);

	va_end(parameters);
	return written;
}

int vfprintf(FILE *stream, const char *__restrict format, va_list parameters) {
	return printf_internal(fprint, stream, format, parameters);
}

int printf_internal(bool (*print)(void *obj, const char *s, size_t len), void *obj, const char *__restrict format, va_list parameters) {
	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(obj, format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		//bitmask with format 000[-][+][ ][#][0]
		unsigned char flags = 0;
		while (*format == '-' || *format == '+' || *format == ' ' || *format == '#' || *format == '0') {
			if (*format == '-')
				flags |= 0b00010000;
			else if (*format == '+')
				flags |= 0b00001000;
			else if (*format == ' ')
				flags |= 0b00000100;
			else if (*format == '#')
				flags |= 0b00000010;
			else
				flags |= 0b00000001;
			format++;
		}

		int width;
		if (*format == '*') { 
			width = va_arg(parameters, int);
			format++; 
		} else {
			size_t width_len = 0;
			while (*(format + width_len) >= '0' && *(format + width_len) <= '9') {
				width_len++;
			}
			width = parseInt(format, width_len);
			format += width_len;
		}

		int precision = -1;
		if (*format == '.') {
			format++;
			if (*format == '*') { 
				precision = va_arg(parameters, int);
				format++; 
			} else {
				size_t prec_len = 0;
				while (*(format + prec_len) >= '0' && *(format + prec_len) <= '9') {
					prec_len++;
				}
				precision = parseInt(format, prec_len);
				format += prec_len;
			}
		}
		/* (none) - 0
			h     - 1
            hh    - 2
			l     - 3
			ll    - 6
			j     - 4
			z     - 5
			t     - 7
			L     - 8
		*/
		unsigned char length_modifier = 0;
		while (*format == 'h') {
			format++;
			length_modifier++;
		}
		while (*format == 'l') {
			format++;
			length_modifier += 3;
		}
		if (*format == 'j') {
			format++;
			length_modifier = 4;
		} else if (*format == 'z') {
			format++;
			length_modifier = 5;
		} else if (*format == 't') {
			format++;
			length_modifier = 7;
		} else if (*format == 'L') {
			format++;
			length_modifier = 8;
		}

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (width < 1) width = 1;
			if (maxrem < (unsigned int) width) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			char space = ' ';
			if (!(flags & 0b00010000)) {
				for (int i = 0; i < width - 1; i++) {
					if (!print(obj, &space, 1))
						return -1;
				}
			}
			if (!print(obj, &c, 1))
				return -1;
			if (flags & 0b00010000) {
				for (int i = 1; i < width; i++) {
					if (!print(obj, &space, 1))
						return -1;
				}
			}
			written += width;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (len > (unsigned int) width) width = len;
			if (precision >= 0 && precision < width) width = precision;
			if (precision >= 0 && (unsigned int) precision < len)
				len = width = precision;
			if (maxrem < (unsigned int) width) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (len < (unsigned int) width) {
				char space = ' ';
				if (flags & 0b00010000) {
					if (!print(obj, str, len))
						return -1;
					while (len++ < (unsigned int) width) {
						if (!print(obj, &space, 1))
							return -1;
					}
				} else {
					for (size_t i = 0; i < width - len; i++)
						if (!print(obj, &space, 1))
							return -1;
					if (!print(obj, str, len))
						return -1;
				}
			} else {
				if (!print(obj, str, len))
					return -1;
			}
			written += width;
		} else if (*format == 'o') {
			format++;
			unsigned int num = va_arg(parameters, unsigned int);
			size_t len = 1;
			size_t len_prec;
			size_t len_width;
			unsigned int div = 1;
			while (num / div > 7) {
				div *= 8;
				len++;
			}
			if (precision == 0 && num == 0) {
				len = 0;
			}
			len_prec = len;
			len_width = len_prec;
			if (precision >= 0 && (unsigned int) precision > len) {
				len_prec = precision;
				len_width = precision;
			}
			if ((flags & 0b00001000) | (flags & 0b00000100))
				len_width++;
			if (flags & 0b00000010)
				len_width += 2;
			if ((unsigned int) width < len_width)
				width = len_width;
			if (maxrem < (unsigned int) width) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			char filler = ' ';
			if (!((flags & 0b00010000) | (flags & 0b00000001))) {
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			if (flags & 0b00001000) {
				char plus = '+';
				if (!print(obj, &plus, 1))
					return -1;
			} else if (flags & 0b00000100) {
				char space = ' ';
				if (!print(obj, &space, 1))
					return -1;
			}
			if (flags & 0b00000010) {
				char zero = '0';
				if (!print(obj, &zero, 1))
					return -1;
				if (!print(obj, format, 1))
					return -1;
			}
			if (!(flags & 0b00010000) && (flags & 0b00000001)) {
				filler = '0';
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			while (len < len_prec) {
				char zero = '0';
				if (!print(obj, &zero, 1))
					return -1;
				len++;
			}
			if (num == 0) {
				if (precision != 0) {
					char zero = '0';
					if (!print(obj, &zero, 1))
						return -1;
				}
			} else {
				char digit;
				while (div > 7) {
					unsigned int n = num / div;
					div /= 8;
					digit = n % 8 + '0';
					if (!print(obj, &digit, 1))
						return -1;
				}
				digit = num % 8 + '0';
				if (!print(obj, &digit, 1))
					return -1;
			}
			if (flags & 0b00010000) {
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			written += width;
		} else if (*format == 'x' || *format == 'X') {
            if (length_modifier == 0) {
                unsigned int num = va_arg(parameters, unsigned int);
                size_t len = 1;
                size_t len_prec;
                size_t len_width;
                unsigned int div = 1;
                while (num / div > 15) {
                    div *= 16;
                    len++;
                }
                if (precision == 0 && num == 0) {
                    len = 0;
                }
                len_prec = len;
                len_width = len_prec;
                if (precision >= 0 && (unsigned int) precision > len) {
                    len_prec = precision;
                    len_width = precision;
                }
                if ((flags & 0b00001000) | (flags & 0b00000100))
                    len_width++;
                if (flags & 0b00000010)
                    len_width += 2;
                if ((unsigned int) width < len_width)
                    width = len_width;
                if (maxrem < (unsigned int) width) {
                    // TODO: Set errno to EOVERFLOW.
                    return -1;
                }
                char filler = ' ';
                if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                if (flags & 0b00001000) {
                    char plus = '+';
                    if (!print(obj, &plus, 1))
                        return -1;
                } else if (flags & 0b00000100) {
                    char space = ' ';
                    if (!print(obj, &space, 1))
                        return -1;
                }
                if (flags & 0b00000010) {
                    char zero = '0';
                    if (!print(obj, &zero, 1))
                        return -1;
                    if (!print(obj, format, 1))
                        return -1;
                }
                if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                    filler = '0';
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                while (len < len_prec) {
                    char zero = '0';
                    if (!print(obj, &zero, 1))
                        return -1;
                    len++;
                }
                if (num == 0) {
                    if (precision != 0) {
                        char zero = '0';
                        if (!print(obj, &zero, 1))
                            return -1;
                    }
                } else {
                    char digit;
                    while (div > 15) {
                        unsigned int n = num / div;
                        div /= 16;
                        digit = n % 16;
                        if (digit < 10) {
                            digit += '0';
                        } else {
                            digit += *format - ('x' - 'a') - 10;
                        }
                        if (!print(obj, &digit, 1))
                            return -1;
                    }
                    digit = num % 16;
                    if (digit < 10) {
                        digit += '0';
                    } else {
                        digit += *format - ('x' - 'a') - 10;
                    }
                    if (!print(obj, &digit, 1))
                        return -1;
                }
                if (flags & 0b00010000) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                written += width;
                format++;
            } else if (length_modifier == 3 || length_modifier == 6) {
                unsigned long num = va_arg(parameters, unsigned long);
                size_t len = 1;
                size_t len_prec;
                size_t len_width;
                unsigned long div = 1;
                while (num / div > 15) {
                    div *= 16;
                    len++;
                }
                if (precision == 0 && num == 0) {
                    len = 0;
                }
                len_prec = len;
                len_width = len_prec;
                if (precision >= 0 && (unsigned int) precision > len) {
                    len_prec = precision;
                    len_width = precision;
                }
                if ((flags & 0b00001000) | (flags & 0b00000100))
                    len_width++;
                if (flags & 0b00000010)
                    len_width += 2;
                if ((unsigned int) width < len_width)
                    width = len_width;
                if (maxrem < (unsigned int) width) {
                    // TODO: Set errno to EOVERFLOW.
                    return -1;
                }
                char filler = ' ';
                if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                if (flags & 0b00001000) {
                    char plus = '+';
                    if (!print(obj, &plus, 1))
                        return -1;
                } else if (flags & 0b00000100) {
                    char space = ' ';
                    if (!print(obj, &space, 1))
                        return -1;
                }
                if (flags & 0b00000010) {
                    char zero = '0';
                    if (!print(obj, &zero, 1))
                        return -1;
                    if (!print(obj, format, 1))
                        return -1;
                }
                if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                    filler = '0';
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                while (len < len_prec) {
                    char zero = '0';
                    if (!print(obj, &zero, 1))
                        return -1;
                    len++;
                }
                if (num == 0) {
                    if (precision != 0) {
                        char zero = '0';
                        if (!print(obj, &zero, 1))
                            return -1;
                    }
                } else {
                    char digit;
                    while (div > 15) {
                        unsigned long n = num / div;
                        div /= 16;
                        digit = n % 16;
                        if (digit < 10) {
                            digit += '0';
                        } else {
                            digit += *format - ('x' - 'a') - 10;
                        }
                        if (!print(obj, &digit, 1))
                            return -1;
                    }
                    digit = num % 16;
                    if (digit < 10) {
                        digit += '0';
                    } else {
                        digit += *format - ('x' - 'a') - 10;
                    }
                    if (!print(obj, &digit, 1))
                        return -1;
                }
                if (flags & 0b00010000) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1))
                            return -1;
                    }
                }
                written += width;
                format++;
                }
		} else if (*format == 'u') {
			format++;
			unsigned int num = va_arg(parameters, unsigned int);
			size_t len = 1;
			size_t len_prec;
			size_t len_width;
			unsigned int div = 1;
			while (num / div > 9) {
				div *= 10;
				len++;
			}
			if (precision == 0 && num == 0) {
				len = 0;
			}
			len_prec = len;
			len_width = len_prec;
			if (precision >= 0 && (unsigned int) precision > len) {
				len_prec = precision;
				len_width = precision;
			}
			if ((flags & 0b00001000) | (flags & 0b00000100))
				len_width++;
			if (flags & 0b00000010)
				len_width += 2;
			if ((unsigned int) width < len_width)
				width = len_width;
			if (maxrem < (unsigned int) width) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			char filler = ' ';
			if (!((flags & 0b00010000) | (flags & 0b00000001))) {
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			if (flags & 0b00001000) {
				char plus = '+';
				if (!print(obj, &plus, 1))
					return -1;
			} else if (flags & 0b00000100) {
				char space = ' ';
				if (!print(obj, &space, 1))
					return -1;
			}
			if (flags & 0b00000010) {
				char zero = '0';
				if (!print(obj, &zero, 1))
					return -1;
				if (!print(obj, format, 1))
					return -1;
			}
			if (!(flags & 0b00010000) && (flags & 0b00000001)) {
				filler = '0';
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			while (len < len_prec) {
				char zero = '0';
				if (!print(obj, &zero, 1))
					return -1;
				len++;
			}
			if (num == 0) {
				if (precision != 0) {
					char zero = '0';
					if (!print(obj, &zero, 1))
						return -1;
				}
			} else {
				char digit;
				while (div > 9) {
					unsigned int n = num / div;
					div /= 10;
					digit = n % 10 + '0';
					if (!print(obj, &digit, 1))
						return -1;
				}
				digit = num % 10 + '0';
				if (!print(obj, &digit, 1))
					return -1;
			}
			if (flags & 0b00010000) {
				for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			written += width;
		} else if (*format == 'd' || *format == 'i') {
			format++;
			int num = va_arg(parameters, int);
			size_t len = 1;
			unsigned int div = 1;
			while (num / div > 9) {
				div *= 10;
				len++;
			}
			if (((flags & 0b00001000) | (flags & 0b00000100)) && num >= 0)
				len++;
			if (num < 0)
				len++;
			if ((unsigned int) width < len)
				width = len;
			if (maxrem < (unsigned int) width) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			char filler = ' ';
			if (!((flags & 0b00010000) | (flags & 0b00000001))) {
				for (unsigned int i = 0; i < (unsigned int) width - len; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			if (num >= 0) {
				if (flags & 0b00001000) {
					char plus = '+';
					if (!print(obj, &plus, 1))
						return -1;
				} else if (flags & 0b00000100) {
					char space = ' ';
					if (!print(obj, &space, 1))
						return -1;
				}
			} else {
				char minus = '-';
				if (!print(obj, &minus, 1))
					return -1;
				num = -num;
			}
			unsigned int to_write_buffer = (unsigned int) num;
			if (!(flags & 0b00010000) && (flags & 0b00000001)) {
				filler = '0';
				for (unsigned int i = 0; i < (unsigned int) width - len; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			if (to_write_buffer == 0) {
				char zero = '0';
				if (!print(obj, &zero, 1))
					return -1;
			} else {
				char digit;
				while (div > 9) {
					int n = to_write_buffer / div;
					div /= 10;
					digit = n % 10 + '0';
					if (!print(obj, &digit, 1))
						return -1;
				}
				digit = to_write_buffer % 10 + '0';
				if (!print(obj, &digit, 1))
					return -1;
			}
			if (flags & 0b00010000) {
				for (unsigned int i = 0; i < (unsigned int) width - len; i++) {
					if (!print(obj, &filler, 1))
						return -1;
				}
			}
			written += width;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(obj, format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	if (print == sprint) {
		char zero = '\0';
		if (!print(obj, &zero, 1))
			return -1;
	}

	return written;
}
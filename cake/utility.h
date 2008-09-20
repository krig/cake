#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

enum Colors { RED = 91, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

template <int SIZE>
struct strfmt
{
	char tmp[SIZE];
	strfmt(const char* str, ...)
	{
		va_list va_args;
		va_start(va_args, str);
		vsnprintf(tmp, SIZE, str, va_args);
		va_end(va_args);
	}

	operator const char* () const { return tmp; }
};

inline void cprintf(int color, const char* fmt, ...)
{
	if (g_silent_flag)
		return;

	char tmp[1024];
	va_list va_args;
	va_start(va_args, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, va_args);
	va_end(va_args);
	if (g_print_colors)
		printf("\e[%dm%s\e[0m\n", color, tmp);
	else
		printf("%s\n", tmp);
}

inline void verbed(int c, const char* verb, int color)
{
	if (c && !g_silent_flag)
		cprintf(color, "%d %s %s.", c, c == 1 ? "file" : "files", verb);
}

inline void printerr(const char* fmt, ...)
{
	if (g_silent_flag)
		return;
		
	char tmp[1024];
	va_list va_args;
	va_start(va_args, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, va_args);
	va_end(va_args);
	if (g_print_colors)
		fprintf(stderr, "\e[%dm%s\e[0m\n", RED, tmp);
	else
		fprintf(stderr, "%s\n", tmp);
}

inline bool endswith1of(const char* str, const char** end, int n)
{
	const int len = strlen(str);
	for (int i = 0; i < n; ++i)
	{
		const int elen = strlen(end[i]);
		if (elen > len)
			continue;
		if (strcmp(str+len-elen, end[i]) == 0)
			return true;
	}
	return false;
}

#endif /* __UTILITY_HPP__ */


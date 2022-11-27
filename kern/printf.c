// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h> // 在里面声明了cputchar()
#include <inc/stdarg.h>


static void
putch(int ch, int *cnt)
{		
	cputchar(ch);
	*cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

// 这个函数是提供给其他文件使用的，用于直接打印？
// int i = 5;
// char *s = "deedy";
// cprintf("%d %s", i, s);
// 
int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}


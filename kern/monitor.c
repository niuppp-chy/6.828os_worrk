// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h> // 

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{"backtrace", "Trace back call stack", mon_backtrace},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	// 函数A调用函数B：
	// A函数调用栈：函数B的形参push，再call B的下一条指令push，
	// B函数调用栈：先函数A栈的%ebp push
	// 
	uint32_t ebp = read_ebp();
	int* ebp_base_ptr = (int*)ebp;
	uint32_t eip = ebp_base_ptr[1];

	cprintf("Stack backtrace:\n");

	while(1) {
		cprintf(" ebp %08x eip %08x", ebp, eip);
		
		int* args = ebp_base_ptr + 2;

		cprintf(" args");
		for (int i = 0; i < 5; i++)
			cprintf(" %0x", args[i]);
		cprintf("\n");

		// 读取debug信息
		struct Eipdebuginfo info;
		int ret = debuginfo_eip(eip, &info);
		cprintf("	%s: %d: %.*s+%d\n",
					info.eip_file, // ...		   所在的源文件名称
					info.eip_line, // 下一条运行指令 所在的源文件的行号
					info.eip_fn_namelen, // 
					info.eip_fn_name,
					eip - info.eip_fn_addr // 下一条指令与所在函数收首条指令的偏移
					);

		if (ret)
			break;

		ebp = *ebp_base_ptr; // 获取到调用函数栈中的ebp
		ebp_base_ptr = (int*)ebp;
		eip = ebp_base_ptr[1];
	}




	// // 打印调用该函数的文件名和行号
	// uintptr_t addr = ; // 传入%eip，指向下一条将要执行的指令 
	// struct Eipdebuginfo *info;
	// debuginfo_eip(addr, info);
	// cprintf("    %s", info->); // 

	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16


// 在打印调用栈时可能看不到这个函数，因为这个函数已经由编译器內联了
static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

// 在init.c中有调用
void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

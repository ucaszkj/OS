#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

/* kernel printf */
int printk(const char *fmt, ...);

/* user printk */
int printf(const char *fmt, ...);

/* kernel (sys) print */
int do_print(const char *fmt, ...);

#endif
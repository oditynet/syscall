//gcc syscall.c -static -nostdlib -fno-stack-protector -o syscall
//#include <stdlib.h>
#include <linux/mman.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#define _GNU_SOURCE



//#include <linux/time.h>
#include <stddef.h>
//#include <sys/syscall.h>

//#define SYS_write      1
//#define SYS_mmap       9
//#define SYS_nanosleep  35
//#define SYS_clone      56
//#define SYS_exit       60
//#define SYS_futex      202
//#define SYS_exit_group 231

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#define SYSCALL1(n, a) \
    syscall6(n,(long)(a),0,0,0,0,0)
#define SYSCALL2(n, a, b) \
    syscall6(n,(long)(a),(long)(b),0,0,0,0)
#define SYSCALL3(n, a, b, c) \
    syscall6(n,(long)(a),(long)(b),(long)(c),0,0,0)
#define SYSCALL4(n, a, b, c, d) \
    syscall6(n,(long)(a),(long)(b),(long)(c),(long)(d),0,0)
#define SYSCALL5(n, a, b, c, d, e) \
    syscall6(n,(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),0)
#define SYSCALL6(n, a, b, c, d, e, f) \
    syscall6(n,(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),(long)(f))
    
static long syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    register long ret;
    register long r10 asm("r10") = d;
    register long r8  asm("r8")  = e;
    register long r9  asm("r9")  = f;
    __asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
}

void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}
int lti(long num){
	return (int) ( num & 0xFFFFFFFF );
}
char* reverse(char *buffer, int i, int j)
{
    while (i < j) {
        swap(&buffer[i++], &buffer[j--]);
    }
 
    return buffer;
}
 int abs(int value)
 {
 	if(value <0)
 		return -1*value;
 	return value;
 }
 
char* itoa(int value, char* buffer, int base)
{
    if (base < 2 || base > 32) {
        return buffer;
    }
    int n = abs(value);
    int i = 0;
    while (n)
    {
        int r = n % base;
        if (r >= 10) {
            buffer[i++] = 65 + (r - 10);
        }
        else {
            buffer[i++] = 48 + r;
        }
        n = n / base;
    }
    if (i == 0) {
        buffer[i++] = '0';
    }
    if (value < 0 && base == 10) {
        buffer[i++] = '-';
    }
    buffer[i] = '\n'; 
    return reverse(buffer, 0, i - 1);
}

static inline void write_str(const char *str) {
  while (*str) {
    syscall6(SYS_write, 1, (long)str, 1, 0, 0, 0);
    str++;
  }
}
static inline void write_num(int dig) {
        char buffer[80];
	write_str(itoa(dig,buffer,10));
}

static void millisleep(int ms)
{
    long ts[] = {ms/1000, ms%1000 * 1000000L};
    SYSCALL2(SYS_nanosleep, ts, ts);
}

static long fullwrite(int fd, void *buf, long len)
{
    for (long off = 0; off < len;) {
        long r = SYSCALL3(SYS_write, fd, buf+off, len-off);
        if (r < 0) {
            return r;
        }
        off += r;
    }
    return len;
}

__attribute((noreturn))
static void exit(int status)
{
    SYSCALL1(SYS_exit, status);
    __builtin_unreachable();
}

__attribute((noreturn))
static void exit_group(int status)
{
    SYSCALL1(SYS_exit_group, status);
    __builtin_unreachable();
}

static void futex_wait(int *futex, int expect)
{
    SYSCALL4(SYS_futex, futex, FUTEX_WAIT, expect, 0);
}

static void futex_wake(int *futex)
{
    SYSCALL3(SYS_futex, futex, FUTEX_WAKE, 0x7fffffff);
}

struct __attribute((aligned(16))) stack_head {
    void (*entry)(struct stack_head *);
    char *message;
    long message_length;
    int print_count;
    int join_futex;
};

int thread_func(void *arg) {
  write_str("Hello from thread!\n");
  millisleep(100000000);
  return 0;
}

__attribute((naked))
static long newthread(struct stack_head *stack)
{
    __asm volatile (
        "mov  %%rdi, %%rsi\n"     // arg2 = stack
        "mov  $0x50f00, %%edi\n"  // arg1 = clone flags
        "mov  $56, %%eax\n"       // SYS_clone
        "syscall\n"
        "mov  %%rsp, %%rdi\n"     // entry point argument
        "ret\n"
        : : : "rax", "rcx", "rsi", "rdi", "r11", "memory"
    );
}

static inline long mmap(void *addr, size_t length, int prot, int flags, int fd,
                        long offset) {
  return syscall6(SYS_mmap, (long)addr, (long)length, (long)prot, (long)flags,
                  (long)fd, offset);
}
static inline long clone3(struct clone_args *args) {
  register long rax __asm__("rax") = SYS_clone3;
  register long rdi __asm__("rdi") = (long)args;
  register long rsi __asm__("rsi") = (long)sizeof(struct clone_args);
  __asm__ volatile("syscall"
                   : "+r"(rax)
                   : "r"(rdi), "r"(rsi)
                   : "r11", "memory");

  if (rax < 0) {
    write_str("clone3 was unsuccessful!");
    exit(1);
  }

  return rax;
}
static void threadentry(struct stack_head *stack)
{
    char *message = stack->message;
    int length = stack->message_length;
    int count = stack->print_count;
    for (int i = 0; i < count; i++) {
        fullwrite(1, message, length);
        millisleep(25);
    }
    __atomic_store_n(&stack->join_futex, 1, __ATOMIC_SEQ_CST);
    futex_wake(&stack->join_futex);
    exit(0);
}

static struct stack_head *newstack(long size)
{
    unsigned long p = SYSCALL6(SYS_mmap, 0, size, 3, 0x22, -1, 0);
    if (p > -4096UL) {
        return 0;
    }
    long count = size / sizeof(struct stack_head);
    return (struct stack_head *)p + count - 1;
}

__attribute((force_align_arg_pointer))
void _start(void)
{
    write_str("[+] Thread create\n");
    //write_num(256);

    /*struct stack_head *stack = newstack(1<<16);
    stack->entry = threadentry;
    stack->message = "hello world\n";
    stack->message_length = 12;
    stack->print_count = 20;
    stack->join_futex = 0;
    write_num(newthread(stack));
    futex_wait(&stack->join_futex, 0);*/
    
    
   void *stack = (void *)mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN, -1, 0);

  void *stack_top = stack + 8192;
  unsigned long *sp = (unsigned long *)stack_top;
  *(--sp) = 0;
  *(--sp) = (unsigned long)thread_func;

    struct clone_args args = {0};
    args.flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND;
  args.stack = (unsigned long)stack;
  args.stack_size = 8192;
  
  long tid = clone3(&args);
  write_num(lti(tid));
  if (tid > 0) {
    write_str("Created thread!\n");
    millisleep(100000);
  }
  else
  {
  	 write_str("Error!\n");
  }
  
    exit_group(0);
}

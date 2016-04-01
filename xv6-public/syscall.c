#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->sz || addr+4 > proc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;

  if(addr >= proc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)proc->sz;
  for(s = *pp; s < ep; s++)
    if(*s == 0)
      return s - *pp;
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size n bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;
  
  if(argint(n, &i) < 0)
    return -1;
  if((uint)i >= proc->sz || (uint)i+size > proc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
// TODO: chky, date system call
extern int sys_date(void);
extern int sys_dup2(void);
extern int sys_alarm(void);

static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
// TODO: chky, date system call
[SYS_date]    sys_date,
[SYS_dup2]    sys_dup2,
[SYS_alarm]    sys_alarm,
};

// FIXME: print syscall args
// by chky
extern int sys_chdir_arg(void);
extern int sys_close_arg(void);
extern int sys_dup_arg(void);
extern int sys_exec_arg(void);
extern int sys_exit_arg(void);
extern int sys_fork_arg(void);
extern int sys_fstat_arg(void);
extern int sys_getpid_arg(void);
extern int sys_kill_arg(void);
extern int sys_link_arg(void);
extern int sys_mkdir_arg(void);
extern int sys_mknod_arg(void);
extern int sys_open_arg(void);
extern int sys_pipe_arg(void);
extern int sys_read_arg(void);
extern int sys_sbrk_arg(void);
extern int sys_sleep_arg(void);
extern int sys_unlink_arg(void);
extern int sys_wait_arg(void);
extern int sys_write_arg(void);
extern int sys_uptime_arg(void);

//static int (*syscall_arg[])(void) = {
//[SYS_fork]    sys_fork_arg,
//[SYS_exit]    sys_exit_arg,
//[SYS_wait]    sys_wait_arg,
//[SYS_pipe]    sys_pipe_arg,
//[SYS_read]    sys_read_arg,
//[SYS_kill]    sys_kill_arg,
//[SYS_exec]    sys_exec_arg,
//[SYS_fstat]   sys_fstat_arg,
//[SYS_chdir]   sys_chdir_arg,
//[SYS_dup]     sys_dup_arg,
//[SYS_getpid]  sys_getpid_arg,
//[SYS_sbrk]    sys_sbrk_arg,
//[SYS_sleep]   sys_sleep_arg,
//[SYS_uptime]  sys_uptime_arg,
//[SYS_open]    sys_open_arg,
//[SYS_write]   sys_write_arg,
//[SYS_mknod]   sys_mknod_arg,
//[SYS_unlink]  sys_unlink_arg,
//[SYS_link]    sys_link_arg,
//[SYS_mkdir]   sys_mkdir_arg,
//[SYS_close]   sys_close_arg,
//};

// FIXME: added by chky
//static char *syscall_names[] = {
//[SYS_fork]    "fork",
//[SYS_exit]    "exit",
//[SYS_wait]    "wait",
//[SYS_pipe]    "pipe",
//[SYS_read]    "read",
//[SYS_kill]    "kill",
//[SYS_exec]    "exec",
//[SYS_fstat]   "fstat",
//[SYS_chdir]   "chdir",
//[SYS_dup]     "dup",
//[SYS_getpid]  "getpid",
//[SYS_sbrk]    "sbrk",
//[SYS_sleep]   "sleep",
//[SYS_uptime]  "uptime",
//[SYS_open]    "open",
//[SYS_write]   "write",
//[SYS_mknod]   "mknod",
//[SYS_unlink]  "unlink",
//[SYS_link]    "link",
//[SYS_mkdir]   "mkdir",
//[SYS_close]   "close",
//[SYS_date]    "date",
//};

void
syscall(void)
{
  int num;

  num = proc->tf->eax;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    proc->tf->eax = syscalls[num]();
    // FIXME: chky
    // cprintf("%s -> %d\n", syscall_names[num], proc->tf->eax);
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            proc->pid, proc->name, num);
    proc->tf->eax = -1;
  }
}

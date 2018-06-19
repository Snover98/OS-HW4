#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <stdio.h>

MODULE_LICENSE("GPL");

#define MAX_NAME_LENGTH 16
#define DEFAULT_SCAN_RANGE 200

void** sys_call_table = NULL;
int scan_range = DEFAULT_SCAN_RANGE;
char* program_name = NULL;

MODULE_PARM(scan_range, "i");
MODULE_PARM(program_name, "s");

void restore_params_to_default(){
	sys_call_table = NULL;
	program_name = NULL;
	scan_range = DEFAULT_SCAN_RANGE;
}

asmlinkage long (*orig_sys_kill)(pid_t,int);

asmlinkage long my_sys_kill(pid_t pid, int sig){
	//find the task
	task_t *p = find_task_by_pid(pid);
	
	//if the command is to kill the program named program_name, return an error
	if(sig == SIGKILL && !strncmp(program_name, p->comm, MAX_NAME_LENGTH)){
		return -EPERM;
	}
	
	//otherwise, use the original syscall
	return orig_sys_kill(pid, sig);
}

void find_sys_call_table(int scan_range) {
   int i;
   void** current_pointer = &system_utsname;
   for(i=0; i<scan_range; i++ && current_pointer++){
	   if(current_pointer[__NR_read] == (void*)&sys_read){
		   sys_call_table = current_pointer;
		   return;
	   }
   }
}

int init_module(void) {
   //if the program name was not inputted or the input range is invalid
   if(program_name == NULL || scan_range <= 0){
	   restore_params_to_default();
	   return 0;
   }
   
   while(sys_call_table==NULL){
		//try and find the sys_call_table
		find_sys_call_table(scan_range);
		scan_range*=2;
   }
   
   //if we did not find it, reset the parameters and return
   if(sys_call_table == NULL){
	   restore_params_to_default();
	   return 0;
   }  
  
   //save the old syscall
   orig_sys_kill = (void*)sys_call_table[__NR_kill];	
	
   //replace it with the new syscall
   sys_call_table[__NR_kill] = my_sys_kill;
   
   return 0;
}

void cleanup_module(void) {
   // TODO: complete the function
   if(program_name == NULL){
	   return;
   }
   
   //restore the old syscall
   sys_call_table[__NR_kill] = orig_sys_kill;
   
   //return parameters to default
   restore_params_to_default();
}


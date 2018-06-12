#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/sched.h>
// TODO: add more #include statements as necessary

MODULE_LICENSE("GPL");

#define current_pointer(i) ((&system_utsname) + (i)*4)
#define READ_OFFSET (3*4)
#define KILL_OFFSET (37*4)
#define MAX_NAME_LENGTH 16
#define DEFAULT_SCAN_RANGE 100

// TODO: add command-line arguments
void** sys_call_table = NULL;
int input_scan_range = DEFAULT_SCAN_RANGE;
char* program_name = NULL;

MODULE_PARM(input_scan_range, "i");
MODULE_PARM(program_name, "s");

int is_prog_name(char* name){
	int i;
	for(i=0; i<MAX_NAME_LENGTH; i++){
		if(program_name[i] != name[i]){
			return 0;
		}
	}
	
	return 1;
}

void restore_params_to_default(){
	sys_call_table = NULL;
	program_name = NULL;
	input_scan_range = DEFAULT_SCAN_RANGE;
}

// TODO: import original syscall and write new syscall
asmlinkage long (*orig_sys_kill)(pid_t,int);

asmlinkage long my_sys_kill(pid_t pid, int sig){
	task_t *p = = find_task_by_pid(pid);
	if(sig == SIGKILL && is_prog_name(p->comm)){
		return -EPERM;
	}
	return orig_sys_kill(pid, sig);
}

void find_sys_call_table(int scan_range) {
   // TODO: complete the function
   int i;
   for(i=0; i<scan_range; i++){
	   if(*current_pointer(i) == sys_read){
		   sys_call_table = current_pointer(i) - READ_OFFSET;
		   return;
	   }
   }
}

int init_module(void) {
   // TODO: complete the function
   //if the program name was not inputted or the input range is invalid
   if(program_name == NULL || input_scan_range <= 0){
	   restore_params_to_default();
	   return 0;
   }
   
   //try and find the sys_call_table
   find_sys_call_table(input_scan_range);
   //if we did not find it, reset the parameters and return
   if(sys_call_table == NULL){
	   restore_params_to_default();
	   return 0;
   }
   
   //save the old syscall
   orig_sys_kill = sys_call_table[KILL_OFFSET];
   //replace it with the new syscall
   sys_call_table[KILL_OFFSET] = my_sys_kill;
}

void cleanup_module(void) {
   // TODO: complete the function
   if(program_name == NULL){
	   return;
   }
   
   //restore the old syscall
   sys_call_table[KILL_OFFSET] = orig_sys_kill;
   //return parameters to default
   restore_params_to_default();
}


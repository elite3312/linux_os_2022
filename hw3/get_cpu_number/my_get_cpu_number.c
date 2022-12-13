//prototype of the new system call is as follows:     
//int get_CPU_number() 
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(my_get_cpu_number){
    
}
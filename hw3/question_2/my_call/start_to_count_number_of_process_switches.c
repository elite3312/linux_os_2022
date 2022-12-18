#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(start_to_count_number_of_process_switches){
    
    current->process_switches_count=0;
    
}
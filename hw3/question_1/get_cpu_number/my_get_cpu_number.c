//prototype of the new system call is as follows:     
//int get_CPU_number() 
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(my_get_cpu_number){
    //unsigned  _cpu_number_from_thread_info=-1;
    //_cpu_number_from_thread_info=current->thread_info->cpu;/* current CPU */
    //if(_cpu_number_from_thread_info==-1){
    //    return -1;
    //}
    unsigned   _cpu_number_from_task_struct=-1;
    _cpu_number_from_task_struct=current->cpu;/* current CPU */
    if(_cpu_number_from_task_struct==-1){
        return -1;
    }
    return _cpu_number_from_task_struct;
}
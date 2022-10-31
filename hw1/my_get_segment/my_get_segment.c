#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#define MAX_BUF_SIZE 128


struct Segment {
    unsigned long start_addr;
    unsigned long end_addr;
	char seg_name[MAX_BUF_SIZE];
};

struct ProcessSegments {
    pid_t pid;
    struct Segment code_seg;
    struct Segment data_seg;
    struct Segment heap_seg;
    struct Segment stack_seg;
};


SYSCALL_DEFINE1(my_get_segment, void* __user, user_thread_seg) {

	struct ProcessSegments* process_segments;
	struct task_struct* task;
    
	//struct vm_area_struct* current_vm_area;
	
	int ret = 0;
	

	process_segments = kmalloc(sizeof(struct ProcessSegments), GFP_KERNEL);
	ret = copy_from_user(process_segments, user_thread_seg, sizeof(struct ProcessSegments));
	
	if(ret != 0) {
        printk(KERN_ALERT "copy_from_user failed\n");
        return ret;
    }

    //System call return0??
	task = find_task_by_vpid(process_segments->pid);
	if (!task) {
        return 0;
    }
    
	
	process_segments->code_seg.start_addr = (unsigned long) task->mm->start_code;
    process_segments->code_seg.end_addr = (unsigned long) task->mm->end_code;
    strcpy(process_segments->code_seg.seg_name, "code_seg");
    
	process_segments->data_seg.start_addr = (unsigned long) task->mm->start_data;
	process_segments->data_seg.end_addr = (unsigned long) task->mm->end_data;
    strcpy(process_segments->data_seg.seg_name, "data_seg");
	
	process_segments->heap_seg.start_addr = (unsigned long) task->mm->start_brk;
    process_segments->heap_seg.end_addr = (unsigned long) task->mm->brk;
    strcpy(process_segments->heap_seg.seg_name, "heap_seg");
	
	process_segments->stack_seg.start_addr = (unsigned long) task->mm->start_stack;
    process_segments->stack_seg.end_addr = (unsigned long) (task->mm->start_stack + task->mm->stack_vm);
    strcpy(process_segments->stack_seg.seg_name, "stack_seg");

    ret = copy_to_user(user_thread_seg, process_segments, sizeof(struct ProcessSegments));


	return ret;

}
# Linux OS 2022 Project 3
**許富皓教授**好人一生平安

第 14 組 吳永璿 李玟卉 翁培馨
## Kernel 與 OS 版本
* Linux Kernel 5.8.1
* Ubuntu 20.04 LTS desktop-amd64
* virtula box 6.1
# Question 1
- (30 points) Write a new system call int get_CPU_number() so that a process can use it to get the number of the CPU that executes it.
```c
//prototype of the new system call is as follows:     
int get_CPU_number()
```
- You need to write a program to prove the effectiveness of your new system call
# Question 1 Solution
## current->cpu
我們發現在`/linux/v5.8.1/source/include/linux/sched.h`中的task_struct定義包含了一個叫做cpu的field。
```c
struct task_struct {
    ...
#ifdef CONFIG_THREAD_INFO_IN_TASK
	/* Current CPU: */
	unsigned int			cpu;
    ...
}
```
我們查看.config，確認到CONFIG_THREAD_INFO_IN_TASK是有開啟的<pre>CONFIG_THREAD_INFO_IN_TASK=y</pre>
因此可以查詢該欄位來得知目前的cpu number。
## 新增之system call
以下是我們新增的syscall:
```c
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(my_get_cpu_number){
    unsigned   _cpu_number_from_task_struct=-1;
    _cpu_number_from_task_struct=current->cpu;/* current CPU */
    if(_cpu_number_from_task_struct==-1){
        return -1;
    }
    return _cpu_number_from_task_struct;
}
```
## getcpu(2)
我們也發現linux內建一個getcpu的方法：
```c
#define _GNU_SOURCE   /* See feature_test_macros(7) */
#include <sched.h>
int getcpu(unsigned int *cpu, unsigned int *node);
```
>The getcpu() system call identifies the processor and node on
which the calling thread or process is currently running and
writes them into the integers pointed to by the cpu and node
arguments.  

因此我們決定用這個system call來驗證我們current->cpu的正確性。
## 測試程式
我們用在user space呼叫我們新增的syscall與getcpu()，發現兩者return之cpu_number一致。
以下是測試程式：
```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#define __NR_my_get_cpu_number 440
int getcpu_default(unsigned *cpu,unsigned *node)
{
        return syscall(SYS_getcpu,cpu,node,NULL);//the 3rd argument for getcpu is depricated since linux 2.6
}


int getcpu_my_call(unsigned *cpu)
{
        return *cpu=syscall(__NR_my_get_cpu_number);
}

int main(void)
{
        unsigned cpu;
        

        if(getcpu_my_call(&cpu)==-1)
        {
                printf("getcpu failed \n");
                return -1;
        }

        printf("cpu = %u (found with task_struct->cpu)\n",cpu);
        /*******/
        unsigned node;

        if(getcpu_default(&cpu,&node)==-1)
        {
                printf("getcpu failed \n");
                return 1;
        }

        printf("cpu = %u node = %u(found with the built in getcpu())\n",cpu,node);

        return 0;
}
```
## 測試結果
這是執行結果:  
由於我配置了4個cpu core給虛擬機，因此cpu_number介於0到3。  
![q1](https://i.imgur.com/hdxQm6T.png)

# Question 2
- (80 points) Write a new system call **void start_to_count_number_of_process_switches()** so that a process can use it to begin to count the number of process switches the process makes. Besides, write another new system call **int stop_to_count_number_of_process_switches()** so that a process can use it to stop to count the number of process switches the process makes and return the number of process switches the process makes.
```c
//prototype of the new system call is as follows:     
void start_to_count_number_of_process_switches()
//prototype of the new system call is as follows:     
int stop_to_count_number_of_process_switches()
```
- You need to write a CPU-bound program and I/O-bound program to counter the number of process switches the programs make.


# Question 2 solution
首先我們來確定題目具體要問甚麼。題目說`count the number of process switches the process makes`。

我們知道process switch發生在process之process state由running轉成ready(把cpu給別人)，或由ready轉running(把cpu搶回來)。如下圖圈圈所示：  
![](https://i.imgur.com/yfo5ECm.png)  
由running轉成ready(把cpu給別人)跟由ready轉running(把cpu搶回來)的數目是一樣的，所以我們只計算由ready轉running(把cpu搶回來)的數目。

## task_struct
由於是要記錄per process之process switch次數資訊，因此我們決定在task_struct新增欄位。
來看看task_struct的code：`/linux-5.8.1/include/linux/sched.h`

我們新增一個`process_switches_count`欄位。
```c
struct task_struct {
    /**略**/      
    unsigned int process_switches_count; // process_switches 次數  
    /*
    * New fields for task_struct should be added above here, so that
    * they are included in the randomized portion of task_struct.
    */
    randomized_struct_fields_end
    /**略**/  
};
```
特別注意在kernel 5.8.1對於在task struct新增欄位的行為有特別規範，必須加在randomized_struct_fields_end之前，而非加在末端。
## __schedule
我們知道躺在run queue中的process會由scheduler排程使用CPU，因此我們開始在Kernel code中尋找scheduler。
我們發現在`/linux-5.8.1/kernel/sched/core.c`中，提到__schedule()包含主要的scheduler邏輯。  
以下節錄__schedule()部分程式邏輯。
```c
static void __sched notrace __schedule(bool preempt)
{
	struct task_struct *prev, *next;
	unsigned long *switch_count;
	unsigned long prev_state;
	struct rq_flags rf;
	struct rq *rq;
	int cpu;

	cpu = smp_processor_id();//cpu number
	rq = cpu_rq(cpu);//run queue
	prev = rq->curr;

	/**中略**/

	if (likely(prev != next)) {/* 如果切換前後 process 不同 */
		rq->nr_switches++; /* run queue switches 數量加一 */
		/*
		 * RCU users of rcu_dereference(rq->curr) may not see
		 * changes to task_struct made by pick_next_task().
		 */
		RCU_INIT_POINTER(rq->curr, next);
		/*
		 * The membarrier system call requires each architecture
		 * to have a full memory barrier after updating
		 * rq->curr, before returning to user-space.
		 *
		 * Here are the schemes providing that barrier on the
		 * various architectures:
		 * - mm ? switch_mm() : mmdrop() for x86, s390, sparc, PowerPC.
		 *   switch_mm() rely on membarrier_arch_switch_mm() on PowerPC.
		 * - finish_lock_switch() for weakly-ordered
		 *   architectures where spin_unlock is a full barrier,
		 * - switch_to() for arm64 (weakly-ordered, spin_unlock
		 *   is a RELEASE barrier),
		 */
		++*switch_count;/* process 切換次數加一 */

		psi_sched_switch(prev, next, !task_on_rq_queued(prev));

		trace_sched_switch(preempt, prev, next);
                //prev->process_switches_count++;//the current process gets swapped out, so add 1 to it's process_switches_count
                next->process_switches_count++;//the next process gets swapped in, so add 1 to it's process_switches_count
		/* Also unlocks the rq: */
		rq = context_switch(rq, prev, next, &rf);
	} else {
		rq->clock_update_flags &= ~(RQCF_ACT_SKIP|RQCF_REQ_SKIP);
		rq_unlock_irq(rq, &rf);
	}

	balance_callback(rq);
}
```
我們看到當scheduler決定要將目前使用cpu之process swap out，會觸發`if(likely(prev != next)){...}`之進入條件。我們更進一步確認到在調用`rq = context_switch(rq, prev, next, &rf);`之前，`prev`與`next`分別對應被swap out之process與即將取得cpu之process descriptor。
因此我們在`rq = context_switch(rq, prev, next, &rf);`之前加入這行，來計算每個process swap out別人的次數:
```c
next->process_switches_count++;//the next process gets swapped in, so add 1 to it's process_switches_count
```
## 新增之system call
1. `void start_to_count_number_of_process_switches()`
```c
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
```
2. `int stop_to_count_number_of_process_switches()`
```c
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

SYSCALL_DEFINE0(stop_to_count_number_of_process_switches){
    
    return current->process_switches_count;
    
}
```
## 測試程式
我們用在user space分別撰寫兩隻測試程式，分別具備I/O bound與cpu bound性質。兩隻程式都會loop兩分鐘，然後計算loop期間發生之process switch。
以下是I/O intensive測試程式：
```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#include <sys/time.h>

#define __NR_start_to_count_number_of_process_switches 441
#define __NR_stop_to_count_number_of_process_switches 442
#define ON  1
#define OFF 0



void main()
    {             
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("This program is I/O intensive!\n");
    /*******/
    int a,b;
    a=0;
    b=0;
    int   _switch=ON;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int start_time_in_seconds=tv.tv_sec;                                                   
    int current_time_in_seconds;
    printf("start_time_in_seconds: %d\n", start_time_in_seconds);
    
    syscall(__NR_start_to_count_number_of_process_switches);
    while(_switch==ON)
    {
                                
        sleep(0.01);
        printf("[%d ]",b++);

        gettimeofday(&tv,NULL);
        current_time_in_seconds=tv.tv_sec;     
        if ((current_time_in_seconds-start_time_in_seconds)>=120){_switch=OFF;            }
                                
    }
    a=syscall(__NR_stop_to_count_number_of_process_switches);
    printf("\nDuring the past 2 minutes the process made %d times process switches.\n",a);
}
```
以下是cpu intensive測試程式：
```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#include <sys/time.h>

#define __NR_start_to_count_number_of_process_switches 441
#define __NR_stop_to_count_number_of_process_switches 442
#define ON  1
#define OFF 0
                   
void main()
{            
    printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
    printf("This program is cpu intensive!\n");
    /******/
    int a;
    int  _switch=ON;
    float   b=0;
                            
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int start_time_in_seconds=tv.tv_sec;                                                   
    int current_time_in_seconds;
    printf("start_time_in_seconds: %d\n", start_time_in_seconds);
    
    syscall(__NR_start_to_count_number_of_process_switches);
    while(_switch==ON)
    {
                 
        b=b+1;
                
        gettimeofday(&tv,NULL);
        current_time_in_seconds=tv.tv_sec;     
        if ((current_time_in_seconds-start_time_in_seconds)>=120){_switch=OFF;            }
    }
    a=syscall(__NR_stop_to_count_number_of_process_switches);
    printf("During the past 2 minutes the process made %d times process switches.\n",a);
 }
```
## 測試結果
我們觀察到CPU intensive program之context switch次數少於I/O intensive program。
I/O intensive program:  
![](https://i.imgur.com/UGGhE49.png)  

CPU intensive program:  
![](https://i.imgur.com/Ek1kmu2.png)
# 參考資料
1. https://frankjkl.github.io/2019/03/09/Linux%E5%86%85%E6%A0%B8-smp_processor_id/
2. https://man7.org/linux/man-pages/man2/getcpu.2.html
3. https://elixir.bootlin.com/linux/v5.8.1/source/arch/alpha/include/asm/thread_info.h#L15
4. https://elixir.bootlin.com/linux/v5.8.1/source/include/linux/sched.h#L629
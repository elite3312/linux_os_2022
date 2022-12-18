# Linux OS 2022 Project 1
好課值得一修再修

第 14 組 吳永璿 李玟卉 翁培馨

# Goal
給定一個多線程c程式，撰寫新的system call來達成以下目標：
* 查看不同的 thread 會共用哪些 segment 
* 加分 : 找出每個 thread 的 segment size、start address、end address

# Kernel 與 OS 版本
* Linux Kernel 5.8.1
* Ubuntu 20.04 LTS desktop-amd64
* virtula box 6.1
-- 4 cores
-- 12G RAM(我們測過給滿64G RAM編kernel會當掉)
# Kernel 編譯/新增System call的過程所遇到的問題

我們主要參考這兩篇教學的步驟:
-- https://dev.to/jasper/adding-a-system-call-to-the-linux-kernel-5-8-1-in-ubuntu-20-04-lts-2ga8
-- https://blog.kaibro.tw/2016/11/07/Linux-Kernel%E7%B7%A8%E8%AD%AF-Ubuntu/

但有加入幾個步驟修正make時遇到的問題：
* No rule to make target ‘debian/canonical-certs.pem‘, needed by ‘certs/x509_certificate_list‘
解法:编辑.config文件，修改CONFIG_SYSTEM_TRUSTED_KEYS，将其赋空值。
參考資料:https://blog.csdn.net/qq_36393978/article/details/118157426
* Failed to generate BTF for vmlinux
解法:编辑.config文件，修改CONFIG_DEBUG_INFO=n
參考資料:無
* grub選單沒出現
解法:編輯grub 
    vim /etc/default/grub
    ```shell
    GRUB_DEFAULT=0
    GRUB_TIMEOUT_STYLE=menu
    GRUB_TIMEOUT=15
    ```
    參考資料:https://docs.vmware.com/en/VMware-NSX-T-Data-Center/3.2/installation/GUID-4630C9D5-71FB-4991-AC1D-9FDBA0B86120.html
*  CONFIG_X86_X32 enabled but no binutils support
解法: 編輯.config文件，修改`CONFIG_X86_X32 = n` 或安裝binutils `sudo apt-get install binutilsg`
參考資料: https://blog.csdn.net/m0_48958478/article/details/121620449

# 新增的System call
我們共新增兩隻System call
* my_other_get_phy_addr.c
我們使用Kernel 5.8.1。
我們這裡有一個小bug：我們原先以為該版本支援5-level page table來管理 virtual memory，所以system call將依序從 pgd、p4d、pud、pmd、pte進行查表。
但實際上若沒有在編譯kernel時勾選5-level page tabel的選項，預設仍會採用4-level page tables。
在kernel中定義有 pgtable-nop4d.h，在針對 p4d 進行查表的過程中，會利用這裡定義的變數及相關函式，使得查找 p4d 的結果回傳為與 pgd 相同的數值。
以下為pgtable-nop4d.h節錄之程式碼：
```c=
#define pgd_page(pgd)				(p4d_page((p4d_t){ pgd }))
```
我們錯誤但仍正確運行之system call：
```c=
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/init_task.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(my_other_get_phy_addr, unsigned long *, initial,
		unsigned long *, result)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	unsigned long padder = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;
	//printk("Here1");
	unsigned long *vir_adds =
		kmalloc(1 * sizeof(unsigned long),
			GFP_KERNEL); //need to use vmalloc in kernel
	unsigned long *phy_adds =
		kmalloc(1 * sizeof(unsigned long), GFP_KERNEL);

	//copy from user
	//initial need to change type?? ->not need
	//printk("Here2");
	unsigned long a =
		copy_from_user(vir_adds, initial, 1 * sizeof(unsigned long)); //
	printk("%lu", a);

	int i = 0;
	//Input virtual addresses //*(intial+i) //
	

	pgd = pgd_offset(current->mm, *(vir_adds + i));
	printk("pgd_val = 0x%lx\n", pgd_val(*pgd));
	printk("pgd_index = %lu\n", pgd_index(*(vir_adds + i)));
	if (pgd_none(*pgd)) {
		printk("not mapped in pgdn");
	}

	p4d = p4d_offset(pgd, *(vir_adds + i));
	printk("p4d_val = 0x%lx\n", p4d_val(*p4d));
	printk("p4d_index = %lu\n", p4d_index(*(vir_adds + i)));
	if (p4d_none(*p4d)) {
		printk("not mapped in p4d");
	}

	pud = pud_offset(p4d, *(vir_adds + i));
	printk("pud_val = 0x%lx\n", pud_val(*pud));
	printk("pud_index = %lu\n", pud_index(*(vir_adds + i)));
	if (pud_none(*pud)) {
		printk("not mapped in pudn");
	}

	pmd = pmd_offset(pud, *(vir_adds + i));
	printk("pmd_val = 0x%lx\n", pmd_val(*pmd));
	printk("pmd_index = %lu\n", pmd_index(*(vir_adds + i)));
	if (pmd_none(*pmd)) {
		printk("not mapped in pmdn");
	}

	pte = pte_offset_kernel(pmd, *(vir_adds + i));
	printk("pte_val = 0x%lx\n", pte_val(*pte));
	printk("pte_index = %lu\n", pte_index(*(vir_adds + i)));
	if (pte_none(*pte)) {
		printk("not mapped in pten");
	}

	
	page_addr = pte_val(*pte) & PAGE_MASK; 
	page_offset = *(vir_adds + i) & ~PAGE_MASK;

	//physical address
	*(phy_adds + i) = page_addr | page_offset;

	printk("page_addr = %lx\n", page_addr);
	printk("page_offset = %lx\n", page_offset);

	printk("vaddr =%lx, paddr = %lx\n", *(vir_adds + i), *(phy_adds + i));

	//Use copytouser -> user space
	copy_to_user(result, phy_adds, 1 * sizeof(unsigned long));
	kfree(vir_adds);
	kfree(phy_adds);

	return 0;
}
```
* my_get_segment.c
首先藉由find_task_by_vpid找出目前執行process的task_struct。
接著存取該task_struct底下的mm_structmanagement相關的mm_struct。
mm_struct 描述了一個process的整個虛擬地址空間，包含各segment的起始結束位址。

補充一點:如果直接從current查詢mm的話，就必須保證current指向的是我們要查詢的thread的task struct，造成syscall使用上的限制。
``` c=
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
```
# Test code

以下為user space的測試程式。包含main與2隻thread。每個thread會印出本身使用的各segment虛擬與實體記憶體位址，包含:
-- text segment
-- data segment (global variables with initial values)
-- BSS segment (global variables without initial values)
-- heap segment (memory area allocated through function malloc())
-- libraries
-- stack segment
* test_multi_thread.c
``` c=
/*gcc -o test_multi_thread.out -pthread test_multi_thread.c */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#define __NR_get_segments 443
#define __NR_get_physical_addresses 442
#define MAX_BUF_SIZE 128
int bss_value;
int data_value = 123;
int code_function()
{
       return 0;
}
static __thread int thread_local_storage_value = 246;

struct Segment
{
       unsigned long start_addr;
       unsigned long end_addr;
       char seg_name[MAX_BUF_SIZE];
};

struct ProcessSegments
{
       pid_t pid;
       struct Segment code_seg;
       struct Segment data_seg;
       struct Segment heap_seg;
       struct Segment stack_seg;
};
unsigned long get_phys_addr(unsigned long virtual_address)
{
       //呼叫轉址的system call
       unsigned long physical_address;

       syscall(__NR_get_physical_addresses, &virtual_address, &physical_address);

       return physical_address;
}
void *thread1(void *arg)
{
       sleep(2);
       int stack_value = 100;
       unsigned long TLS = (unsigned long)&thread_local_storage_value;
       unsigned long stack = (unsigned long)&stack_value;
       unsigned long lib = (unsigned long)getpid;
       unsigned long heap = (unsigned long)malloc(sizeof(int));
       unsigned long bss = (unsigned long)&bss_value;
       unsigned long data = (unsigned long)&data_value;
       unsigned long code = (unsigned long)code_function;

       int len = 7;
       unsigned long vir_addrs[7] = {TLS, stack, lib, heap, bss, data, code};
       unsigned long phy_addrs[7];

       printf("============= thread1 =============\n");
       printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
       printf("segment\tvir_addr\tphy_addr\n");
       printf("TLS\t%lx\t%lx\n", vir_addrs[0], get_phys_addr(vir_addrs[0]));
       printf("stack\t%lx\t%lx\n", vir_addrs[1], get_phys_addr(vir_addrs[1]));
       printf("lib\t%lx\t%lx\n", vir_addrs[2], get_phys_addr(vir_addrs[2]));
       printf("heap\t%lx\t%lx\n", vir_addrs[3], get_phys_addr(vir_addrs[3]));
       printf("bss\t%lx\t%lx\n", vir_addrs[4], get_phys_addr(vir_addrs[4]));
       printf("data\t%lx\t%lx\n", vir_addrs[5], get_phys_addr(vir_addrs[5]));
       printf("code\t%lx\t%lx\n", vir_addrs[6], get_phys_addr(vir_addrs[6]));

       printf("\n=== finding the start, end address and size for this thread segment=== \n");
       struct ProcessSegments thread_segs;
       int tid = 0;
       // tid = syscall(__NR_gettid);(int)gettid())
       tid = (int)gettid();
       thread_segs.pid = tid;
       // get segments //把上面那個get_segments 寫進system call
       syscall(__NR_get_segments, (void *)&thread_segs);
       //code seg
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in words)\n");
       unsigned long _segment_start_addr = get_phys_addr(thread_segs.code_seg.start_addr);
       unsigned long _segment_end_addr = get_phys_addr(thread_segs.code_seg.end_addr);
       unsigned long _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.code_seg.seg_name,
              thread_segs.code_seg.start_addr,
              thread_segs.code_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //data seg
       _segment_start_addr = get_phys_addr(thread_segs.data_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.data_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.data_seg.seg_name,
              thread_segs.data_seg.start_addr,
              thread_segs.data_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //heap_seg
       _segment_start_addr = get_phys_addr(thread_segs.heap_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.heap_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.heap_seg.seg_name,
              thread_segs.heap_seg.start_addr,
              thread_segs.heap_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //stack_seg
       _segment_start_addr = get_phys_addr(thread_segs.stack_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.stack_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.stack_seg.seg_name,
              thread_segs.stack_seg.start_addr,
              thread_segs.stack_seg.end_addr,
               _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       pthread_exit(NULL); // 離開子執行緒
}

void *thread2(void *arg)
{
       sleep(4);
       int stack_value = 200;
       unsigned long TLS = (unsigned long)&thread_local_storage_value;
       unsigned long stack = (unsigned long)&stack_value;
       unsigned long lib = (unsigned long)getpid;
       unsigned long heap = (unsigned long)malloc(sizeof(int));
       unsigned long bss = (unsigned long)&bss_value;
       unsigned long data = (unsigned long)&data_value;
       unsigned long code = (unsigned long)code_function;
       int len = 7;
       unsigned long vir_addrs[7] = {TLS, stack, lib, heap, bss, data, code};
       unsigned long phy_addrs[7];

       printf("============= thread2 =============\n");
       printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
       printf("segment\tvir_addr\tphy_addr\n");
       printf("TLS\t%lx\t%lx\n", vir_addrs[0], get_phys_addr(vir_addrs[0]));
       printf("stack\t%lx\t%lx\n", vir_addrs[1], get_phys_addr(vir_addrs[1]));
       printf("lib\t%lx\t%lx\n", vir_addrs[2], get_phys_addr(vir_addrs[2]));
       printf("heap\t%lx\t%lx\n", vir_addrs[3], get_phys_addr(vir_addrs[3]));
       printf("bss\t%lx\t%lx\n", vir_addrs[4], get_phys_addr(vir_addrs[4]));
       printf("data\t%lx\t%lx\n", vir_addrs[5], get_phys_addr(vir_addrs[5]));
       printf("code\t%lx\t%lx\n", vir_addrs[6], get_phys_addr(vir_addrs[6]));

       printf("\n=== finding the start, end address and size for this thread segment=== \n");
       struct ProcessSegments thread_segs;
       int tid = 0;
       // tid = syscall(__NR_gettid);(int)gettid())
       tid = (int)gettid();
       thread_segs.pid = tid;
       // get segments //把上面那個get_segments 寫進system call
       syscall(__NR_get_segments, (void *)&thread_segs);
       //code seg
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in words)\n");
       unsigned long _segment_start_addr = get_phys_addr(thread_segs.code_seg.start_addr);
       unsigned long _segment_end_addr = get_phys_addr(thread_segs.code_seg.end_addr);
       unsigned long _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.code_seg.seg_name,
              thread_segs.code_seg.start_addr,
              thread_segs.code_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //data seg
       _segment_start_addr = get_phys_addr(thread_segs.data_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.data_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.data_seg.seg_name,
              thread_segs.data_seg.start_addr,
              thread_segs.data_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //heap_seg
       _segment_start_addr = get_phys_addr(thread_segs.heap_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.heap_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.heap_seg.seg_name,
              thread_segs.heap_seg.start_addr,
              thread_segs.heap_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //stack_seg
       _segment_start_addr = get_phys_addr(thread_segs.stack_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.stack_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.stack_seg.seg_name,
              thread_segs.stack_seg.start_addr,
              thread_segs.stack_seg.end_addr,
               _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       pthread_exit(NULL); // 離開子執行緒
}

// 主程式
int main()
{
       pthread_t t1, t2;

       printf("syscall\n"); // Syscall

       pthread_create(&t1, NULL, thread1, NULL);
       pthread_create(&t2, NULL, thread2, NULL);

       sleep(8);
       int stack_value = 10;
       unsigned long TLS = (unsigned long)&thread_local_storage_value;
       unsigned long stack = (unsigned long)&stack_value;
       unsigned long lib = (unsigned long)getpid;
       unsigned long heap = (unsigned long)malloc(sizeof(int));
       unsigned long bss = (unsigned long)&bss_value;
       unsigned long data = (unsigned long)&data_value;
       unsigned long code = (unsigned long)code_function;

       int len = 7;
       unsigned long vir_addrs[7] = {TLS, stack, lib, heap, bss, data, code};
       unsigned long phy_addrs[7];

       printf("============= main =============\n");
       printf("pid = %d  tid = %d\n", (int)getpid(), (int)gettid());
       printf("segment\tvir_addr\tphy_addr\n");
       printf("TLS\t%lx\t%lx\n", vir_addrs[0], get_phys_addr(vir_addrs[0]));
       printf("stack\t%lx\t%lx\n", vir_addrs[1], get_phys_addr(vir_addrs[1]));
       printf("lib\t%lx\t%lx\n", vir_addrs[2], get_phys_addr(vir_addrs[2]));
       printf("heap\t%lx\t%lx\n", vir_addrs[3], get_phys_addr(vir_addrs[3]));
       printf("bss\t%lx\t%lx\n", vir_addrs[4], get_phys_addr(vir_addrs[4]));
       printf("data\t%lx\t%lx\n", vir_addrs[5], get_phys_addr(vir_addrs[5]));
       printf("code\t%lx\t%lx\n", vir_addrs[6], get_phys_addr(vir_addrs[6]));
       
       printf("\n=== finding the start, end address and size for this thread segment=== \n");
       struct ProcessSegments thread_segs;
       int tid = 0;
       // tid = syscall(__NR_gettid);(int)gettid())
       tid = (int)gettid();
       thread_segs.pid = tid;
       // get segments //把上面那個get_segments 寫進system call
       syscall(__NR_get_segments, (void *)&thread_segs);
       //code seg
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in words)\n");
       unsigned long _segment_start_addr = get_phys_addr(thread_segs.code_seg.start_addr);
       unsigned long _segment_end_addr = get_phys_addr(thread_segs.code_seg.end_addr);
       unsigned long _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.code_seg.seg_name,
              thread_segs.code_seg.start_addr,
              thread_segs.code_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //data seg
       _segment_start_addr = get_phys_addr(thread_segs.data_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.data_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.data_seg.seg_name,
              thread_segs.data_seg.start_addr,
              thread_segs.data_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //heap_seg
       _segment_start_addr = get_phys_addr(thread_segs.heap_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.heap_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t\t\t%u\n",
              thread_segs.heap_seg.seg_name,
              thread_segs.heap_seg.start_addr,
              thread_segs.heap_seg.end_addr,
              _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       //stack_seg
       _segment_start_addr = get_phys_addr(thread_segs.stack_seg.start_addr);
       _segment_end_addr = get_phys_addr(thread_segs.stack_seg.end_addr);
       _segment_size_in_bytes = _segment_start_addr - _segment_end_addr;
       printf(" %s:\t%lx-%lx\t(%lx-%lx)\t%u\n",
              thread_segs.stack_seg.seg_name,
              thread_segs.stack_seg.start_addr,
              thread_segs.stack_seg.end_addr,
               _segment_start_addr, _segment_end_addr,_segment_size_in_bytes);
       printf("----------- thread address ------------\n");
       printf("t1 = %p\n", &t1);
       printf("t2 = %p\n", &t2);

       return 0;
}
```
# Result
- 我們建立了3個thread(main Thread 、 Thread1、Thread2)，查看他們之間共用的情況，有共用的部分我們使用綠色將其標示出來。
- 根據第一張表格的實驗結果，Thread 之間共用 Lib、Bss、Data、Code 段實體位置，不共用 Stack、Heap 段實體位置。
- 我們也找出了每個segment的實體記憶體起始與結束位址。我們發現每個thread回傳的起始結束位址都是相同的，而且都是以main為主。這可能是我們在get_segment的system call中沒有正確查詢到thread的process descriptor，或是process descriptor只記錄main process的資訊。
<table>
    <tr>
        <th></th><th>segment</th><th>Main Thread</th><th>Thread1</th><th>Thread2</th>
    </tr>
    <!--Code--->
    <tr>
        <td rowspan="2">Code</td><td>Virtual</td><td style="background-color:rgb(118, 238, 198)">565402a42259</td><td style="background-color:rgb(118, 238, 198)">565402a42259</td><td style="background-color:rgb(118, 238, 198)">565402a42259</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">3bd4cc259</td><td style="background-color:rgb(118, 238, 198)">3bd4cc259</td><td style="background-color:rgb(118, 238, 198)">3bd4cc259</td>
    </tr>
    <!--Data--->
    <tr>
        <td rowspan="2">Data</td><td >Virtual</td><td style="background-color:rgb(118, 238, 198)">565402a46010</td><td style="background-color:rgb(118, 238, 198)">565402a46010</td><td style="background-color:rgb(118, 238, 198)">565402a46010</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e010</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e010</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e010</td>
    </tr>
    <!--BSS--->
    <tr>
        <td rowspan="2">BSS</td><td>Virtual</td><td style="background-color:rgb(118, 238, 198)">565402a46018</td><td style="background-color:rgb(118, 238, 198)">565402a46018</td><td style="background-color:rgb(118, 238, 198)">565402a46018</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e018</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e018</td><td style="background-color:rgb(118, 238, 198)">8000000354a9e018</td>
    </tr>
    <!--Heap--->
    <tr>
        <td rowspan="2">Heap</td><td>Virtual</td><td>565402f13910</td><td>7f4ef4000b60</td><td>7f4ef4001270</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000035709e910</td><td>8000000356ae2b60</td><td>8000000356ae3270</td>
    </tr>
    <!--Library--->
    <tr>
        <td rowspan="2">Library</td><td>Virtual</td><td style="background-color:rgb(118, 238, 198)">7f4ef9cae0c0</td><td style="background-color:rgb(118, 238, 198)">7f4ef9cae0c0</td><td style="background-color:rgb(118, 238, 198)">7f4ef9cae0c0</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">18d3a60c0</td><td style="background-color:rgb(118, 238, 198)">18d3a60c0</td><td style="background-color:rgb(118, 238, 198)">18d3a60c0</td>
    </tr>
    <!--Stack--->
    <tr>
        <td rowspan="2">Stack</td><td>Virtual</td><td>7ffc8c1cb154</td><td>7f4ef9bc5bf4</td><td>7f4ef93c4bf4</td>
    </tr>
    <tr>
        <td>Physical</td><td>80000003666b6154</td><td>8000000379c43bf4</td><td>8000000360445bf4</td>
    </tr>
    <!--TLS--->
    <tr>
        <td rowspan="2">TLS</td><td>Virtual</td><td>7f4ef9bc773c</td><td>7f4ef9bc66fc</td><td>7f4ef93c56fc</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000035e59673c</td><td>80000003569e36fc</td><td>80000003716b46fc</td>
    </tr>
    

</table>

<table>
    <tr>
        <th></th><th></th><th>start address</th><th>end address</th><th>segment size (bytes)</th>
    </tr>
    <!--Code--->
    <tr>
        <td rowspan="2">Code</td><td>Virtual</td><td>565402a42000</td><td>565402a433a5</td><td rowspan="2">4294954075</td>
    </tr>
    <tr>
        <td>Physical</td><td>3bd4cc000</td><td>3bd4cf3a5</td>
    </tr
    <!--Data--->
    <tr>
        <td rowspan="2">Data</td><td>Virtual</td><td>565402a45d5c</td><td>565402a46014</td><td rowspan="2">686001480</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000037d8d6d5c</td><td>8000000354a9e014</td>
    </tr>
    <!--Heap--->
    <tr>
        <td rowspan="2">Heap</td><td>Virtual</td><td>565402f13000</td><td>565402f34000</td><td rowspan="2">1460264960</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000035789e000</td><td>0</td>
    </tr>
    <!--Stack--->
    <tr>
        <td rowspan="2">Stack</td><td>Virtual</td><td>7ffc8c1cb571</td><td>7ffc8c1cb551</td><td rowspan="2">4294967263</td>
    </tr>
    <tr>
        <td>Physical</td><td>80000003666b6550</td><td>80000003666b6571</td>
    </tr>
</table>

![](https://i.imgur.com/y37u46w.png)


# References
![](https://i.imgur.com/mysO7U5.jpg)
![](https://i.imgur.com/w7D66Ad.png)

+ task_struct
https://blog.csdn.net/qq_26768741/article/details/54348586

+ mm_struct
https://blog.csdn.net/qq_26768741/article/details/54375524

![](https://i.imgur.com/J3SWvlA.png)


+ v to p && pgd pud pmd pte 簡單說明
https://www.796t.com/content/1537503608.html


+ virtual address 轉成 physical address
> 由上圖可以知道要找到對應的physical address
> 目標 : 
> 
> 找到Page table的位置
> 
> 作法:
>  
> 從task_struct //current指標能取得目前執行process的task_struct
> 
> 獲得該process的mm_point , mm將會指向mm_struct//而current->mm則會取得跟這個process memory management相關的mm_struct
> 
> mm_struct 描述了一個process的整個虛擬地址空間，所以可以利用此點找到pgd的起始位置
> 
> PGD每個條目中指向一個P4D,P4D的每個條目指向一個PUD,PUD的每個條目指向一個PMD,PMD的每個條目指向一個PTE,PTE的每個條目指向一個頁面(Page)的物理首地址
> 
> 最後可得到pte 中的實體位置再將後面的offset清成0(使用PAGE_MASK)，接著加上真正的offset可得到正確的實體位置。
>
>一樣是long unsigned


# QA
補充一些demo時被助教問倒的問題
+ p4d 實際上跟pgd_offset拿到一樣的東西，因為預設還是4層 - wen
>下圖為查詢某 segment 的 address，可以發現 p4d 的值與 pgd 的值相同
![](https://i.imgur.com/qkYn309.png)
>原本猜測可能為預設未開 5-level page tables support，但打開設定發現已有設為 5-level paging
![](https://i.imgur.com/tv37VoU.png)

>根據找到的資料說法: On 64-bit x86, it's usually 4 levels, but support for 5 got merged recently (however, only really high-end Intel server processors support this, and only very recent ones).
參考資料:
https://unix.stackexchange.com/questions/379230/how-many-page-table-levels-does-linux-kernel-use-4-or-5


+ 為甚麼pgd = pgd_offset(current->mm...參數要帶current?current 底層做什麼? - perry
> 根據[這篇參考資料](https://stackoverflow.com/questions/16975393/current-mm-gives-null-in-linux-kernel)，在linux kernel system call的source code看見current，都可以理解成"一個指標，指向user space發出system call的process"，且它的型態是struct task_struct.。mm底下有一個指標指向頁全局目錄，可以查詢相應表項的線性地址。


+ 請解釋Syscall_Define這個macro做了啥，為啥不用Asmlinkage define? - wen
>asmlinkage 是一個 gcc 標籤，在函式定義前面加上這個標籤，表示 C function 會由 stack 取參數，而不是從 register 取參數。
參考資料:
https://www.jollen.org/blog/2006/10/_asmlinkage.html

>SYSCALL_DEFINE macro 是定義系統呼叫的標準方法，其結尾 n 代表引數（argument）的個數，第一個參數為系統呼叫的名字，後面接2n個參數，每一對指系統呼叫的參數類型及名字。
>主要是將系統呼叫的參數統一變成使用 long 的形式來接收，再強轉爲 int 的形式，原因是避免以前 64 位元 Linux 存在的 CVE-2009-2009 漏洞，確保對於 64 位元的 Kernel 平台，32 位元的值會有正確的符號擴展（sign-extended）。
>參考資料:
>https://blog.csdn.net/hxmhyp/article/details/22699669
>https://blog.csdn.net/hazir/article/details/11835025

+ 為甚麼要用copy_from_user?在user space發生page fault會怎樣?(會交給kernel，確定這個記憶體是不是存在)。 -Amy

> 直接訪問的話，無法保證被訪問的用戶態虛擬地址是否有對應的頁表項，即無法保證該虛擬地址已經分配了相應的物理內存，如果此時沒有對應的頁表項，那麼此時將產生page fault
而由於內核空間和用戶空間是不能互相訪問的，如果需要訪問就必須藉助內核函數進行數據讀寫。copy_to_user():完成內核空間到用戶空間的複制，copy_from_user()：是完成用戶空間到內核空間的複制

> 發生page fault後，Linux核心可以處理Page Fault問題。
會去查「一個記憶體分配表格」，看看這個記憶體位址是否合法。
合法➣找到一個空的frame，從硬碟載入該page到RAM，修改page table，invalid=>valid
非法➣segmentation fault


+ pthread_create做了啥 - Amy

>pthread_create 
>
>函數可以用來建立新的執行緒，並以函數指標指定子執行緒所要執行的函數，子執行緒在建立之後，就會以平行的方式執行，在子執行緒的執行期間，主執行緒還是可以正常執行自己的工作，最後主執行緒再以 
>
>pthread_join 
>
>函數等待子執行緒執行結束，處理後續收尾的動作。

+ 加分題我們為甚麼不直接用current去查?current底下也會有mm，也就有start_code那些(? -perry
> 如果直接從current查詢mm的話，就必須保證current指向的是我們要查詢的thread的task struct，造成syscall使用上的限制。使用find_task_by_vpid查詢task struct的話可以支援較廣泛的應用情境。
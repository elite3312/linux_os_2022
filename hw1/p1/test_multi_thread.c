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
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in bytes)\n");
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
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in bytes)\n");
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
       printf(" segname\tvir_start  -  vir_end\t\t(phy_start  -  phy_end)\t\tsegment size(in bytes)\n");
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
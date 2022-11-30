# Linux OS 2022 Project 2
**許富皓教授**好人一生平安

第 14 組 吳永璿 李玟卉 翁培馨
Project 1 可以參考[這篇](https://hackmd.io/MBn8nRiLQFy66B0L_RFevg?view#Result)
# Goal
- Write a program using the system call you wrote in Project 1 to check how memory areas are shared by two processes that execute this program simultaneously.
- The memory areas include code segments, data segments, BSS segments, heap segments, libraries, stack segments.
- Hint
    - When making your check, both related processes must be in progress. Hence you may need to use function sleep() to guarantee this requirement.
    - Inside the Linux kernel, you need to use function copy_from_user() and function copy_to_user() to copy data from/to a user address buffer.
# Kernel 與 OS 版本
* Linux Kernel 5.8.1
* Ubuntu 20.04 LTS desktop-amd64
* virtula box 6.1
# 使用的System call
* my_other_get_phy_addr.c
```cpp=
SYSCALL_DEFINE2(my_other_get_phy_addr, unsigned long *, initial,
		unsigned long *, result){...}
```
Usage: Given a virtual_address, return the corresponding physical address.
```cpp
    syscall(__NR_get_physical_addresses, &virtual_address, &physical_address);
```
* my_get_segment.c
使用find_task_by_vpid查詢到user space process的pid對應到的task_struct，並查詢code seg, data_seg, heap_seg, stack_seg之實體記憶體起始與結束位址。
```cpp
SYSCALL_DEFINE1(my_get_segment, void* __user, user_thread_seg) {...}
```
usage: 宣告一ProcessSegments物件來接收前述記憶體區段之起始與結束位址查詢結果。
```cpp
struct ProcessSegments
{
       pid_t pid;
       struct Segment code_seg;
       struct Segment data_seg;
       struct Segment heap_seg;
       struct Segment stack_seg;
};
...
struct ProcessSegments thread_segs;
int tid = 0;
tid = (int)gettid();
thread_segs.pid = tid;
syscall(__NR_get_segments, (void *)&thread_segs);
```
# Test code
以下為user space的測試程式。包含main，會印出本身使用的各segment虛擬與實體記憶體位址，包含:
-- text segment
-- data segment (global variables with initial values)
-- BSS segment (global variables without initial values)
-- heap segment (memory area allocated through function malloc())
-- libraries
-- stack segment
```cpp
/*gcc -o test_single_thread.out -pthread test_single_thread.c */
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

// 主程式
int main()
{
       
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

       printf("\n============= main =============\n");
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
       
       getchar();

       return 0;
}
```
# Results
同時開兩個terminal，都執行test_single_thread.out。
觀察兩process印出的結果，可以看到lib, code segment具有相同的physical address。
另外，兩隻process的stack區段大小相同，而heap、data區段的大小不同。
- process 1(pid = 5714  tid = 5714)
```bash=
perry@perry-VirtualBox:~/Desktop/linux_os_2022/hw2$ ./test_single_thread.out 
============= main =============
pid = 5714  tid = 5714
segment vir_addr        phy_addr
TLS     7f36443fc57c    8000000339dd557c
stack   7ffd2ee044b4    800000032c1624b4
lib     7f36442ed0c0    2ae9280c0
heap    5563104742a0    80000003757592a0
bss     55631029f018    8000000334dd1018
data    55631029f010    8000000334dd1010
code    55631029c219    37da3a219

=== finding the start, end address and size for this thread segment=== 
 segname        vir_start  -  vir_end           (phy_start  -  phy_end)                 segment size(in bytes)
 code_seg:      55631029c000-55631029c855       (37da3a000-37da3a855)                   4294965163
 data_seg:      55631029ed7c-55631029f014       (8000000326016d7c-8000000334dd1014)     4045692264
 heap_seg:      556310474000-556310495000       (8000000375759000-0)                    1970638848
 stack_seg:     7ffd2ee048a0-7ffd2ee048c1       (800000032c1628a0-800000032c1628c1)     4294967263
```
- process 2(pid = 5730  tid = 5730)
```bash=
perry@perry-VirtualBox:~/Desktop/linux_os_2022/hw2$ ./test_single_thread.out 

============= main =============
pid = 5730  tid = 5730
segment vir_addr        phy_addr
TLS     7f419072b57c    800000037423657c
stack   7fff6aecad94    8000000372c2bd94
lib     7f419061c0c0    2ae9280c0
heap    564ee49762a0    800000036953d2a0
bss     564ee332f018    800000035a7a4018
data    564ee332f010    800000035a7a4010
code    564ee332c219    37da3a219

=== finding the start, end address and size for this thread segment=== 
 segname        vir_start  -  vir_end           (phy_start  -  phy_end)         segment size(in bytes)
 code_seg:      564ee332c000-564ee332c855       (37da3a000-37da3a855)                   4294965163
 data_seg:      564ee332ed7c-564ee332f014       (800000034d599d7c-800000035a7a4014)     4074724712
 heap_seg:      564ee4976000-564ee4997000       (800000036953d000-0)                    1767100416
 stack_seg:     7fff6aecb180-7fff6aecb1a1       (800000034a60e180-800000034a60e1a1)     4294967263
```
<table>
    <tr>
        <th>Segment</th><th>vir or phy</th><th>Process 1</th><th>Process 2</th>
    </tr>
    <!--Code--->
    <tr>
        <td rowspan="2">Code</td><td>Virtual</td><td >55631029c219</td><td >564ee332c219</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">37da3a219</td><td style="background-color:rgb(118, 238, 198)">37da3a219</td>
    </tr>
    <!--Data--->
    <tr>
        <td rowspan="2">Data</td><td >Virtual</td><td >55631029f010</td><td >564ee332f010</td>
    </tr>
    <tr>
        <td>Physical</td><td >8000000334dd1010</td><td>800000035a7a4010</td>
    </tr>
    <!--BSS--->
    <tr>
        <td rowspan="2">BSS</td><td>Virtual</td><td >55631029f018</td><td >564ee332f018</td>
    </tr>
    <tr>
        <td>Physical</td><td >8000000334dd1018</td><td >800000035a7a4018</td>
    </tr>
    <!--Heap--->
    <tr>
        <td rowspan="2">Heap</td><td>Virtual</td><td>5563104742a0</td><td>564ee49762a0</td>
    </tr>
    <tr>
        <td>Physical</td><td>80000003757592a0</td><td>800000036953d2a0</td>
    </tr>
    <!--Library--->
    <tr>
        <td rowspan="2">Library</td><td>Virtual</td><td >7f36442ed0c0</td><td >7f419061c0c0</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">2ae9280c0</td><td style="background-color:rgb(118, 238, 198)">2ae9280c0</td>
    </tr>
    <!--Stack--->
    <tr>
        <td rowspan="2">Stack</td><td>Virtual</td><td>7ffd2ee044b4</td><td>7fff6aecad94</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000032c1624b4</td><td>8000000372c2bd94</td>
    </tr>
    <!--TLS--->
    <tr>
        <td rowspan="2">TLS</td><td>Virtual</td><td>7f36443fc57c</td><td>7f419072b57c</td>
    </tr>
    <tr>
        <td>Physical</td><td>8000000339dd557c</td><td>800000037423657c</td>
    </tr>
    

</table>
<table>
    <tr>
        <th>process 1</th><th>vir or phy</th><th>start address</th><th>end address</th><th>segment size (bytes)</th>
    </tr>
    <!--Code--->
    <tr>
        <td rowspan="2">Code</td><td>Virtual</td><td>55631029c000</td><td>55631029c855</td><td rowspan="2" style="background-color:rgb(118, 238, 198)">4294965163</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">37da3a000</td><td style="background-color:rgb(118, 238, 198)">37da3a855</td>
    </tr
    <!--Data--->
    <tr>
        <td rowspan="2">Data</td><td>Virtual</td><td>55631029ed7c</td><td>55631029f014</td><td rowspan="2">4045692264</td>
    </tr>
    <tr>
        <td>Physical</td><td>8000000326016d7c</td><td>8000000334dd1014</td>
    </tr>
    <!--Heap--->
    <tr>
        <td rowspan="2">Heap</td><td>Virtual</td><td>556310474000</td><td>556310495000</td><td rowspan="2">1970638848</td>
    </tr>
    <tr>
        <td>Physical</td><td>8000000375759000</td><td>0</td>
    </tr>
    <!--Stack--->
    <tr>
        <td rowspan="2">Stack</td><td>Virtual</td><td>7ffd2ee048a0</td><td>7ffd2ee048c1</td><td rowspan="2" style="background-color:rgb(118, 238, 198)">4294967263</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000032c1628a0</td><td>800000032c1628c1</td>
    </tr>
</table>

<table>
    <tr>
        <th>process 2</th><th>vir or phy</th><th>start address</th><th>end address</th><th>segment size (bytes)</th>
    </tr>
    <!--Code--->
    <tr>
        <td rowspan="2">Code</td><td>Virtual</td><td>564ee332c000</td><td>564ee332c855</td><td rowspan="2" style="background-color:rgb(118, 238, 198)">4294965163</td>
    </tr>
    <tr>
        <td>Physical</td><td style="background-color:rgb(118, 238, 198)">37da3a000</td><td style="background-color:rgb(118, 238, 198)">37da3a855</td>
    </tr
    <!--Data--->
    <tr>
        <td rowspan="2">Data</td><td>Virtual</td><td>564ee332ed7c</td><td>564ee332f014</td><td rowspan="2">4074724712</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000034d599d7c</td><td>800000035a7a4014</td>
    </tr>
    <!--Heap--->
    <tr>
        <td rowspan="2">Heap</td><td>Virtual</td><td>564ee4976000</td><td>564ee4997000</td><td rowspan="2">1767100416</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000036953d000</td><td>0</td>
    </tr>
    <!--Stack--->
    <tr>
        <td rowspan="2">Stack</td><td>Virtual</td><td>7fff6aecb180</td><td>7fff6aecb1a1</td><td rowspan="2" style="background-color:rgb(118, 238, 198)">4294967263</td>
    </tr>
    <tr>
        <td>Physical</td><td>800000034a60e180</td><td>800000034a60e1a1</td>
    </tr>
</table>
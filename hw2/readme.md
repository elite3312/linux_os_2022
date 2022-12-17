# Linux OS 2022 Project 2
**許富皓教授**好人一生平安

>Group: 第 14 組 
Member: 吳永璿 李玟卉 翁培馨
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
觀察兩process印出的結果，可以看到==lib, code segment具有相同的physical address==。
另外，兩隻process的stack區段大小相同，而heap、data區段的大小不同。
- process 1(pid = 5714  tid = 5714)
```sh
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
```sh
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

# Q&A
1. 你們測試程式使用getchar()讓程式等待，從getchar()到你輸入前程式是甚麼狀態(task_State)?
>開好兩隻shell都跑test_single_thread.out，再開第三隻shell跑ps a，
可以看到以下結果:
```sh
perry@perry-VirtualBox:~/Desktop/linux_os_2022$ ps a
    PID TTY      STAT   TIME COMMAND
   1040 tty2     Ssl+   0:00 /usr/lib/gdm3/gdm-x-session --run-script env GNOME_SHELL_SESSION_MODE=ubuntu /us
   1050 tty2     Sl+    0:30 /usr/lib/xorg/Xorg vt2 -displayfd 3 -auth /run/user/1000/gdm/Xauthority -backgro
   1180 tty2     Sl+    0:00 /usr/libexec/gnome-session-binary --systemd --systemd --session=ubuntu
   3749 pts/0    Ss     0:00 /usr/bin/bash --init-file /usr/share/code/resources/app/out/vs/workbench/contrib
   3776 pts/1    Ss     0:00 /usr/bin/bash --init-file /usr/share/code/resources/app/out/vs/workbench/contrib
   3805 pts/2    Ss     0:00 /usr/bin/bash --init-file /usr/share/code/resources/app/out/vs/workbench/contrib
   3897 pts/2    S+     0:00 ./test_single_thread.out
   3913 pts/1    S+     0:00 ./test_single_thread.out
   4057 pts/0    R+     0:00 ps a
```
>兩隻shell的test_single_thread.out都在等getchar()，所以state code都在interruptible sleep (可以參考ps的[man page](https://man7.org/linux/man-pages/man1/ps.1.html#PROCESS_STATE_CODES)來查state code)。  

2. 你說你會壓ctrl+c在shell中斷程式執行，ctrl+c送了甚麼signal?
>  Control+C 送出signal SIGINT信號, 可以用來中斷程式，且程式收到該信號可以決定要做啥(可能在關閉自己前做一些cleaning，或直接忽略信號。)
>  壓Control+Z則會送出SIGSTOP，可以暫停程式執行，直到收到SIGCONT。
3. 前面提到的signal由誰送出?
>  signal都是由一個process送往另一個process。根據[man page signal](https://man7.org/linux/man-pages/man7/signal.7.html)，Control+C送的SIGINT是用來Interrupt from keyboard，所以SIGINT應該是某一個跟keyboard有關的process送出的。
4. user space程式呼叫mmap的時候，mmap做了甚麼?
> mmap() 是一種虛擬記憶體映射文件的方法，將文件或其它對象映射到行程位址空間，為創建一個新的 vm_area_struct 結構，並賦值給 vma，設置 vma 的起始位址、结束位址等，過程主要可分成三個階段：
> 1. 行程開始映射，並且在虛擬記憶體為映射分配區域
>`void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);`
>`flags 可指定映射對象為 file 或 anonymous`
> ![](https://i.imgur.com/ZvWHZqw.png)
> https://zhikunhuo.blog.csdn.net/article/details/119938889
> 2. 調用 mmap（與上述不同）實現文件的實體位址和行程虛擬位址的映射關係
    `int mmap(struct file *filp, struct vm_area_struct *vma)`
> 3. 行程實際對該映射空間存取，造成 page fault，此時才將文件的內容移至實體記憶體
>
> 詳細步驟參考：
> https://www.cnblogs.com/huxiao-tee/p/4660352.html
> https://blog.csdn.net/weixin_42730667/article/details/120695939
- 研究一下copy_on_write機制
> 當多個行程共享資源時，若其中一個行程需要寫入時，不改變原始資源，複製一份修改後的副本（private copy）給該行程，而無修改的資源仍然保持，可達到節省記憶體空間。
> 參考資料：https://pythonspeed.com/articles/reduce-memory-array-copies/
- 哪些原因會造成page fault
> mmap() 只是建立了行程的 vma，但還沒有建立虛擬位址和實體位址的映射關係，因此當行程存取未建立映射關係的虛擬位址時，會觸發 page fault。
> 1. mmap() 將 file/anonymous 映射到 user space 的虛擬位址，沒有實際建立虛擬位址與實體位址的映射關係，因此當 process 要讀寫實體位址時觸發 page fault。
> 2. 匿名頁被回收到 swap 空間。
> 3. 當行程創建子行程時，複製父行程所有 vma 到子行程，以 readonly 方式共享資源，父或子行程第一次修改共享資源時觸發 page fault。
- 如何handle page fault?
> 根據發生 page fault 的虛擬位址和查找對應的 vma 以及 error code 轉換後的 flag 進行處理。
>1. 通過 vma_is_anonymous 方法，判斷 vmf->vma 的分段是否為 anonymous，如果是則調用 `do_anonymous_page`，如果不是則調用 `do_fault`。
>anonymous page fault 處理主要判斷是讀取還是寫入導致的，分爲兩個不同的處理方式。
>file page fault 處理分為三類種處理方式，讀取、私有資料寫入、共享資料寫入。
>2. 調用 `do_swap_page`，將之前 swap out 的匿名頁 swap in 回來。
>3. 調用 `do_wp_page`，其中一個行程發生寫入時，處理 CoW，分配新的實體空間。
>
>詳細步驟參考： 
>https://zhikunhuo.blog.csdn.net/article/details/122774021
>https://zhikunhuo.blog.csdn.net/article/details/123190194
>https://zhuanlan.zhihu.com/p/359929532

5. 你們的作業發現了兩隻process有辦法共用code segment跟lib segment，那你覺得有沒有辦法共用這兩個segment以外的segment?具體來說，兩隻process有辦法用mmap各自拿到相同記憶體嗎?(用mmap share memory實作)
> 可以，在mmap中有一個flags參數。該flags參數用於進行各種定制：
MAP_SHARED ->頁面是跨進程共享
MAP_PRIVATE ->頁面是單個進程擁有。
```
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
```
>參考資料 : 
https://blog.minhazav.dev/memory-sharing-in-linux/
https://en.wikipedia.org/wiki/Mmap
https://man7.org/linux/man-pages/man2/mmap.2.html
6. gcc fork如何使用copy_on_write機制?
>fork system call 會建立新的 process ，Linux 會先複製原 process 的 mm_struct , vm_area_struct跟page table，並將parent process的每個page的flag設為 read-only。簡單的說就是，fork()之後的child process程和parent process共享同一個地址空間。當parent process或child process對內存進行write時觸發page fault，kernel(page fault handler)才複製一份內存空間。
>ref:
>1. [fork()后copy on write的一些特性](https://zhou-yuxin.github.io/articles/2017/fork()%E5%90%8Ecopy%20on%20write%E7%9A%84%E4%B8%80%E4%BA%9B%E7%89%B9%E6%80%A7/index.html)
>2. [Linux 核心 Copy On Write 實作機制](https://hackmd.io/@linD026/Linux-kernel-COW-Copy-on-Write#Linux-%E6%A0%B8%E5%BF%83-Copy-On-Write-%E5%AF%A6%E4%BD%9C%E6%A9%9F%E5%88%B6)
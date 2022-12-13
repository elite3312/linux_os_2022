# linux OS course 2022
Includes some problems solved by adding linux system calls.
## Testing Environment:
- amd64 windows 10 physical os
- virtual box 6.1
- Ubuntu 20.04 LTS vm
- linux 5.8.1 kernel(https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/)
## How to reproduce results
Compile the kernel with the new system calls, and run the test-drivers.
## How to add a system call
- https://dev.to/jasper/adding-a-system-call-to-the-linux-kernel-5-8-1-in-ubuntu-20-04-lts-2ga8
- https://blog.kaibro.tw/2016/11/07/Linux-Kernel%E7%B7%A8%E8%AD%AF-Ubuntu/

# HW1
Write a multi-thread program with three threads (main thread, thread 1, and thread 2) and the new system calls to check which segments of a thread are shared by which other thread(s).
# HW2 
### Question
- Write a program using the system call you wrote in Project 1 to check how memory areas are shared by two processes that execute this program simultaneously.
- The memory areas include code segments, data segments, BSS segments, heap segments, libraries, stack segments.
### Hint
- When making your check, both related processes must be in progress. Hence you may need to use function sleep() to guarantee this requirement.
- Inside the Linux kernel, you need to use function copy_from_user() and function copy_to_user() to copy data from/to a user address buffer.
# HW3 
## Question 1
- (30 points) Write a new system call int get_CPU_number() so that a process can use it to get the number of the CPU that executes it.
```c
//prototype of the new system call is as follows:     
int get_CPU_number()
```
- You need to write a program to prove the effectiveness of your new system call
## Question 2
- (80 points) Write a new system call **start_to_count_number_of_process_switches()** so that a process can use it to begin to count the number of process switches the process makes. Besides, write another new system call **int stop_to_count_number_of_process_switches()** so that a process can use it to stop to count the number of process switches the process makes and return the number of process switches the process makes.
```c
//prototype of the new system call is as follows:     
void start_to_count_number_of_process_switches()
//prototype of the new system call is as follows:     
int stop_to_count_number_of_process_switches()
```
- You need to write a CPU-bound program and I/O-bound program to counter the number of process switches the programs make.

1. What follows is a code excerpt that you need to use in your I/O-bound program.
```c
#define ON  1;
#define OFF 0;
 
    void main()
    {           ...
        int              a,b=0;
        int              switch=ON;
                     
                ...                 
        start_to_count_number_of_process_switches();
        while(switch==ON)
        {
                ...  
            sleep(0.01 second);
            printf("[%d ]",b++);

                ... 
            if (this process has run 2 minutes)
                switch=OFF;            
                    ...
                    ... 
        }
        a=stop_to_count_number_of_process_switches();
        printf("\nDuring the past 2 minutes the process makes %d times process switches.\n",a);
    }
```
2. What follows is a code excerpt that you need to use in your CPU-bound program.
```c
#define ON  1;
#define OFF 0;

    void main()
    {           
        ...  
        int a;
        int switch=ON;
        float  b=0;
        ...                   
        start_to_count_number_of_process_switches();
        while(switch==ON)
        {
            ...
            b=b+1;
            ...
            if (this process has run 2 minutes)
                switch=OFF;            
                ...
                ... 
        }
        a=stop_to_count_number_of_process_switches();
        printf("During the past 2 minutes the process makes %d times process switches.\n",a);
    }
```
## Hint:
1. You can add a new field in the process descript of a process to store the number of process switches the process has made.
2. If you want to add a new field in struct task_struct, append it in the end of the struct. Do NOT insert it into struct task_struct.
3. Check the "Referenced Material" part of the Course web site to see how to add a new system call in Linux.
4. You can use API gettimeofday() to calculate the time a process used.
5. Process switches occur in function __switch_to().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#define __NR_my_get_cpu_number 444



int getcpu(unsigned *cpu)//the 3rd argument for getcpu is depricated since linux 2.6
{
        return *cpu=syscall(__NR_my_get_cpu_number);
}

int main(void)
{
        unsigned cpu;
        

        if(getcpu(&cpu)==-1)
        {
                printf("getcpu failed \n");
                return -1;
        }

        printf("cpu = %u ",cpu);

        return 0;
}
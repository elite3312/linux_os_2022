#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#define __NR_my_get_cpu_number 444
int getcpu_default(unsigned *cpu,unsigned *node)//the 3rd argument for getcpu is depricated since linux 2.6
{
        return syscall(SYS_getcpu,cpu,node,NULL);
}


int getcpu_my_call(unsigned *cpu)//the 3rd argument for getcpu is depricated since linux 2.6
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
        //unsigned cpu;
        unsigned node;

        if(getcpu_default(&cpu,&node)==-1)
        {
                printf("getcpu failed \n");
                return 1;
        }

        printf("cpu = %u node = %u(found with the built in getcpu())\n",cpu,node);

        return 0;
}
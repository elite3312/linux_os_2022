#define _GNU_SOURCE

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>



int getcpu(unsigned *cpu,unsigned *node)//the 3rd argument for getcpu is depricated since linux 2.6
{
        return syscall(SYS_getcpu,cpu,node,NULL);
}

int main(void)
{
        unsigned cpu;
        unsigned node;

        if(getcpu(&cpu,&node)==-1)
        {
                printf("getcpu failed \n");
                return 1;
        }

        printf("cpu = %u node = %u\n",cpu,node);

        return 0;
}
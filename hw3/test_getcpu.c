define _GNU_SOURCE

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>



int getcpu(unsigned *cpu,unsigned *node)
{

        return syscall(SYS_getcpu,cpu,node,NULL);
}

int main(void)
{
        unsigned cpu;
        unsigned node;

        if(getcpu(&cpu,&node,NULL)==-1)
        {
                printf("getcpu failed \n");
                return 1;
        }

        printf("cpu = %u node = %u\n",cpu,node);

        return 0;
}
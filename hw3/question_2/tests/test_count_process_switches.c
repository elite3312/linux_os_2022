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
    return 0;
    while(_switch==ON)
    {
                                
        //sleep(0.01);
        sleep(1);
        printf("[%d ]",b++);

        gettimeofday(&tv,NULL);
        current_time_in_seconds=tv.tv_sec;     
        if ((current_time_in_seconds-start_time_in_seconds)>=120)_switch=OFF;            
                                
    }
    a=syscall(__NR_stop_to_count_number_of_process_switches);
    printf("\nDuring the past 2 minutes the process makes %d times process switches.\n",a);
}
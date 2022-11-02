/* gcc -o my_other_test_multi_thread.out -pthread my_other_test_multi_thread.c */
#include <syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
//#include <dlfcn.h>

#define __NR_get_segments 443
#define __NR_get_physical_addresses 442
#define MAX_BUF_SIZE 128


//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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


struct AddrInfo {
	unsigned long int virt_addr;
	unsigned long int phys_addr;
};


//Data Segment//TLS
__thread int var = 1;

//Data Segment//Gloal
int MyDataInData = 100;

unsigned long *MyPhysicalAddress[6];

unsigned long get_phys_addr(unsigned long virtual_address) {
	//呼叫轉址的system call
	unsigned long physical_address;
     
    syscall(__NR_get_physical_addresses, &virtual_address, &physical_address);
    
	return physical_address;
}

void* worker(void* address_main, pthread_t pid){

	printf("\n\n");	
	//Stack
	int MyDataInStack = 1000;
	//BSS
	int MyDataInBss;   
	//Heap
	int *MyDataInHeap = malloc(sizeof(int));	
	*MyDataInHeap = 100;
	//libraries


	unsigned long MyVirtualAddress[6];
	MyVirtualAddress[0] = &var;
	MyVirtualAddress[1] = &MyDataInStack;
	MyVirtualAddress[2] = &MyDataInBss;
	MyVirtualAddress[3] = MyDataInHeap;

	MyVirtualAddress[4] = &MyDataInData;
	MyVirtualAddress[5] = address_main; 

	printf("Convert virtual addresses to physical addresses ...\n");
	
	#define NUMBER_OF_STRING 6
	#define MAX_STRING_SIZE 40

	char mesg_arr[NUMBER_OF_STRING][MAX_STRING_SIZE] =
	{ "TLS address",
  		"Stack address",
  		"BSS address",
  		"Heap address",
		
		"Data address",
		"Main address"
	};


	for(int i=0; i<6; i++){
		puts(mesg_arr[i]);
		printf("Virtual address : %lx ===> ",MyVirtualAddress[i]);
		printf("Physical address : %lx\n", get_phys_addr(MyVirtualAddress[i]));
	}
	/*finding the start, end address and size of each thread segment*/
	printf("\n=== finding the start, end address and size of each thread segment=== \n");
	struct ProcessSegments thread_segs;
	int tid = 0;
	tid = syscall(__NR_gettid);
	thread_segs.pid = tid;
   
	// get segments //把上面那個get_segments 寫進system call
    syscall(__NR_get_segments, (void *) &thread_segs);
	
    
	printf("%s: %lx-%lx (%lx-%lx) \n", 
			thread_segs.code_seg.seg_name,
			thread_segs.code_seg.start_addr,
			thread_segs.code_seg.end_addr,
			get_phys_addr(thread_segs.code_seg.start_addr),
			get_phys_addr(thread_segs.code_seg.end_addr));
	
    printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.data_seg.seg_name,
			thread_segs.data_seg.start_addr,
			thread_segs.data_seg.end_addr,
			get_phys_addr(thread_segs.data_seg.start_addr),
			get_phys_addr(thread_segs.data_seg.end_addr));

    printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.heap_seg.seg_name, 
			thread_segs.heap_seg.start_addr, 
			thread_segs.heap_seg.end_addr,
			get_phys_addr(thread_segs.heap_seg.start_addr), 
			get_phys_addr(thread_segs.heap_seg.end_addr));
    
	printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.stack_seg.seg_name, 
			thread_segs.stack_seg.start_addr, 
			thread_segs.stack_seg.end_addr,
			get_phys_addr(thread_segs.stack_seg.start_addr), 
			get_phys_addr(thread_segs.stack_seg.end_addr));
}


void get_thread_seg(char* thread_name, pthread_t pid) {
	
	//pthread_mutex_lock(&mutex);
	
	printf("\n=== THREAD START : %s === \n", thread_name);
	struct ProcessSegments thread_segs;
	int tid = 0;
	tid = syscall(__NR_gettid);
	thread_segs.pid = tid;
   
	// get segments //把上面那個get_segments 寫進system call
    syscall(__NR_get_segments, (void *) &thread_segs);
	
    
	printf("%s: %lx-%lx (%lx-%lx) \n", 
			thread_segs.code_seg.seg_name,
			thread_segs.code_seg.start_addr,
			thread_segs.code_seg.end_addr,
			get_phys_addr(thread_segs.code_seg.start_addr),
			get_phys_addr(thread_segs.code_seg.end_addr));
	
    printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.data_seg.seg_name,
			thread_segs.data_seg.start_addr,
			thread_segs.data_seg.end_addr,
			get_phys_addr(thread_segs.data_seg.start_addr),
			get_phys_addr(thread_segs.data_seg.end_addr));

    printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.heap_seg.seg_name, 
			thread_segs.heap_seg.start_addr, 
			thread_segs.heap_seg.end_addr,
			get_phys_addr(thread_segs.heap_seg.start_addr), 
			get_phys_addr(thread_segs.heap_seg.end_addr));
    
	printf("%s: %lx-%lx (%lx-%lx)\n", 
			thread_segs.stack_seg.seg_name, 
			thread_segs.stack_seg.start_addr, 
			thread_segs.stack_seg.end_addr,
			get_phys_addr(thread_segs.stack_seg.start_addr), 
			get_phys_addr(thread_segs.stack_seg.end_addr));
	
	//pthread_mutex_unlock(&mutex);


}


int main(){
	
  	pthread_t pid1,pid2;//,pid3;
	//char thread_name_1[] = "Thread1";
    //char thread_name_2[] = "Thread2";
	pthread_create(&pid1,NULL,worker,main);
	sleep(1);
	pthread_create(&pid2,NULL,worker,main);
	sleep(1);
	//pthread_create(&pid3,NULL,worker,main);
	//sleep(1);
	
	//char thread_name_3[] = "Thread3";
        

	//pthread_create(&pid1, NULL, get_thread_seg, thread_name_1);
    //sleep(1);
	//pthread_create(&pid2, NULL, get_thread_seg, thread_name_2);
    //sleep(1);
	// pthread_create(&pid3, NULL, get_thread_seg, thread_name_3);
    // sleep(1);

	pthread_join(pid1,NULL);
	pthread_join(pid2,NULL);
	//pthread_join(pid3,NULL);
	
   	return 0;

}
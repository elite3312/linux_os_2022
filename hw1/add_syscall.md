# my cheat sheet for adding syscall
before you start, use ctrl+f to replace stop_to_count_number_of_process_switches with your syscall
- cd ~/linux-5.8.1/

- mkdir stop_to_count_number_of_process_switches
- vim stop_to_count_number_of_process_switches/stop_to_count_number_of_process_switches.c
    - #write syscall
- vim stop_to_count_number_of_process_switches/Makefile
    - obj-y := stop_to_count_number_of_process_switches.o
- vim Makefile
    - /core-y
    - kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ stop_to_count_number_of_process_switches/

- vim include/linux/syscalls.h
    - at the bottom add
    - asmlinkage long sys_stop_to_count_number_of_process_switches(void);
- vim arch/x86/entry/syscalls/syscall_64.tbl
    - 444     common  stop_to_count_number_of_process_switches                sys_stop_to_count_number_of_process_switches
- sudo make menuconfig
- time sudo make -j4 #this takes several hours
- sudo make modules_install install -j4
- sudo update-grub
- systemctl reboot
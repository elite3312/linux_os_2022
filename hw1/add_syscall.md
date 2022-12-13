# my cheat sheet for adding syscall
before you start, use ctrl+f to replace my_get_cpu_number with your syscall
- cd ~/linux-5.8.1/

- mkdir my_get_cpu_number
- vim my_get_cpu_number/my_get_cpu_number.c
    - #write syscall
- vim my_get_cpu_number/Makefile
    - obj-y := my_get_cpu_number.o
- vim Makefile
    - /core-y
    - kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ my_get_cpu_number/

- vim include/linux/syscalls.h
    - at the bottom add
    - asmlinkage long sys_my_get_cpu_number(void);
- vim arch/x86/entry/syscalls/syscall_64.tbl
    xxx     common  my_get_cpu_number                sys_my_get_cpu_number
- sudo make menuconfig
- sudo make -j4
- sudo make modules_install install -j4
- sudo update-grub
- systemctl reboot
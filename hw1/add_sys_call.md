# my cheat sheet for adding syscall
before you start, use ctrl+f to replace my_get_physical_address with your syscall

- mkdir my_get_physical_address
- vim my_get_physical_address/my_get_physical_address.c
    - #write syscall
- vim my_get_physical_address/Makefile
    - obj-y := my_get_physical_address.o
- vim Makefile
    - /core-y
    - kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ my_get_physical_address/

- vim include/linux/syscalls.h
    - at the bottom add
    - asmlinkage long sys_my_get_physical_address(void);
- vim arch/x86/entry/syscalls/syscall_64.tbl
    xxx     common  my_get_physical_address                sys_my_get_physical_address
- sudo make menuconfig
- sudo make -j4
- sudo make modules_install install -j4
- sudo update-grub
- systemctl reboot
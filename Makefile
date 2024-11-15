ifneq ($(KERNELRELEASE),)

obj-m += klrm.o

ccflags-y:= -I ${src} -I ${src}/lib

klrm-y := mod_main.o api/api.o api/ioctl.o
klrm-y += password_setup/password_setup.o password_setup/password.o
klrm-y += state_machine/state_machine.o 
klrm-y += path_store/path_store.o path_store/inode_store.o path_store/store_iterate.o
klrm-y += lib/scth/scth.o wrappers/wrappers.o
klrm-y += lib/logfs/logfilefs_src.o lib/logfs/dir.o lib/logfs/file.o
klrm-y += logger/logger.o
klrm-y += oracles/probe.o oracles/pathname.o

else

TABLE_ADDR = $(shell sudo cat /sys/module/the_usctm/parameters/sys_call_table_address)

start:
	sudo insmod klrm.ko the_syscall_table=$(TABLE_ADDR)
	sudo mknod /dev/klrm-api c $$(sudo cat /sys/module/klrm/parameters/major) 0

stop:
	-sudo rm /dev/klrm-api
	sudo rmmod klrm.ko

all:
	$(CC) --version
	./utils/password_gen/passwordgen.sh
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rm ./password_setup/password.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

endif
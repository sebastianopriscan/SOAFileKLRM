ifneq ($(KERNELRELEASE),)

obj-m += klrm.o

ccflags-y:= -I ${src} -I ${src}/lib

klrm-y := mod_main.o api/api.o api/ioctl.o
klrm-y += password_setup/password_setup.o password_setup/password.o
klrm-y += state_machine/state_machine.o path_store/path_store.o
klrm-y += lib/scth/scth.o wrappers/wrappers.o

else

TABLE_ADDR = $(shell cat /sys/module/the_usctm/parameters/sys_call_table_address)

start:
	sudo insmod klrm.ko the_syscall_table=$(A)
	sudo mknod /dev/klrm-api c $$(sudo cat /sys/module/klrm/parameters/major) 0

stop:
	sudo rm /dev/klrm-api
	sudo rmmod klrm.ko

all:
	./utils/password_gen/passwordgen.sh
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rm ./password_setup/password.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

endif
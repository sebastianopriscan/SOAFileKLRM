ifneq ($(KERNELRELEASE),)

obj-m += klrm.o

ccflags-y:= -I ${src}

klrm-y := mod_main.o api/api.o api/ioctl.o
klrm-y += password_setup/password_setup.o password_setup/password.o
klrm-y += state_machine/state_machine.o path_store/path_store.o

else

start:
	sudo insmod klrm.ko
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
ifneq ($(KERNELRELEASE),)

obj-m += klrm.o

ccflags-y:= -I ./include

klrm-y := mod_main.o api/api.o password_setup/password_setup.o password_setup/password.o
klrm-y += state_machine/state_machine.o 

else

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

endif
obj-m += mod_main.o api/api.c password_setup/password_setup.c password_setup/password.c state_machine/state_machine.c

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean




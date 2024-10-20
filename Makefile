obj-m += klrm.o

klrm-y := mod_main.o $(wildcard state_machine/*) $(wildcard password_setup/*) $(wildcard api/*)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean




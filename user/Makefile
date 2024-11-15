.PHONY : reboot

reboot :
	-mkdir mount
	$(CC) -Wall -Wextra logfilemakefs.c -o logfilemakefs
	-rm image
	dd if=/dev/zero of=image bs=4096 count=100
	./logfilemakefs 100 image
	sudo mount -o loop -t singlefilefs image ./mount/

stop :
	sudo umount ./mount
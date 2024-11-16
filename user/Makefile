.PHONY : stop start clean

start : image logfilemakefs
	-mkdir mount
	sudo mount -o loop -t singlefilefs image ./mount/


image : logfilemakefs
	-rm image
	dd if=/dev/zero of=image bs=4096 count=100
	./logfilemakefs 100 image

logfilemakefs : logfilemakefs.c
	gcc -Wall -Wextra logfilemakefs.c -o logfilemakefs

klrmctl : client.c
	gcc -Wall -Wextra client.c -o 

stop :
	-sudo umount ./mount

clean :
	-rm image
	-rm logfilemakefs
	-rm klrmctl
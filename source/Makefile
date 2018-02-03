obj-m+=ic80_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) ic80_test.c -o ic80_test
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm ic80_test

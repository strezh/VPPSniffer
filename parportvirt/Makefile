obj-m += parportvirt.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/parportvirt modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/parportvirt clean


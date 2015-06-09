obj-m += parportsnif.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/parportsnif modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/parportsnif clean


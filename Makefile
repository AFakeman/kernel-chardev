obj-m := chardev.o
KVER := $(shell uname -r)
KPATH := /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KPATH) M=$(CURDIR)

install: chardev.ko
	sudo insmod chardev.ko

remove:
	sudo rmmod chardev

clean:
	$(MAKE) -C $(KPATH) M=$(CURDIR) clean

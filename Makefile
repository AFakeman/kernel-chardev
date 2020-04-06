obj-m := chardev.o
KVER := $(shell uname -r)
KPATH := /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KPATH) M=$(CURDIR)

clean:
	$(MAKE) -C $(KPATH) M=$(CURDIR) clean

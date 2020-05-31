MODULE := thread_test
obj-m := $(MODULE).o
KVER := $(shell uname -r)
KPATH := /lib/modules/$(KVER)/build
.PHONY: profile

all:
	$(MAKE) -C $(KPATH) M=$(CURDIR)

install: $(MODULE).ko
	sudo insmod $(MODULE).ko

remove:
	sudo rmmod $(MODULE)

profile: $(MODULE).ko
	sudo ./profile.sh $(MODULE)

clean:
	$(MAKE) -C $(KPATH) M=$(CURDIR) clean

obj-y := elevator_calls_wrappers.o
obj-m := elevator.o
elevatorMod-objs := elevator_calls_wrappers.o

PWD := $(shell pwd)
KDIR := /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.* Module.* modules.*

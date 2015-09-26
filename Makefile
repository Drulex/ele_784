# Variables pointing to different paths
KERNEL_DIR    	:= /lib/modules/$(shell uname -r)/build
PWD        	:= $(shell pwd)

# This is required to compile your <module.c> module
obj-m        := <module.o>

all: <module>

# We build our module in this section
myModule:
	@echo "Building the ELE784 Lab MODULE"
	@make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

# It's always a good thing to offer a way to cleanup our development directory
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order
	-rm -rf .tmp_versions

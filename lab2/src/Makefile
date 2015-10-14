# Variables pointing to different paths
KERNEL_DIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

# This is required to compile your ele784-lab1.c module
obj-m             = ele784-lab1.o
ele784-lab1-objs := charDriver.o circularBuffer.o

all: 

# We build our module in this section
charDriver:
	@echo "Building the ELE784 Lab1: Ring buffer driver..."
	@make -C $(KERNEL_DIR) M=$(PWD) modules

testCharDriver: testchardriver.o
	@echo "Building the ELE784 Lab1: Test ring buffer driver..."
	@echo "Invoking: GCC C Linker"
	gcc -o testchardriver testchardriver.o


# It's always a good thing to offer a way to cleanup our development directory
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order testchardriver
	-rm -rf .tmp_versions

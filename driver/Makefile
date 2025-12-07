# ======================================================
#  KERNEL MODULE MAKEFILE
# ======================================================

# Name of the module
obj-m += logitech_orbit_driver.o

# Source files
logitech_orbit_driver-y := src/logitech_orbit_driver.o

# Tell the kernel build system where to find your headers
EXTRA_CFLAGS += -I$(PWD)/include

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: logitech_orbit_driver.ko
	sudo insmod logitech_orbit_driver.ko || true
	sudo dmesg | tail -n 30

unload:
	sudo rmmod logitech_orbit_driver || true
	sudo dmesg | tail -n 30

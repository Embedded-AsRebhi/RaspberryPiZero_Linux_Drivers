# Project Overview
This repository contains a set of Driver designed to practice Linux device driver development on a Raspberry Pi Zero running Linux kernel version 6.12. 
These drivers are developed as part of a smart home system using various sensors, each performing a specific function.
# Repositories

 * The repository is organized into folders.
 * Each folder corresponds to a specific device driver .

# Environment 


* Hardware: Raspberry Pi Zero
* Kernel version: Linux 6.12

# Development Steps

 * Writing and compiling Linux kernel modules
 * Interfacing with hardware using platform drivers
 * Managing GPIOs and PWM using gpiod and PWM APIs
 * Defining hardware via the Device Tree
 * Debugging and testing drivers on embedded systems


# Getting Started

### 1. Clone the repository
```dts
git clone https://github.com/Embedded-AsRebhi/RaspberryPiZero_Linux_Drivers.git
```
## ?For each Driver you should :
### 2. Compile each driver

- Each driver must be recompiled individually to generate a .ko kernel module.

### 3. Makefile Used for Cross-Compilation

* Example of Makefile:
```dts
obj-m := module.o
KERNEL_DIR ?= $(HOME)/Linux_drivers_programmation_noyau/rpi0b32/linux

all:
	make -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) clean

deploy:
	scp *.ko rpiname@ipadress:/home/
```

### 4. Load the module on Raspberry Pi
```dts
sudo insmod driver.ko
```

### 5. Verify driver insertion

 * Use dmesg to check kernel logs

 * check /proc/devices and /dev/

### 6. Remove the module

```dts
sudo rmmod driver
```

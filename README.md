# 🏠 Project Overview
This repository contains a set of Driver designed to practice Linux device driver development on a Raspberry Pi Zero running Linux kernel version 6.12. 
These drivers are developed as part of a smart home system using various sensors, each performing a specific function.

# 🛠️ Repositories

 * The repository is organized into folders.
 * Each folder corresponds to a specific device driver .

# 📋 Environment 

* Hardware: Raspberry Pi Zero
* Kernel version: Linux 6.12
# 🌐 Project Communication Architecture

- After adding and configuring all the drivers, the project’s core idea is to establish communication between a server and a client using TCP sockets (TKinter interface).

* The client sends specific requests.
* The server, running on the Raspberry Pi, processes these requests and responds accordingly.
* For example, the client can:

  - Request the distance measured by the ultrasonic sensor,
  - Send a command to control a servo motor,
  - Ask for temperature and humidity values from the sensor.

# ⚙️ Development Steps

 * Writing and compiling Linux kernel modules
 * Interfacing with hardware using platform drivers
 * Managing GPIOs and PWM using gpiod and PWM APIs
 * Defining hardware via the Device Tree
 * Debugging and testing drivers on embedded systems


# 📊 Getting Implement the Driver 

### 1. Clone the repository
```dts
git clone https://github.com/Embedded-AsRebhi/RaspberryPiZero_Linux_Drivers.git
```
## For each Driver you should :
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
# 🚀 Getting launch the serveur and the client 

### 1.Compile and launch the server on the Raspberry Pi
- The server code is available in this repository. You need to compile it using gcc with the pigpio library:

```dts
sudo gcc serveur.c -o serveur -lpigpio
sudo ./serveur

```

### 2.Launch the client on your PC

- On the client side (your PC), run the Python application that contains the Tkinter interface.

### 3.Send requests from the client
- Through the GUI, you can do a different act as:
- Request distance values
- Control the servo motor
- Get temperature and humidity readings

* Make sure that both devices (client and server) are on the same network, and that the server IP is correctly set in the client code.

# 🧠 Understanding the Server-Client Logic

- To better understand the communication flow between the server and the client,
I have prepared a diagram that explains the overall logic and interaction steps.

- This visual representation helps clarify:

  - How requests are sent from the client (PC)
  - How the server (Raspberry Pi) processes them
![alt text](Messenger_creation_8672B7DE-BBD6-41AC-9EBE-1714E957AEFF.jpeg) 
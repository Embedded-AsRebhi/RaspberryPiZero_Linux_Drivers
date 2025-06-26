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
```dts
python3 dashboard.py
```

### 3.Send requests from the client
- The client requests are simulated using buttons in a Tkinter-based graphical interface, for example: 
- To get temperature and humidity, simply click on the "Refresh" button.
- To measure distance, click the "Distance" button.
- To change the interval for saving temperature and humidity data, you can select a value from a predefined dropdown list.
- You can also click "Historique" to view a log/history of previous temperature and humidity readings.


# 🧠 Understanding the Server-Client Logic

- To better understand the communication flow between the server and the client,
I have prepared a diagram that explains the overall logic and interaction steps.

- This visual representation helps clarify:

  - How requests are sent from the client (PC)
  - How the server (Raspberry Pi) processes them
 


# 💡Notes 

* Make sure that both devices (client and server) are on the same network, and that the server IP is correctly set in the client code.
* You can find all the wiring and connection details in the figure: cablage.png.
* Sometimes, multiple parts of the program try to access a shared resource at the same time so to prevent conflicts and ensure data integrity, I used a mutex to protect these shared resources.
* I launched parallel threads:
  - One thread periodically logs temperature and humidity data into a file.
  - Another thread is responsible for measuring distance continuously.
* On the other hand, I also prepared a separate program to retrieve the latest temperature and humidity , you can find the source file and more details in the folder LCD_I2c
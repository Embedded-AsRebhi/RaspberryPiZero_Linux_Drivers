#  Linux Kernel Driver for LED 
This kernel module implements a platform driver and a misc character device to interface with a DHT11 temperature and humidity sensor using the GPIO descriptor API (gpiod)available in Linux kernel 6.12 and above.

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "comp,dht11"; "`

# Hardware Interface

- Requires only an available (non-reserved) GPIO pin.
- Ensure the GPIO used is not already taken by another peripheral.

# DHT11 Sensor Communication – Wake-up Phase

- The microcontroller (master) initiates communication by pulling the data line LOW for   at least 18 ms.

- During this time, the DHT11 sensor wakes up and prepares a temperature and humidity measurement.

- After the 18 ms delay, the master releases the data line (sets it HIGH) and switches to listening mode.

- The sensor must respond within a timeout of 1000 µs (1 ms), otherwise the communication is considered failed.



# DHT11 Sensor Response and Data Transmission

- The sensor then transmits a total of 40 bits (5 bytes):

    * The first 2 bytes represent the humidity measurement.

    * The next 2 bytes represent the temperature measurement.

    * The fifth byte is a checksum, used to verify data integrity.

# Device Tree 

```dts
dht {
			compatible = "comp,dht11";
			label="dht"; 	
			data-gpio = <&gpio 14 GPIO_ACTIVE_HIGH>; 
			status = "okay"; 
		};
```
# Usage


- To read the current Temp and Hum:
  - `cat /dev/dht`
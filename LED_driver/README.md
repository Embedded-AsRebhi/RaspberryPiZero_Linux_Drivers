#  Linux Kernel Driver for LED 
This Linux kernel module implements a platform driver and exposes a miscellaneous character device interface for controlling LED devices.

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "Comp,mygpio"; "`

# Hardware Interface

- Requires only an available (non-reserved) GPIO pin.
- Ensure the GPIO used is not already taken by another peripheral.



# Device Tree 

```dts
led@2 {
    compatible = "Comp,mygpio";
    label = "led"; // fichier /dev/led
    led-gpio = <&gpio 4 GPIO_ACTIVE_HIGH>;
    status = "okay";
};
```
# Usage

- The driver accepts:
  - `echo 0 > /dev/led` → **Turns ON** the LED  
  - `echo 1 > /dev/led` → **Turns OFF** the LED

- To read the current state of the LED:
  - `cat /dev/led`
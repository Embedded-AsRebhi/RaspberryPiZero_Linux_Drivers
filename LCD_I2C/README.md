#  Linux Kernel I2C
This Linux kernel module implements an I2C-based driver to interface with an LCD screen. 

# 🖥️ LCD with I2C Communication

- To minimize the number of GPIO pins used for communication between the LCD and the Raspberry Pi, I used the PCF8574 I/O expander, which enables communication via the I2C protocol.
- You can find the detailed wiring in the figure: [LCD_Raspberry](LCD_Raspberry.png).

- In this setup:

  * The LCD is connected using I2C to the Raspberry Pi through the PCF8574 module.
  * I configured the communication in 4-bit mode to reduce the number of data lines required.
  * Only essential data pins are wired from the PCF8574 to the LCD.
  * A potentiometer is connected to adjust and control the LCD backlight brightness.
- This configuration provides a clean and efficient way to interface an LCD display using minimal GPIOs


# 🧠 Understanding PCF8574  Pinout

- The PCF8574 I2C I/O expander has a fixed base address of 0x20.
- To allow multiple devices on the same I2C bus, it uses three address pins:

 * A0, A1, A2: Logic inputs that define the full I2C address.
 * These form the 3 least significant bits of the I2C address.

 * This allows up to 8 different PCF8574 modules on the same I2C bus.
###  PCF8574 Address Table

| A2 | A1 | A0 | I2C Address (hex) |
|----|----|----|-------------------|
|  0 |  0 |  0 | `0x20`            |
|  0 |  0 |  1 | `0x21`            |
|  0 |  1 |  0 | `0x22`            |
|  0 |  1 |  1 | `0x23`            |
|  1 |  0 |  0 | `0x24`            |
|  1 |  0 |  1 | `0x25`            |
|  1 |  1 |  0 | `0x26`            |
|  1 |  1 |  1 | `0x27`            |

- There are two main methods to determine or set the I2C address used by the LCD module:

1. **Manual Configuration via Jumpers (A0, A1, A2)**  
- The I2C address can be manually set by changing the positions of the A0, A1, and A2 address selection jumpers on the module. Each combination alters the least significant bits of the address, allowing multiple LCDs to be used on the same I2C bus.

2. **Automatic Detection Using I2C Tools**  
- Using the I2C-tools as describing below.

# 🔍 Checking the I2C Address

- To ensure the correct I2C address of your device, follow these steps:
1. Install the i2c-tools package:
```dts
sudo apt install i2c-tools
```
2. List all accessible I2C buses:

```dts
i2cdetect -l
```

3. Scan the devices connected to a specific I2C bus (typically bus 1 on Raspberry Pi):
```dts
i2cdetect -y 1
```
- The detected address will be between 0x20 and 0x27 
# 🚀 Launching the Communication
```dts
gcc I2c_lcd.c -o lcd_test -lpigpio
sudo ./lcd_test
```

- Make sure the pigpio library is installed and the pigpiod daemon is running before executing the program.
- This will initiate communication with the LCD over I2C using the PCF8574 module.
- By default, the display will be on and no cursor is displayed

  * To enable cursor, use lcd.enableCursor(), or lcd.enableCursor(true)

  * To disable cursor, use lcd.enableCursor(false)

# 💡Notes 
- there are two useful link to more understand the PCF8574 and LCD

- LCD details : 
**https://circuitdigest.com/article/16x2-lcd-display-module-pinout-datasheet**
- PCF8574 details: 
**https://passionelectronique.fr/tutorial-pcf8574/**
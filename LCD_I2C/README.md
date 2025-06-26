#  Linux Kernel I2C
This Linux kernel module implements an I2C-based driver to interface with an LCD screen. 

# 🖥️ LCD with I2C Communication

- To minimize the number of GPIO pins used for communication between the LCD and the Raspberry Pi, I used the PCF8574 I/O expander, which enables communication via the I2C protocol.
🔌 You can find the detailed wiring in the figure: LCD_Raspberry.png.

- In this setup:

  * The LCD is connected using I2C to the Raspberry Pi through the PCF8574 module.
  * I configured the communication in 4-bit mode to reduce the number of data lines required.
  * Only essential data pins are wired from the PCF8574 to the LCD.
  * A potentiometer is connected to adjust and control the LCD backlight brightness.
- This configuration provides a clean and efficient way to interface an LCD display using minimal GPIOs

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
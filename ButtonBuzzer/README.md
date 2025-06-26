#  Linux Kernel Driver for Buzzer and push Button 
This Linux kernel module provides a platform driver that manages a buzzer and a button via GPIOs, enabling basic sound signaling and input detection. 

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "comp,intbutton"; "`
  `" compatible = "comp,buzzer"; "`

# Hardware Interface

- Requires only an available  GPIO and PWM pin.
- Ensure the GPIO and PWM used are not already taken by another peripheral.

# Device Tree of Button : interruption

```dts
soc{
	int_button@0 {
	compatible = "comp,intbutton"; //label
	pinctrl-names = "default";
	pinctrl-0 = <&button_pin>;
	gpios = <&gpio 17 0>;   
	interrupts = <17 1>;  
	interrupt-parent = <&gpio>; 
	debounce-interval = <200>; 
	status = "okay"; 
	};

}

&gpio{
	button_pin: key_pin {
		brcm,pins = <17>;
		brcm,function = <0>; /* pour forcer comme Input */
		brcm,pull = <BCM2835_PUD_UP>; //pour marquer qu'on va activer  une resistance pull up
	};
} 
```

- To find the device tree of the buzzer , you can search in the directory of Buzzer Driver to find it and for more information.

# Usage
- I configured the push button as an interrupt source and set the buzzer to operate at 2000 Hz.
- Each time the push button is pressed, the buzzer toggles its state:

    * On the first press, the buzzer starts emitting sound.
    * On the second press, the buzzer stops.


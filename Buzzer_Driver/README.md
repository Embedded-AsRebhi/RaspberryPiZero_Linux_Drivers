#  Linux Kernel Driver for Buzzer 
This kernel module implements a platform driver and a misc character device to interface with a Buzzer passif using PWM, typically used for sound  regulation.The driver allows dynamic adjustment of the duty cycle from user space.

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "comp,buzzer"; "`

- Exposes PWM control through a misc character device
- Implements proper PWM state management
- Thread-safe operations with error checking
- Initializes with 50% duty cycle

# Hardware Interface

- Requires only an available PWM pin.
- Ensure the PWM used is not already taken by another peripheral.

# Device Tree 

```dts
soc {
buzzer {
			compatible = "comp,buzzer";
			label="buzeer";
			pwms = <&pwm 0 250000 0>; // pwm0 → canal 0, 250000ns = 250kHz
			pwm-names = "buzzer";
			status = "okay";
		};
}

&pwm {
	pinctrl-names = "default";
	pinctrl-0 = <&pwm1_pins &pwm0_pins>;
	status = "okay";

};

&gpio{
pwm1_pins: pwm1_pins{
		brcm,pins = <13>;
		brcm,function = <4>; /* Input */
	};
}
```
# Usage

- To control the Buzzer sound, simply write the frequency value  to the device. This value determines the PWM duty cycle and the period :
    - `echo 2000 > /dev/buzzer` → **Turns ON** the buzzer  
    - `echo 0 > /dev/led` → **Turns OFF** the Buzzer
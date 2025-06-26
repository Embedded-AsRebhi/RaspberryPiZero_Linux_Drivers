#  Linux Kernel Driver for Servomotor 
This Linux kernel module implements a platform driver for controle the angle servomotor devices.

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "comp,servomotor"; "`

# Hardware Interface

- Requires only an available (non-reserved) GPIO pin.
- Ensure the GPIO used are not already taken by another peripheral.

# Device Tree 

```dts
soc{
		servomotor {
			compatible = "comp,servomotor";
			label="servomotor";
			pwms = <&pwm 1 250000 0>; // pwm1 , canal 1, 100000ns = 10kHz
			pwm-names = "servomotor";
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
		brcm,function = <4>; 
	};
} 
```
# Usage

- The driver accepts:
  - `echo 180 > /dev/servomotor` → **Turns ON 160°** the servomotor  
  ....
  - `echo 0 > /dev/servomotor` → **Turns ON 0°** the servomotor


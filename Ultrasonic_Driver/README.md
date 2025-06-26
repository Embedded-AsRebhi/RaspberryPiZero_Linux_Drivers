#  Linux Kernel Driver for Ultrason 
This Linux kernel module implements a platform driver and exposes a miscellaneous character device interface for measure a distance via Ultrasonic devices.

# Features

- Platform driver bound via Device Tree using the compatible :  
  `" compatible = "comp,ultrasonic"; "`

# Hardware Interface

- Requires only an available (non-reserved) 2 GPIOs pins.
- Ensure the GPIOs used are not already taken by another peripheral.

# Ultrasonic Sensor response

  - The ultrasonic sensor uses a transmitter that emits sound waves at frequencies        inaudible to humans.

  - When an object is present, the emitted sound reflects off the object.

  - The receiver captures the echoed sound within a certain time frame.

  - Based on the time elapsed between transmission and reception, the sensor calculates the distance to the object.

`

# Ultrasonic Sensor Measurement Steps

  * Trigger pulse sent:
  A 10 µs pulse is sent to the TRIG pin → this starts the measurement.

  * Ultrasonic emission:
  The sensor emits 8 ultrasonic pulses at 40 kHz (~200 µs duration).

  * Echo waiting:
  The ECHO pin goes HIGH when the signal is sent.

  * Echo reception:
  The ECHO pin goes LOW when the echo is received → this marks the end of the measurement.

  * Time calculation:
  Measure the duration for which ECHO was HIGH → this is the round-trip time.

  * Distance conversion:
  ```dts
  Distance=(SpeedofSound × Time)/2
  ```

  * Wait before next measurement:
  * Wait at least 60 ms before starting a new measurement for reliable results.

# Device Tree 

```dts
soc {
	ultrasonic {
			compatible = "comp,ultrasonic";
			label = "ultrasonic";
			pinctrl-names = "default";
			pinctrl-0 = <&echo_ultrasonic>;
			trig-gpios = <&gpio 5 GPIO_ACTIVE_HIGH>;   // GPIO5 - TRIG (output)
			echo-gpios = <&gpio 6 GPIO_ACTIVE_HIGH>;   // GPIO6 - ECHO (input)
			interrupt-parent = <&gpio> ;
			interrupts = <6 IRQ_TYPE_EDGE_BOTH>;
			status = "okay";
		};
}
&gpio{
	echo_ultrasonic: echo_ultrasonic {
		brcm,pins = <6>;
		brcm,function = <0>; 
	};
} 
```
# Usage

- To read the current distance:
  - `cat /dev/ultrasonic`
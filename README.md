# StepperMotorDriver
A stepper motor driver for Arduino Uno.

It uses a simple command line interface available via both Serial line (USB) and Ethernet (TCP, port 23, simple text console).

#### Used devices:
* Arduino Uno
* Stepper motor driver: HY-DIV-268N-5A
* Ethernet Shield (based on Wiznet W5100), link [here](https://botland.com.pl/pl/arduino-shield-komunikacja/3779-ethernet-shield-w5100-dla-arduino-z-czytnikiem-kart-microsd.html). Board is compatible with standard Arduino Ethernet library.
* Limit switches

#### Connections:
 * Limit switches -- used to limit motor's movement range   
   * Left limit switch (used when motor's position is going negative) is connected to **D2**.
   * Right limit switch (used when motor's position is going positive) is connected to **D3**.
   * Both input pins (**D2** and **D3**) have a 10k Ohm pull-down resistors. 
   * When a switch is hit, it shorts +5V (high state) to the **D2** or **D3**.

* Driver - HY-DIV268N-5A
  * Inputs **DIR-**, **PUL-**, **EN-** are connected to **GND**.
  * Input **DIR+** is connected to output **D5** via a 200 Ohm serial resistor.
  * Input **PUL+** is connected to output **D6** via a 200 Ohm serial resistor.
  * Input **EN+** is connected to output **D4** via a 200 Ohm serial resistor.
  * Driver's power stage input (**DC+**, **DC-**) is connected to a power supply, according to the drive's and motor's specs.
  * Driver's power stage output (**A+**, **B+**, **A-**, **B-**) is connected to a motor according to the specs.
  * Configuration DIP switches (1-6) sets up 5A current limit and 1/2 step division (half-step).

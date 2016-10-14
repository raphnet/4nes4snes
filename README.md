# 4nes4snes: Firmware to connect up to four NES/SNES controllers to USB

4nes4snes if a firmware for Atmel ATmega8 and Atmega168 which allows one to
connect up to 4 NES and/or SNES controllers to a PC using a single circuit.

The device connects to an USB port and appears to the
PC as standard HID joystick with 4 report Id's. This means
that it looks like 4 controllers in the Windows
control_panel->game_controllers window.

## Project homepage

Schematic and additional information such as build examples are available on the project homepage:

English: [4 NES and/or 4 SNES controller(s) to USB](http://www.raphnet.net/electronique/4nes4snes/index_en.php)
French: [4 manettes NES et/ou 4 SNES à USB](http://www.raphnet.net/electronique/4nes4snes/index.php)

## Supported micro-controller(s)

Currently supported micro-controller(s):

* Atmega8
* Atmega168

Adding support for other micro-controllers should be easy, as long as the target has enough
IO pins, enough memory (flash and SRAM) and is supported by V-USB.

## Built with

* [avr-gcc](https://gcc.gnu.org/wiki/avr-gcc)
* [avr-libc](http://www.nongnu.org/avr-libc/)
* [gnu make](https://www.gnu.org/software/make/manual/make.html)

## License

This project is licensed under the terms of the GNU General Public License, version 2.

## Acknowledgments

* Thank you to Objective development, author of [V-USB](https://www.obdev.at/products/vusb/index.html) for a wonderful software-only USB device implementation.

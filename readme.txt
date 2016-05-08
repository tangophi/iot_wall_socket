This repo contains code for a Arduino/ESP8266 based IOT wall socket project.

Details of the project can be found at:
http://www.instructables.com/id/IoT-Wall-Outlet-with-Arduino-and-ESP8266/?ALLSTEPS

iot_wall_socket_arduino contains the Arduino sketch for ATMEGA1284.  Use Arduino IDE to flash the code to the microcontroller.

bitmaps contains bmp files that need to be copied to a SD card that should be installed in the LCD shield.

iot_wall_socket_esp8266 contains the firmware source for ESP8266.  Use the ESP8266 SDK kit to compile and flash the firmware to the module.

wall_socket_kicad contains the Kicad files.

images contains snaps of the board.

readme2.txt contains some useful info and links.



Changelog:

May 07, 2016
============
* Added miscellaneous files/images.

March 15, 2015
==============
* Added Kicad files for the project.

July 22, 2015
=============
* Changed how the security alarm logo is displayed.  Instead of a single image and writing "Unarmed", "Arming" or "Armed" over it as before, now different images will be displayed for Arming and Armed states.
* Added two new bmp files, arming.bmp and armed.bmp, to the SD card.
* Changed checkalarminterval from 20 seconds to 5 seconds, so that the beeps can be heard more prominently in AlarmArming and AlarmTriggered states.

July 20, 2015
=============
* Light on/off works
* Fan on/off as well as changing speed works
* Touchscreen works
* IR remote control works
* Smart algorithms works
* Security Alarm works
* Twitter/SMS Alerts works - but due to Twitter notification limits, some issues remain
* Physical switcehs work
* Light/Fan logo on display changes irrespective of mode of control - IR remote, smartphone app or physical switches
* Power consumption readings very erratic, probably due to noise in the circuit (relays may be the main culprit, they are not opto isolated)
* Mood lighting works




Explanation about pullup resistors for Arduino Input pins... 39 kohms resistor is used in the zero cross detector circuit.. 10K was not enough...
http://electronics.stackexchange.com/questions/23645/how-do-i-calculate-the-required-value-for-a-pull-up-resistor

Modified Adafruit GFX and ILI9341 libraries that work with UTouch library  (But is not used)
http://forum.arduino.cc/index.php?topic=181679.210

Fonts for Adafruit GFX library  (This lib is used - works with UTouch lib and has some fonts)
http://www.instructables.com/id/Arduino-TFT-display-and-font-library/?ALLSTEPS



http://www.homautomation.org/2013/09/17/current-monitoring-with-non-invasive-sensor-and-arduino/
https://github.com/openenergymonitor/EmonLib
http://openenergymonitor.org/emon/buildingblocks/ct-and-ac-power-adaptor-installation-and-calibration-theory


Twitter SMS notification...

NetIO with node-red





{"EnvData":{"Temperature":"29.60","Humidity":"48.10","LightState":1,"FanState":0}}



http://stackoverflow.com/questions/12380471/how-do-i-install-websocket-module-for-node-js-on-debian-vps
http://stackoverflow.com/questions/27160643/node-red-works-on-localhost-but-error-lost-connection-to-server-on-azure
 npm install websocket
 npm install -g node-gyp
 
 Enable https in settings.js
 
 http://nodejs.org/api/https.html#https_https_createserver_options_requestlistener
 
http://stackoverflow.com/questions/21397809/create-a-self-signed-ssl-cert-for-localhost-for-use-with-express-node 
 http://www.akadia.com/services/ssh_test_certificate.html
 
 
 
 
 
 
 node-red
 For both cubietruck and raspberry pi:
 
 1. Install nodejs.  The standard packages dont work.  Only the one below works.
 Even though the link below is for Rpi, it worked for cubietruck too...
 http://conoroneill.net//download-compiled-version-of-nodejs-0120-stable-for-raspberry-pi-here
 
 2. Install node-red
  /usr/local/bin/npm install -g node-red

 
 
 http://nodered.org/docs/hardware/raspberrypi.html
 
 
 
 
 
 
 
 
 
 
 How to send data from node-red to common IoT cloud platforms such as Thingspeak
 
 http://logic.sysbiol.cam.ac.uk/?p=1473
 http://logic.sysbiol.cam.ac.uk/teaching/Node-RED_IoT-platform_Test.pdf
 
 
 
 
 
 
 
 
 
 var field = "";
 switch(msg.topic) {
     case "/socket1/Temperature":
         field = "field1";
     break;
     case "/socket1/Humidity":
         field = "field2";
     break;
     case "/socket1/LightState":
         field = "field3";
     break;
     case "/socket1/FanState":
         field = "field4";
     break;
     case "/socket1/LedState":
         field = "field6";
     break;
     case "/socket1/Alarm":
         field = "field7";
     break;
 }
 
 context.buff = context.buff || "";
 context.count = context.count || 0;
 
 if (msg.topic != "purge") {
     if (context.buff.length > 0) {
         context.buff = context.buff+"&"+field+"="+msg.payload;
     } else {
         context.buff = field+"="+msg.payload;
     }
     context.count += 1;
  }
  
 if (context.count == 8 || (msg.payload=="purge" && context.count >= 1)) {
      msg.topic="";
      msg.payload = context.buff;
      context.buff = "";
      context.count = 0;
      return msg;
 }
  
return null;




TCP: data received 26 bytes
{
"LightCommand":1
}TCP: data received 44 bytes
COLOR: 206
COLOR: 255
COLOR: 0
{
"LedColorCommand":{
"red":206,
"green":255,
"blue":0
}
}










Intro:
In this instructable, I show you how to add enhanced accessibility, smartness and connectivity to an ordinary wall socket.  This is done with a combination of Arduino, various sensors, ESP8266 and a combination of softwares.

Features:
Light
Fan
Fan Speed
IR remote control
Touchscreen
Smartphone App
Twitter Alerts
SMS Alerts
Mood Lighting
Log to Cloud services
Smart Algorithms


Design Considerations:
This project was conceived when I felt the need to automatically reduce the speed of AC ceiling fan in the bedroom as time passes or temperature drops through the night.  Because in Bangalore, where I live, during winters, I like to switch on the fan when I go to sleep.  But early mornings, when the temperature drops, the fan makes it even colder.  Which disturbs my sleep and I dont want to or cant get up to switch off the fan since the wall socket is not close to the bed.

Based on this requirement, the following were considered while desiging this project.
1.  Should automatically reduce fan speed based on the time elapsed or temperature drop.
2.  Should still be able to operate physical swiches.
3.  Should be able to remotely control light and fan.
4.  Should be able to function independently.
5.  If the board breaks down for any reason, before it can be fixed, it should still be possible to use the physical switches without any changes.
6.  The board should update the state of the devices controlled properly irrespective of the mode of control - IR remote, smartphone app or physical switches.


Basics:

What is an Arduino?
Arduino is an open-source electronic prototyping platform allowing to create interactive electronic objects.
http://www.arduino.cc/

What is ATMEGA1284P-PU?
This is the microcontroller used in this project.
http://www.atmel.com/devices/ATMEGA1284P.aspx

What is ESP8266?
ESP8266 is a highly integrated chip designed for the needs of a new connected world. It offers a complete and self-contained Wi-Fi networking solution, allowing it to either host the application or to offload all Wi-Fi networking functions from another application processor.

https://espressif.com/en/products/esp8266/.
http://mcuoneclipse.com/2014/10/15/cheap-and-simpl...
https://scargill.wordpress.com/?s=esp8266

What is MQTT?
MQTT is a machine-to-machine (M2M)/"Internet of Things" connectivity protocol. It was designed as an extremely lightweight publish/subscribe messaging transport. It is useful for connections with remote locations where a small code footprint is required and/or network bandwidth is at a premium. For example, it has been used in sensors communicating to a broker via satellite link, over occasional dial-up connections with healthcare providers, and in a range of home automation and small device scenarios.
http://mqtt.org/
http://en.wikipedia.org/wiki/MQTT

What is node-red?
Node-RED is a tool for wiring together hardware devices, APIs and online services in new and interesting ways.
http://nodered.org/

What is NetIO?
NetIO is a multi platform smartphone application, a generic remote controller for almost everything. It simply sends and reads strings over a network socket. You can easily communicate with microcontrollers or PCs connected to your LAN!
http://netio.davideickhoff.de/en/

What is DeviceHub.net?
DeviceHub.net is an IoT platform that allows makers and companies to easily connect their Internet-enabled hardware projects to a dashboard for data gathering, data analysis and remote control. 
https://devicehub.net/

What is Thingspeak?
ThingSpeak is an open source “Internet of Things” application and API to store and retrieve data from things using HTTP over the Internet or via a Local Area Network. With ThingSpeak, you can create sensor logging applications, location tracking applications, and a social network of things with status updates.
https://thingspeak.com/



Parts:

ATMEGA1284P microcontroller chip
ESP8266 ESP-01 module
TFT Touch SPI LCD screen
OLED i2c LCD screen
DHT11 or DHT22 sensor
5v relays
WS2812B LED strip
IR receiver
Active buzzer
BTA136 traics
MOC3041 triacs
4N25 zero cross detectors
Bridge rectifier
DIP switches
Resistors
Capacitor
Breakaway male and female headers
PCBs
Enclosures
220v AC to 5v DC 2 Amps power supply module
Current transformer
LM1117 3.3v LDO voltage regulator
FTDI USB to TTL adapter cable

These are the main parts needed.  However there might be a few other things needed as well.

In addition to the above, you need a working Arduino board (any model should be fine) to install the bootloader on the ATMEGA1284P chip.












Circuit:
The circuit is split into two parts:
1.  First is the main microcontroller circuit.  Only DC here.
2.  Second is for all the AC components.  Mix of AC and DC here.

This is a combination of various circuits I have found on the web.  The only part of the circuit that I had to struggle with is the one for controlling the speed of a AC fan.  Although I found a few variations of this circuit, I did not get a proper schematic.  Hence, this part of the circuit needs a little explanation.

Basically the idea is that the speed of the AC fan could be varied by using 250v AC capacitors.  Depending on how much capacitance is in the circuit, the speed of the fan could be increased.  In my circuit, you can notice that there are four MOC3041/BTA136 triacs.  Each of the first three triacs is connected to capacitors of 1uF, 2uF and 3uF capacitors and then to the fan.  The last is directly connected to the fan.

With this circuit, its possible to have 7 different speeds by triggering the triacs as needed.

Speed 0:  None of the MOC3041 triacs are triggered.
Speed 1:  Only the MOC3041 connected to 1uF is triggered.
Speed 2:  Only the MOC3041 connected to 2uF is triggered.
Speed 3:  Only the MOC3041 connected to 3uF is triggered.
Speed 4:  MOC3041s connected to 1uF and 3uF are triggered.
Speed 5:  MOC3041s connected to 2uF and 3uF are triggered.
Speed 6:  MOC3041s connected to 1uF, 2uF and 3uF are triggered.
Speed 7:  Only the MOC3041 directly connected to the fan is triggered.


Another thing about this circuit are the two zero cross detectors - one for the light and the the other for the fan.  These are needed to check if the light or fan is actually switched on or off.    Note that there are two physical 2-way switches on the wall socket that can still be used.   The zero cross detector circuit is used when the physical switch is used and the state of the light or the fan needs to be updated appropriately.



Connections:
============





Code:
For the ATMEGA1284p, the sketch is available at:


For the ESP8266, the firmware source is available at:


The node-red code is available at:



Operations:
Touch Screen
Physical switches
IR remote
Smartphone


Monitor:
=======
thingspeak and devicehub links and screenshots here.


Security Alarm:
The PIR motion detector can be used as a security alarm.  The alarm can be "armed" when going out.  And when the alarm is armed, and if motion is detected, the screen will show a keypad so that the security code can be entered.  If the correct security code is entered within a minuted then the alarm is diarmed.  But if the correct code is not entered within a minute, a tweet is sent along with acvitating the buzzer. 

Also, a SMS message will be sent to the mobile number associated with the twitter account.   This is achieved by enabling SMS notifications in Twitter account settings.



Smart Features/Algorithms:
1.  Automatic Fan Speed Control:
    Various options to set the fan speed are vailable:
    a.  Decrease fan speed by 1 step for every 1 degree drop in temperature.
    b.  Decrease fan speed by 2 steps for every 1 degree drop in temperature.
    c.  Decrease fan speed by 1 step every 90 minutes.
    d.  Decrease fan speed by 2 steps every 90 minutes.
    e.  Stop fan if temperature falls below the specified temperature.
    
2.  Time alarm options:
    When the clock arm goes off, along with the buzzer, the following options can be set.
    a.  Switch on light
    b.  Switch off fan.
    c.  Switch on fan and set it to maximum speed.
    
3.  Automatic full screen clock display.
    During nights (when the fan is switched on and the light is switched off), if a movement is detected after 10 minutes of idleness, show the current time in full screen so that it is visible clearly from the bed.  
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
smartphone app screenshot
IR remote control photo
High level diagram of various components
video
big clock screenshot
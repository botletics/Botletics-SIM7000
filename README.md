## Overview
- This library is based on the [Adafruit FONA library](https://github.com/adafruit/Adafruit_FONA) and supports a variety of SIMCom 2G/3G/4G LTE/CAT-M/NB-IoT modules, including the SIM7000 LTE CAT-M/NB-IoT module.
- More info on the Botletics SIM7000 shield hardware can be found [here](https://www.botletics.com/products/sim7000-shield) and [here on Amazon](https://a.co/d/apOaGUD)
- Technical documentation and GitHub wiki can be found in the [SIM7000-LTE-Shield repo](https://github.com/botletics/SIM7000-LTE-Shield)

For a list of library functions, please [see this wiki page](https://github.com/botletics/SIM7000-LTE-Shield/wiki/Library-Functions)

To get help or share a project you've done using this hardware or library, please go to the [Botletics community forum](https://forum.botletics.com/)

### Confirmed functionalities
- SSL for SIM7000
- Dedicated MQTT commands on SIM7000A. Tested on firmware versions 1351B03SIM7000A and 1351B04SIM7000A
- Sleep mode and e-DRX configuration (~1.5mA)
- FTP functions on SIM7000A firmware version 1351B03SIM7000A
- FTP functions on SIM7000G firmware version 1529B01SIM7000G
- Hologram SIM card works great on both the AT&T and Verizon CAT-M1 networks in the USA!
- HTTP and HTTPS via 3G with SIM5320
- MQTT working with SIM7000 and SIM808 using TCP commands
- HTTP functions on SIM808 and other 2G modules
- Network time works
- Ultra low-power power down mode (~7.4uA)
- GPS works on SIM7000 without a SIM card! The NMEA data includes UTC date/time, latitutde, longitude, altitude, etc and is quite accurate! It also obtains a GPS fix fairly quickly, only about 20-30s from cold start and even less (a couple seconds or so) when the device has already been on! However, this may differ based on your location and how deep you're buried inside a building.
- HTTP functions via LTE CAT-M1 with SIM7000
- SMS functions (sending/reading/deleting SMS)
- Generic stuff (reading supply voltage, netowrk connection, RSSI, etc.)

### To-Do List
- FTP image transfering
- FTP extended GET/PUT methods
- Test and document MDM9206 SDK for standalone SIM7000 operation without external microcontroller

### Completed Tasks
- Tested SSL connection without certificate verification. Sent data to https://dweet.io using LTE_Demo sketch
- Tested SIM7070G general features with firmware 1951B14SIM7070 on SIM7070G: sending/receiving SMS, HTTP GET, HTTP POST, and GPS
- Added SSL for TCP: opens port 8883 and allows MQTTS over SSL. Currently this feature is only for sending data. To use the function addRootCA(), set `#define SSL_FONA 1` in Adafruit_FONA.h
- Added SIM7070G compatibility
- Added SIM7600 compatibility
- Tested SIM7000 dedicated MQTT commands and SIM7000_MQTT_Demo.ino with CloudMQTT
- Added the [SIM7000_MQTT_Demo sketch](https://github.com/botletics/SIM7000-LTE-Shield/blob/master/Code/examples/SIM7000_MQTT_Demo/SIM7000_MQTT_Demo.ino) which uses the dedicated MQTT commands
- Added MQTT functions for SIM7000 (see SIM7000 MQTT app note)
- Documented all available library functions [on this page](https://github.com/botletics/SIM7000-LTE-Shield/wiki/Library-Functions)
- Fixed some minor inconsistencies in some Adafruit functions, mainly functions that were declared as int32_t but never actually outputting a negative number, or spelling mistakes in comments, among other things.
- Added setPreferredMode(), setPreferredLTEMode(), and setOperatingBand() functions
- Added SAMD_LTE_Demo sketch for ATSAMD microcontrollers (Arduino Zero, Adafruit M0, etc.)
- Added ESP32_LTE_Demo sketch
- Set AT+CFUN=1 and AT+CGDCONT (PDP context) in example code during module initialization
- Set PDP context with AT+CGDCONT in setNetworkSettings() function
- Added enableSleepMode(), set_eDRX(), enablePSM(), and setNetLED() methods
- Fixed and tested FTP connect, GET/PUT, rename, delete, and quit functions
- Added setFunctionality() method for changing AT+CFUN
- Confirmed NTP time sync on SIM7000G firmware version 1529B01SIM7000G
- Added "postData3G" function for SIM5320 and other 3G modules
- Successfully tested MQTT connect and publish using TCP/IP!
- Added "getNetworkInfo()" function (command "1" in LTE_Demo sketch) to get connection info, connection status, cellular band, etc.
- Added if statements that can be uncommented to eliminate unnecessary AT commands from "enableGPRS()" function for LTE modules (SIM7000) in case GPRS isn't being used at all
- Added HTTP status and data length verification in postData() function
- Added driver files in documentation folder
- Added voice and audio support on revision v5
- Added baud rate setup in the setup() function in example sketches to eliminate the need for a separate "setbaud" sketch.
- Tested the [Hologram developer SIM card](https://hologram.io/devplan/)
- Updated the IoT example sketch to include support for a GPS tracker (repeated data posting)
- Created [this cool IoT example](https://github.com/botletics/NB-IoT-Shield/tree/master/Code/examples/IoT_Example) that posts GPS location, temperature, and battery data to the cloud!
- Updated the function "getGPS()" to include the LTE shield class
- I have tested almost every AT command and they have all worked!
- Added a function "postData()" for posting data to dweet.io, a free cloud API. You can choose to use HTTP GET or POST and I have added an example in the "FONA_LTE_Test" sketch (enter "2" in the menu for the option)
- Tested the [AT&T Trio SIM card](https://marketplace.att.com/products/trio-sim-trial)
- Tweaked the library so that the LTE declaration "Adafruit_FONA_LTE fona = Adafruit_FONA_LTE()" doesn't include the reset pin because the reset pin is reserved for emergencies only, according to the SIM7000 design document.
- Tested HTTP commands
- Tested the dual LTE/GNSS antenna signal strength. Inside (near the window) I get an RSSI around 31 (-52 dBm) for AT&T
- Added "powerDown()" function using "AT+CPOWD=1" to turn off the SIM7000
- Added "hangUp()" function
- Created "FONA_LTE_setbaud.ino" sketch to set baud rate to 4800 from default 115200 because at 115200 some parts of text would show up weird in the serial monitor.
-	Added “setBaudrate(uint16_t baud)” function for LTE class using "AT+IPR=<rate>". 
-	Included SIM7000 types (A/C/E/G versions)
-	Included FONA_LTE class to the library for SIM7000A module
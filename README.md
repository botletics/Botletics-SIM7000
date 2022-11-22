### Overview
- This library is based on the [Adafruit FONA library](https://github.com/adafruit/Adafruit_FONA) and supports a variety of SIMCom 2G/3G/4G LTE/CAT-M/NB-IoT modules, including SIM800/SIM808/SIM5320/SIM7000/SIM7070/SIM7500/SIM7600 modules.
- More info on the Botletics SIM7000 shield hardware can be found [here on Botletics](https://www.botletics.com/products/sim7000-shield) and [here on Amazon](https://a.co/d/apOaGUD)
- Technical documentation and GitHub wiki for the Botletics SIM7000 shield can be found in the [SIM7000-LTE-Shield repo](https://github.com/botletics/SIM7000-LTE-Shield)
- To get help or share a project you've done using this hardware or library, please go to the [Botletics community forum](https://forum.botletics.com/)

### Confirmed functionalities
- SIM5320: SMS, HTTP(S)
- SIM7000: SMS, HTTP(S), GPS, FTP, MQTT using TCP & dedicated commands, sleep mode & e-DRX, power down mode
- SIM7070: SMS, HTTP(S), GPS
- SIM7500: SMS, HTTP(S), GPS, voice calling

### To-Do List
- SIM7000 FTP image transfering
- SIM7000 FTP extended GET/PUT methods
- Test and document MDM9206 SDK for standalone SIM7000 operation without external microcontroller
- SIM7500 FTP & MQTT

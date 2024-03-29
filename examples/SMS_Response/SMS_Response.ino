/*  This is an example sketch that receives text messages from the user
 *   and sends a response back dto the user depending on what the
 *   message was! You can use this code for all sorts of cool applications
 *   like remote sensors, SMS-activated controls, and many other things!
 *  
 *  Author: Timothy Woo (www.botletics.com)
 *  Github: https://github.com/botletics/NB-IoT-Shield
 *  Last Updated: 1/7/2021
 *  License: GNU GPL v3.0
 */

#include "BotleticsSIM7000.h" // https://github.com/botletics/Botletics-SIM7000/tree/main/src

/******* ORIGINAL ADAFRUIT FONA LIBRARY TEXT *******/
/***************************************************
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// #include "Botletics_modem.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

// Define *one* of the following lines:
//#define SIMCOM_2G // SIM800/808/900/908, etc.
//#define SIMCOM_3G // SIM5320
#define SIMCOM_7000
//#define SIMCOM_7070
//#define SIMCOM_7500
//#define SIMCOM_7600

// For botletics SIM7000 shield
#define PWRKEY 6
#define RST 7
//#define DTR 8 // Connect with solder jumper
//#define RI 9 // Need to enable via AT commands
#define TX 10 // Microcontroller RX
#define RX 11 // Microcontroller TX
//#define T_ALERT 12 // Connect with solder jumper

// For botletics SIM7500 shield
//#define PWRKEY 6
//#define RST 7
////#define DTR 9 // Connect with solder jumper
////#define RI 8 // Need to enable via AT commands
//#define TX 11 // Microcontroller RX
//#define RX 10 // Microcontroller TX
////#define T_ALERT 5 // Connect with solder jumper

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial modemSS = SoftwareSerial(TX, RX);

// Use the following line for ESP8266 instead of the line above (comment out the one above)
//SoftwareSerial modemSS = SoftwareSerial(TX, RX, false, 256); // TX, RX, inverted logic, buffer size

SoftwareSerial *modemSerial = &modemSS;

// Hardware serial is also possible!
//HardwareSerial *modemSerial = &Serial1;

// For ESP32 hardware serial use these lines instead
//#include <HardwareSerial.h>
//HardwareSerial modemSS(1);

// Use this for 2G modules
#ifdef SIMCOM_2G
  Botletics_modem modem = Botletics_modem(RST);
  
// Use this one for 3G modules
#elif defined(SIMCOM_3G)
  Botletics_modem_3G modem = Botletics_modem_3G(RST);
  
// Use this one for LTE CAT-M/NB-IoT modules (like SIM7000)
// Notice how we don't include the reset pin because it's reserved for emergencies on the LTE module!
#elif defined(SIMCOM_7000) || defined(SIMCOM_7070) || defined(SIMCOM_7500) || defined(SIMCOM_7600)
Botletics_modem_LTE modem = Botletics_modem_LTE();
#endif

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;
char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!

void setup() {
//  while (!Serial);

  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state
  
  // Turn on the module by pulsing PWRKEY low for a little bit
  // This amount of time depends on the specific module that's used
  modem.powerOn(PWRKEY); // Power on the module

  Serial.begin(9600);
  Serial.println(F("SMS Response Test"));
  Serial.println(F("Initializing....(May take several seconds)"));

  // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
  // Press Arduino reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset

  // Software serial:
  modemSS.begin(115200); // Default SIM7000 shield baud rate

  Serial.println(F("Configuring to 9600 baud"));
  modemSS.println("AT+IPR=9600"); // Set baud rate
  delay(100); // Short pause to let the command run
  modemSS.begin(9600);
  if (! modem.begin(modemSS)) {
    Serial.println(F("Couldn't find modem"));
    while (1); // Don't proceed if it couldn't find the device
  }

  // Hardware serial:
  /*
  modemSerial->begin(115200); // Default SIM7000 baud rate

  if (! modem.begin(*modemSerial)) {
    DEBUG_PRINTLN(F("Couldn't find SIM7000"));
  }
  */

  // The commented block of code below is an alternative that will find the module at 115200
  // Then switch it to 9600 without having to wait for the module to turn on and manually
  // press the reset button in order to establish communication. However, once the baud is set
  // this method will be much slower.
  /*
  modemSerial->begin(115200); // Default LTE shield baud rate
  modem.begin(*modemSerial); // Don't use if statement because an OK reply could be sent incorrectly at 115200 baud

  Serial.println(F("Configuring to 9600 baud"));
  modem.setBaudrate(9600); // Set to 9600 baud
  modemSerial->begin(9600);
  if (!modem.begin(*modemSerial)) {
    Serial.println(F("Couldn't find modem"));
    while(1); // Don't proceed if it couldn't find the device
  }
  */
  
  type = modem.type();
  Serial.println(F("Modem is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case SIM800L:
      Serial.println(F("SIM800L")); break;
    case SIM800H:
      Serial.println(F("SIM800H")); break;
    case SIM808_V1:
      Serial.println(F("SIM808 (v1)")); break;
    case SIM808_V2:
      Serial.println(F("SIM808 (v2)")); break;
    case SIM5320A:
      Serial.println(F("SIM5320A (American)")); break;
    case SIM5320E:
      Serial.println(F("SIM5320E (European)")); break;
    case SIM7000:
      Serial.println(F("SIM7000")); break;
    case SIM7070:
      Serial.println(F("SIM7070")); break;
    case SIM7500:
      Serial.println(F("SIM7500")); break;
    case SIM7600:
      Serial.println(F("SIM7600")); break;
    default:
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  uint8_t imeiLen = modem.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }

  // Set modem to full functionality
  modem.setFunctionality(1); // AT+CFUN=1

  // Configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  //modem.setNetworkSettings(F("your APN"), F("your username"), F("your password"));
  //modem.setNetworkSettings(F("m2m.com.attz")); // For AT&T IoT SIM card
  //modem.setNetworkSettings(F("telstra.internet")); // For Telstra (Australia) SIM card - CAT-M1 (Band 28)
  modem.setNetworkSettings(F("hologram")); // For Hologram SIM card

  // Optionally configure HTTP gets to follow redirects over SSL.
  // Default is not to follow SSL redirects, however if you uncomment
  // the following line then redirects over SSL will be followed.
  //modem.setHTTPSRedirect(true);

  /*
  // Other examples of some things you can set:
  modem.setPreferredMode(38); // Use LTE only, not 2G
  modem.setPreferredLTEMode(1); // Use LTE CAT-M only, not NB-IoT
  modem.setOperatingBand("CAT-M", 12); // AT&T uses band 12
//  modem.setOperatingBand("CAT-M", 13); // Verizon uses band 13
  modem.enableRTC(true);
  
  modem.enableSleepMode(true);
  modem.set_eDRX(1, 4, "0010");
  modem.enablePSM(true);

  // Set the network status LED blinking pattern while connected to a network (see AT+SLEDS command)
  modem.setNetLED(true, 2, 64, 3000); // on/off, mode, timer_on, timer_off
  modem.setNetLED(false); // Disable network status LED
  */

  modemSerial->print("AT+CNMI=2,1\r\n");  // Set up the modem to send a +CMTI notification when an SMS is received

  Serial.println("Modem Ready");
}

  
char modemNotificationBuffer[64];  //for notifications from the modem
char smsBuffer[250];
char callerIDbuffer[32];  //we'll store the SMS sender number in here

int slot = 0;            //this will be the slot number of the SMS

void loop() {
  
  char* bufPtr = modemNotificationBuffer;    //handy buffer pointer
  
  if (modem.available())      //any data available from the modem?
  {
    int charCount = 0;
    //Read the notification into modemInBuffer
    do  {
      *bufPtr = modem.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (modem.available()) && (++charCount < (sizeof(modemNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(modemNotificationBuffer, "+CMTI: " MODEM_PREF_SMS_STORAGE ",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
            
      // Retrieve SMS sender address/phone number.
      if (! modem.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
      uint16_t smslen;
      if (modem.readSMS(slot, smsBuffer, 250, &smslen)) { // pass in buffer and max len!
        Serial.println(smsBuffer);

        // OPTIONAL: Check for a magic password and do something special!
        // Maybe turn on a relay, send a reading only with this password,
        // or send GPS coordinates! Use your imagination!
        const char* magicPass = "OPEN SESAME!"; // You can create more magic passwords!
        if (strcmp(smsBuffer, magicPass) == 0) { // Check if strictly identical
          // Unlock the secret stash of chocolate!
          sendText("Treasure chest unlocked!");
          // Do stuff like digitalWrite(lockPin, HIGH); or something
          // Use your imagination!
        }
        else if (strstr(smsBuffer, magicPass) != NULL) { // Only check if the magic password is somewhere in the text
          sendText("Opening secret door!");
          // Do stuff like digitalWrite(door, HIGH)
        }
        else { // The text didn't contain the magic password!
          // Reply with normal sensor data!
          float sensorVal = analogRead(A0) * 1.123; // For testing
          char textMessage[100]; // Make sure this is long enough!
          char sensorBuff[16];
          dtostrf(sensorVal, 1, 2, sensorBuff); // float_val, min_width, digits_after_decimal, char_buffer
          strcpy(textMessage, "Sensor reading: ");
          strcat(textMessage, sensorBuff);

          sendText(textMessage);
        }
      }
    }
  }
}

// Send an SMS response
void sendText(const char* textMessage) {
  Serial.println("Sending reponse...");
  
  if (!modem.sendSMS(callerIDbuffer, textMessage)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }
  
  // Delete the original message after it is processed.
  // Otherwise we will fill up all the slots and
  // then we won't be able to receive any more!
  if (modem.deleteSMS(slot)) {
    Serial.println(F("OK!"));
  } else {
    Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
    modem.print(F("AT+CMGD=?\r\n"));
  }
}

/*  This example sketch loads the Adafruit IO ISRG Root X1 certificate
    onto the SIM7000 using its Electronic File System (EFS) commands for HTTPS/MQTTS.

    Reference: https://github.com/adafruit/certificates/blob/main/data/roots-full.pem
    
    Author: Timothy Woo (www.botletics.com)
    Github: https://github.com/botletics/SIM7000-LTE-Shield
    Last Updated: 5/21/2026
    License: GNU GPL v3.0
 */

#include "BotleticsSIM7000.h" // https://github.com/botletics/Botletics-SIM7000/tree/main/src

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

/************************* PIN DEFINITIONS *********************************/
// For botletics SIM70X0 shield
#define PWRKEY 6
#define RST 7
#define TX 10 // Microcontroller RX
#define RX 11 // Microcontroller TX

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

Botletics_modem_LTE modem = Botletics_modem_LTE();

uint8_t type;
char imei[16] = {0}; // Use this for device ID

void setup() {
  Serial.begin(9600);
  Serial.println(F("*** SIM7000 EFS Certificate Loader ***"));

  #ifdef LED
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
  #endif
  
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state

  modem.powerOn(PWRKEY); // Power on the module
  moduleSetup(); // Establishes first-time serial comm and prints IMEI

  // Unlock SIM card if needed
  // Remember to uncomment the "PIN" variable definition above
  /*
  if (!modem.unlockSIM(PIN)) {
    Serial.println(F("Failed to unlock SIM card"));
  }
  */

  // Set modem to full functionality
  modem.setFunctionality(1); // AT+CFUN=1

  // Inject your script here:
  Serial.println(F("Flashing Root CA to EFS..."));
  if (loadEFSCert()) {
    Serial.println(F("CA flashed successfully!"));
  } else {
    Serial.println(F("CA flash failed)"));
  }
}

void loop() {
  // Nothing here
}

void moduleSetup() {
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
}

const char PROGMEM root_ca[] =
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"-----END CERTIFICATE-----\r\n";

bool loadEFSCert() {
  char cmdBuff[100];
  uint16_t certLength = sizeof(root_ca) - 1;

  Serial.println(F(">>> Initialize EFS..."));
  modem.sendCheckReply(F("AT+CFSINIT"), F("OK")); // Will give ERROR if already initiated previously

  Serial.println(F(">>> Write File onto EFS..."));
  sprintf(cmdBuff, "AT+CFSWFILE=3,\"root.pem\",0,%i,5000", certLength);
  if (! modem.sendCheckReply(cmdBuff, F("DOWNLOAD"), 2000)) return false;

  unsigned long timeout = millis();
  String buffer = "";

  Serial.println(F(">>> Streaming data..."));
  Serial.println(F("---------------- BEGIN FILE DATA DUMP ----------------"));
  
  for (int i = 0; i < certLength; i++) {
    char c = pgm_read_byte_near(root_ca + i);
    
    modemSS.write(c); // Sends the character to the SIM7000G EFS
    Serial.write(c);  // Echoes the exact same character to the Serial Monitor
    
    // Tiny delay every 64 bytes to prevent serial buffer overrun on the modem
    if (i % 64 == 0) delay(4);
  }
  
  Serial.println(F("\n----------------- END FILE DATA DUMP -----------------"));
  Serial.println(F(">>> Stream complete. Waiting for EFS save confirmation..."));

  if (! modem.expectReply(F("OK"))) return false;

  Serial.println(F(">>> Verify cert..."));
  modem.sendCheckReply(F("AT+CFSGFIS=3,\"root.pem\""), F("OK")); // Check to see if the file is actually there

  modem.sendCheckReply(F("AT+CSSLCFG=\"convert\",2,\"root.pem\""), F("OK")); // Permanent, 1-time operation

  Serial.println(F(">>> Terminating EFS..."));
  modem.sendCheckReply(F("AT+CFSTERM"), F("OK"));
  
  return true;
}

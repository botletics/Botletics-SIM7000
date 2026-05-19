 /* This is an example sketch to send battery, temperature, and GPS location data to
    the cloud via either HTTP GET, POST, or MQTT to Adafruit IO.
    
    SETTINGS: You can choose to post only once or to post periodically
    by commenting/uncommenting "#define samplingRate 30". When this line is 
    commented out the AVR microcontroller and MCP9808 temperature sensor are put to 
    sleep to conserve power, but when the line is being used data will be sent to the
    cloud periodically. This makes it operate like a GPS tracker!
    
    PROTOCOL: You can use HTTP GET or POST requests and you can change the URL to pretty
    much anything you want. You can also use MQTT to publish data to different feeds
    on Adafruit IO. You can also subscribe to Adafruit IO feeds to command the device
    to do something! In order to select a protocol, simply uncomment a line in the #define
    section below!
    
    IoT Example Getting-Started Tutorial: https://github.com/botletics/SIM7000-LTE-Shield/wiki/GPS-Tracker-Example
    GPS Tracker Tutorial Part 1: https://www.instructables.com/id/Arduino-LTE-Shield-GPS-Tracking-Freeboardio/
    GPS Tracker Tutorial Part 2: https://www.instructables.com/id/LTE-Arduino-GPS-Tracker-IoT-Dashboard-Part-2/
    
    Author: Timothy Woo (www.botletics.com)
    Github: https://github.com/botletics/Botletics-SIM7000
    Last Updated: 5/19/2026
    License: GNU GPL v3.0
  */

#include "BotleticsSIM7000.h" // https://github.com/botletics/Botletics-SIM7000/tree/main/src

// You don't need the following includes if you're not using MQTT
// You can find the Adafruit MQTT library here: https://github.com/adafruit/Adafruit_MQTT_Library
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"

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

// Uncomment *one* of the following protocols you want to use
// to send data to the cloud! Leave the other commented out
#define PROTOCOL_HTTP_GET         // Generic
// #define PROTOCOL_HTTP_POST     // Generic
// #define PROTOCOL_MQTT           // Adafruit IO or other broker

/************************* PIN DEFINITIONS *********************************/
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

#define LED 13 // Just for testing if needed!

char replybuffer[255]; // Large buffer for server replies

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

/************************* ADAFRUIT IO PARAMETERS *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_PORT        1883
#define AIO_USERNAME    "USERNAME"
#define AIO_PASSWORD    "PASSWORD"   // AIO key

#ifdef PROTOCOL_MQTT
  // Set topic names to publish and subscribe to
  // NOTE: For Adafruit IO, the feed name format is: "{username}/feeds/{feed_name}"
  #define LOC_TOPIC       "location"
  #define TEMP_TOPIC      "temperature"
  #define BATT_TOPIC      "battery"
  #define SUB_TOPIC       "command"     // Subscribe topic name

  uint8_t replyidx = 0;
#endif

/****************************** OTHER STUFF ***************************************/
// For sleeping the AVR
#include <avr/sleep.h>
#include <avr/power.h>

// For temperature sensor
#include <Wire.h>
#include "Adafruit_MCP9808.h"

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// The following line is used for applications that require repeated data posting, like GPS trackers
// Comment it out if you only want it to post once, not repeatedly every so often
#define samplingRate 30 // The time in between posts, in seconds

// The following line can be used to turn off the shield after posting data. This
// could be useful for saving energy for sparse readings but keep in mind that it
// will take longer to get a fix on location after turning back on than if it had
// already been on. Comment out to leave the shield on after it posts data.
//#define turnOffShield // Turn off shield after posting data

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char imei[16] = {0}; // Use this for device ID
uint8_t type;
uint16_t battLevel = 0; // Battery level (percentage)
float latitude, longitude, speed_kph, heading, altitude, second;
uint16_t year;
uint8_t month, day, hour, minute;
uint8_t counter = 0;
unsigned long timer = 0;
bool firstTime = true;
//char PIN[5] = "1234"; // SIM card PIN

char URL[200];  // Make sure this is long enough for your request URL
char body[100]; // Make sure this is long enough for POST body
char latBuff[12], longBuff[12], locBuff[50], speedBuff[12],
     headBuff[12], altBuff[12], tempBuff[12], battBuff[12];

void setup() {
  Serial.begin(9600);
  Serial.println(F("*** SIMCom Module IoT Example ***"));

  #ifdef LED
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
  #endif
  
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state

  modem.powerOn(PWRKEY); // Power on the module
  moduleSetup(); // Establishes first-time serial comm and prints IMEI

  if (!tempsensor.begin()) {
    Serial.println("Couldn't find the MCP9808!");
    tempsensor.wake(); // Wake up the MCP9808 if it was sleeping and retry
    if (!tempsensor.begin()) while (1);
  }
 
  // Unlock SIM card if needed
  // Remember to uncomment the "PIN" variable definition above
  /*
  if (!modem.unlockSIM(PIN)) {
    Serial.println(F("Failed to unlock SIM card"));
  }
  */

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

  // Perform first-time GPS/GPRS setup if the shield is going to remain on,
  // otherwise these won't be enabled in loop() and it won't work!
  #ifndef turnOffShield
    // Enable GPS
    while (!modem.enableGPS(true)) {
      Serial.println(F("Failed to turn on GPS, retrying..."));
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Turned on GPS!"));

    #if !defined(SIMCOM_3G) && !defined(SIMCOM_7500) && !defined(SIMCOM_7600)
      // Disable GPRS just to make sure it was actually off so that we can turn it on
      if (!modem.enableGPRS(false)) Serial.println(F("Failed to disable GPRS!"));
      
      // Turn on GPRS
      while (!modem.enableGPRS(true)) {
        Serial.println(F("Failed to enable GPRS, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Enabled GPRS!"));
    #endif
  #endif
}

void loop() {
  // --- NON-BLOCKING TIMER TO SEND DATA PERIODICALLY ---
  if (firstTime || millis() - timer > samplingRate * 1000UL) {
    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus()) {
      Serial.println(F("Failed to connect to cell network, retrying..."));
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));

    // Measure battery level
    // Note: on the LTE shield this won't be accurate because the SIM7000
    // is supplied by a regulated 3.6V, not directly from the battery. You
    // can use the Arduino and a voltage divider to measure the battery voltage
    // and use that instead, but for now we will use the function below
    // only for testing.
    battLevel = readVcc(); // Get voltage in mV

    // Measure temperature
    tempsensor.wake(); // Wake up the MCP9808 if it was sleeping
    float tempC = tempsensor.readTempC();
    float tempF = tempC * 9.0 / 5.0 + 32;
    Serial.print("Temp: "); Serial.print(tempC); Serial.print("*C\t"); 
    Serial.print(tempF); Serial.println("*F");
    
    Serial.println("Shutting down the MCP9808...");
    tempsensor.shutdown(); // In this mode the MCP9808 draws only about 0.1uA

    float temperature = tempC; // Select what unit you want to use for this example

    delay(500); // Found that this helps

    // Turn on GPS if it wasn't on already (e.g., if the module wasn't turned off)
    #ifdef turnOffShield
      while (!modem.enableGPS(true)) {
        Serial.println(F("Failed to turn on GPS, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Turned on GPS!"));
    #endif

    // Get a fix on location, try every 2s
    // Use the top line if you want to parse UTC time data as well, the line below it if you don't care
    //  while (!modem.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
    while (!modem.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude)) {
      Serial.println(F("Failed to get GPS location, retrying..."));
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Found 'eeeeem!"));
    Serial.println(F("---------------------"));
    Serial.print(F("Latitude: ")); Serial.println(latitude, 6);
    Serial.print(F("Longitude: ")); Serial.println(longitude, 6);
    Serial.print(F("Speed: ")); Serial.println(speed_kph);
    Serial.print(F("Heading: ")); Serial.println(heading);
    Serial.print(F("Altitude: ")); Serial.println(altitude);
    /*
    // Uncomment this if you care about parsing UTC time
    Serial.print(F("Year: ")); Serial.println(year);
    Serial.print(F("Month: ")); Serial.println(month);
    Serial.print(F("Day: ")); Serial.println(day);
    Serial.print(F("Hour: ")); Serial.println(hour);
    Serial.print(F("Minute: ")); Serial.println(minute);
    Serial.print(F("Second: ")); Serial.println(second);
    */
    Serial.println(F("---------------------"));
    
    // If the shield was already on, no need to re-enable
    #if defined(turnOffShield) && !defined(SIMCOM_3G) && !defined(SIMCOM_7500) && !defined(SIMCOM_7600)
      // Disable GPRS just to make sure it was actually off so that we can turn it on
      if (!modem.enableGPRS(false)) Serial.println(F("Failed to disable GPRS!"));
      
      // Turn on GPRS
      while (!modem.enableGPRS(true)) {
        Serial.println(F("Failed to enable GPRS, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Enabled GPRS!"));
    #endif

    // Post something like temperature and battery level to the web API
    // Construct URL and post the data to the web API

    // Format the floating point numbers
    dtostrf(latitude, 1, 6, latBuff);
    dtostrf(longitude, 1, 6, longBuff);
    dtostrf(speed_kph, 1, 0, speedBuff);
    dtostrf(heading, 1, 0, headBuff);
    dtostrf(altitude, 1, 1, altBuff);
    dtostrf(temperature, 1, 2, tempBuff); // float_val, min_width, digits_after_decimal, char_buffer
    dtostrf(battLevel, 1, 0, battBuff);

    // Also construct a combined, comma-separated location array
    // (many platforms require this for dashboards, like Adafruit IO):
    sprintf(locBuff, "%s,%s,%s,%s", speedBuff, latBuff, longBuff, altBuff); // This could look like "10,33.123456,-85.123456,120.5"
    
    // Construct the appropriate URL's and body, depending on request type
    // In this example we use the IMEI as device ID

    #ifdef PROTOCOL_HTTP_GET
      // GET request
      #if defined(SIMCOM_7000) || defined(SIMCOM_7070)
          // Add headers as needed
          // modem.HTTP_addHeader("User-Agent", "SIM7000", 7);
          // modem.HTTP_addHeader("Cache-control", "no-cache", 8);
          // modem.HTTP_addHeader("Connection", "keep-alive", 10);
          // modem.HTTP_addHeader("Accept", "*/*", 3);

          // ---------- CONNECT TO SERVER ---------- //
          if (! modem.HTTP_connect(AIO_SERVER))
              Serial.println(F("Failed to connect to server..."));

          // ---------- ADAFRUIT IO HTTP(S) GET ---------- //
          // Format URI with GET request query string
          // Format: "/api/v2/{username}/feeds/{feed_key}/data?{key1}={value1}&{key2}={value2}"
          // Send to a feed called "sim7000"
          sprintf(URL, "/api/v2/%s/feeds/sim7000/data?lat=%s&long=%s&speed=%s&head=%s&alt=%s&temp=%s&batt=%s", AIO_USERNAME,
                  latBuff, longBuff, speedBuff, headBuff, altBuff, tempBuff, battBuff);
          
          modem.HTTP_addHeader("x-aio-key", AIO_PASSWORD, strlen(AIO_PASSWORD));
          modem.HTTP_GET(URL);

          // Can also include buffer to parse through server response afterward
          // modem.HTTP_GET(URL, replybuffer, sizeof(replybuffer));
          // Serial.print(F("Server reply: ")); Serial.println(replybuffer); // Debug
      #elif defined(SIMCOM_3G) || defined(SIMCOM_7500) || defined(SIMCOM_7600)
          // Format URI with GET request query string
          // You can adjust the contents of the request if you don't need certain things like speed, altitude, etc.
          // Send to a feed called "sim7000"
          sprintf(URL, "GET /api/v2/%s/feeds/sim7000/data?lat=%s&long=%s&speed=%s&head=%s&alt=%s&temp=%s&batt=%s HTTP/1.1\r\nHost: io.adafruit.com\r\n\r\n",
                  AIO_USERNAME, latBuff, longBuff, speedBuff, headBuff, altBuff, tempBuff, battBuff);
          
          // Add headers
          modem.HTTP_para(F("x-aio-key"), AIO_PASSWORD);

          // Try a total of three times if the post was unsuccessful (try additional 2 times)
          while (counter < 3 && !modem.postData("io.adafruit.com", 443, "HTTPS", URL)) { // Server, port, connection type, URL
            Serial.println(F("Failed to complete HTTP(S) request..."));
            counter++; // Increment counter
            delay(1000);
          }

          counter = 0;
      #else
          sprintf(URL, "https://io.adafruit.com/api/v2/%s/feeds/sim7000/data?lat=%s&long=%s&speed=%s&head=%s&alt=%s&temp=%s&batt=%s",
                  AIO_USERNAME, imei, latBuff, longBuff, speedBuff, headBuff, altBuff, tempBuff, battBuff);
          
          // Add headers
          modem.HTTP_para(F("x-aio-key"), AIO_PASSWORD);

          while (counter < 3 && !modem.postData("GET", URL)) {
            Serial.println(F("Failed to post data, retrying..."));
            counter++; // Increment counter
            delay(1000);
          }

          counter = 0;

      #endif
    #elif defined(PROTOCOL_HTTP_POST)
      // POST request
      #if defined(SIMCOM_7000) || defined(SIMCOM_7070)
          // ---------- CONNECT TO SERVER ---------- //
          if (! modem.HTTP_connect(AIO_SERVER))
              Serial.println(F("Failed to connect to server..."));

          // ---------- ADAFRUIT IO HTTP(S) POST ---------- //
          // Post new data
          // Format: "/api/v2/{username}/feeds/{feed_key}/data"
          // Send data feed by feed:

          // Location
          sprintf(URL, "/api/v2/%s/feeds/location/data", AIO_USERNAME);
          modem.HTTP_addHeader("x-aio-key", AIO_PASSWORD, strlen(AIO_PASSWORD));
          modem.HTTP_addHeader("Content-Type", "application/x-www-form-urlencoded", 34);
          modem.HTTP_addPara("value", locBuff, strlen(locBuff));
          modem.HTTP_POST(URL);

          // Can also include buffer to parse through server response afterward:
          // modem.HTTP_POST(URL, replybuffer, sizeof(replybuffer)); 
          // Serial.print(F("Server reply: ")); Serial.println(replybuffer); // Debug

          // modem.HTTP_clearHeaders();
          // modem.HTTP_clearParams();

          // Temperature
          memset(URL, 0, sizeof(URL)); // Clear buffer
          sprintf(URL, "/api/v2/%s/feeds/temperature/data", AIO_USERNAME);    
          modem.HTTP_addPara("value", tempBuff, strlen(tempBuff));
          modem.HTTP_POST(URL);

          modem.HTTP_disconnect();
          
      #elif defined(SIMCOM_3G) || defined(SIMCOM_7500) || defined(SIMCOM_7600)
          // Format JSON body and query string
          // Send to a feed called "sim7000"
          sprintf(body, "{\"lat\":%s,\"long\":%s,\"speed\":%s,\"head\":%s,\"alt\":%s,\"temp\":%s,\"batt\":%s}\r\n",
                  latBuff, longBuff, speedBuff, headBuff, altBuff, tempBuff, battBuff); // Terminate with CR+NL
          sprintf(URL, "POST /api/v2/%s/feeds/sim7000/data HTTP/1.1\r\nHost: io.adafruit.com\r\nContent-Length: %i\r\n\r\n", AIO_USERNAME, strlen(body));
          
          // Add headers
          modem.HTTP_para(F("x-aio-key"), AIO_PASSWORD);

          // Can try with other servers like thingsboard.io
          /*
          const char * token = "qFeFpQIC9C69GDFLWdAv"; // From thingsboard.io device
          sprintf(URL, "http://demo.thingsboard.io/api/v1/%s/telemetry", token);
          sprintf(body, "{\"lat\":%s,\"long\":%s,\"speed\":%s,\"head\":%s,\"alt\":%s,\"temp\":%s,\"batt\":%s}", latBuff, longBuff,
                  speedBuff, headBuff, altBuff, tempBuff, battBuff);
          // sprintf(body, "{\"lat\":%s,\"long\":%s}", latBuff, longBuff); // If all you want is lat/long
          */

          // Try a total of three times if the post was unsuccessful (try additional 2 times)
          while (counter < 3 && !modem.postData("io.adafruit.com", 443, "HTTPS", URL, body)) {
            Serial.println(F("Failed to complete HTTP(S) request..."));
            counter++; // Increment counter
            delay(1000);
          }

          counter = 0;

      #else
          sprintf(body, "{\"lat\":%s,\"long\":%s,\"speed\":%s,\"head\":%s,\"alt\":%s,\"temp\":%s,\"batt\":%s}\r\n",
                  latBuff, longBuff, speedBuff, headBuff, altBuff, tempBuff, battBuff); // Terminate with CR+NL
          sprintf(URL, "https://io.adafruit.com/api/v2/%s/feeds/sim7000/data", AIO_USERNAME);
          
          // Add headers
          modem.HTTP_para(F("x-aio-key"), AIO_PASSWORD);
          modem.HTTP_para(F("Content-Type"), F("application/x-www-form-urlencoded"));

          while (counter < 3 && !modem.postData("POST", URL, body)) {
            Serial.println(F("Failed to post data, retrying..."));
            counter++; // Increment counter
            delay(1000);
          }

          counter = 0;

      #endif
    #elif defined(PROTOCOL_MQTT)
      // Let's use MQTT!
      // modem.MQTT_connect(false); // In case you want to reset the connection.

      // If not already connected, connect to MQTT
      if (! modem.MQTT_connectionStatus()) {
        // Set up MQTT parameters (see MQTT app note for explanation of parameter values)
        modem.MQTT_setParameter("URL", AIO_SERVER, AIO_PORT);
        modem.MQTT_setParameter("CLEANSS", "1"); // This prevents problems with subscribing

        // Set up MQTT username and password if necessary
        modem.MQTT_setParameter("CLIENTID", imei); // You need this, otherwise it may not connect
        modem.MQTT_setParameter("USERNAME", AIO_USERNAME);
        modem.MQTT_setParameter("PASSWORD", AIO_PASSWORD);
        // modem.MQTT_setParameter("KEEPTIME", "60"); // Time to connect to server, 60s by default
        
        Serial.println(F("Connecting to MQTT broker..."));
        if (! modem.MQTT_connect(true)) {
          Serial.println(F("Failed to connect to broker!"));
        }
      }
      else {
        Serial.println(F("Already connected to MQTT server!"));
      }

      // Now publish all the GPS and temperature data to their respective topics!
      // Parameters for MQTT_publish: Topic, message (0-512 bytes), message length, QoS (0-2), retain (0-1)
      char GPS_feed[80];
      char TEMP_feed[80];
      char BATT_feed[80];
      char SUB_feed[80];

      sprintf(GPS_feed, "%s/feeds/%s", AIO_USERNAME, LOC_TOPIC);
      sprintf(TEMP_feed, "%s/feeds/%s", AIO_USERNAME, TEMP_TOPIC);
      sprintf(BATT_feed, "%s/feeds/%s", AIO_USERNAME, BATT_TOPIC);
      sprintf(SUB_feed, "%s/feeds/%s", AIO_USERNAME, SUB_TOPIC);

      if (!modem.MQTT_publish(GPS_feed, locBuff, strlen(locBuff), 1, 0)) Serial.println(F("Failed to publish!")); // Send GPS location
      if (!modem.MQTT_publish(TEMP_feed, tempBuff, strlen(tempBuff), 1, 0)) Serial.println(F("Failed to publish!")); // Send temperature
      if (!modem.MQTT_publish(BATT_feed, battBuff, strlen(battBuff), 1, 0)) Serial.println(F("Failed to publish!")); // Send battery level

      // Note the command below may error out if you're already subscribed to the topic!
      modem.MQTT_subscribe(SUB_feed, 1); // Topic name, QoS
      
      // Unsubscribe from topics if wanted:
      // modem.MQTT_unsubscribe(SUB_feed);

      // Enable MQTT data format to hex
      // modem.MQTT_dataFormatHex(true); // Input "false" to reverse

      // Disconnect from MQTT
      // modem.MQTT_connect(false);
    #endif

    //Only run the code below if you want to turn off the shield after posting data
    #ifdef turnOffShield
      // Disable GPRS
      // Note that you might not want to check if this was successful, but just run it
      // since the next command is to turn off the module anyway
      if (!modem.enableGPRS(false)) Serial.println(F("Failed to disable GPRS!"));

      // Turn off GPS
      if (!modem.enableGPS(false)) Serial.println(F("Failed to turn off GPS!"));
      
      // Power off the module. Note that you could instead put it in minimum functionality mode
      // instead of completely turning it off. Experiment different ways depending on your application!
      // You should see the "PWR" LED turn off after this command
      //  if (!modem.powerDown()) Serial.println(F("Failed to power down modem!")); // No retries
      counter = 0;
      while (counter < 3 && !modem.powerDown()) { // Try shutting down 
        Serial.println(F("Failed to power down modem!"));
        counter++; // Increment counter
        delay(1000);
      }
    #endif
    
    // Alternative to the AT command method above:
    // If your modem has a PWRKEY pin connected to your MCU, you can pulse PWRKEY
    // LOW for a little bit, then pull it back HIGH, like this:
    //  digitalWrite(PWRKEY, LOW);
    //  delay(600); // Minimum of 64ms to turn on and 500ms to turn off for modem 3G. Check spec sheet for other types
    //  delay(1300); // Minimum of 1.2s for SIM7000
    //  digitalWrite(PWRKEY, HIGH);
    
    // Shut down the MCU to save power
    #ifndef samplingRate
      Serial.println(F("Shutting down..."));
      delay(5); // This is just to read the response of the last AT command before shutting down
      MCU_powerDown(); // You could also write your own function to make it sleep for a certain duration instead
    #else
      // The following lines are for if you want to periodically post data (like GPS tracker)
      // Non-blocking delay until next post. Read incoming MQTT subscribed topic messages, if any
      Serial.print(F("Waiting for ")); Serial.print(samplingRate); Serial.println(F(" seconds\r\n"));

      firstTime = false;
      timer = millis(); // Reset timer at the end

      // Only run the initialization again if the module was powered off
      // since it resets back to 115200 baud instead of 4800.
      #ifdef turnOffShield
        modem.powerOn(PWRKEY); // Powers on the module if it was off previously
        moduleSetup();
      #endif
        
    #endif
  }

  #ifdef PROTOCOL_MQTT
      checkSubscription(); // Check for incoming MQTT subscription data
  #endif
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

// Read the module's power supply voltage
float readVcc() {
  // Read battery voltage
  if (!modem.getBattVoltage(&battLevel)) Serial.println(F("Failed to read batt"));
  else Serial.print(F("battery = ")); Serial.print(battLevel); Serial.println(F(" mV"));

  // Read LiPo battery percentage
//  if (!modem.getBattPercent(&battLevel)) Serial.println(F("Failed to read batt"));
//  else Serial.print(F("BAT % = ")); Serial.print(battLevel); Serial.println(F("%"));

  return battLevel;
}

bool netStatus() {
  while(modem.available()) { modem.read(); }
  delay(100);
  
  uint8_t n = modem.getNetworkStatus();
  
  Serial.print(F("Network status ")); Serial.print(n); Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

  if (!(n == 1 || n == 5)) return false;
  else return true;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
#ifdef PROTOCOL_MQTT
  void checkSubscription() {
    // --- RUN UNCONDITIONAL CHARACTER HARVESTING (NON-BLOCKING) ---
    while (modem.available()) {
      char c = modem.read();
      
      // Asynchronously catch characters, filtering out row terminators
      if (c != '\r' && c != '\n' && replyidx < (sizeof(replybuffer) - 1)) {
        replybuffer[replyidx++] = c;
      } 
      // When a complete line arrives, execute the built-in parser block
      else if (c == '\n' && replyidx > 0) {
        replybuffer[replyidx] = '\0'; // Seal the C-string string safely
        
        // We got an MQTT message! Parse the topic and message
        // Format: +SMSUB: "topic_name","message"
        if (strstr(replybuffer, "+SMSUB:") != NULL) {
          Serial.println(F("\n*** Received MQTT message! ***"));
          // Serial.println(replybuffer); // Debug
          
          char *p = strtok(replybuffer, ",\"");
          char *topic_p = strtok(NULL, ",\"");
          strtok(NULL, "\""); // Skip intermediate comma and quote
          char *message_p = strtok(NULL, ",\"");
          
          Serial.print(F(" Topic: ")); Serial.println(topic_p);
          Serial.print(F(" Message: ")); Serial.println(message_p);

          // Do something with the message
          // For example, if the topic was "command" and we received "on", turn on an LED!
          if (strstr(topic_p, "command") != NULL) {
            if (strcmp(message_p, "on") == 0) {
              Serial.println(F("Turning on LED!"));
              digitalWrite(LED, HIGH);
            }
            else if (strcmp(message_p, "off") == 0) {
              Serial.println(F("Turning off LED!"));
              digitalWrite(LED, LOW);
            }
          }
        }
        memset(replybuffer, 0, sizeof(replybuffer));
        replyidx = 0; // Clear index for the next incoming line chunk
      }
    }
  }
#endif

// Turn off the MCU completely. Can only wake up from RESET button
// However, this can be altered to wake up via a pin change interrupt
void MCU_powerDown() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA = 0; // Turn off ADC
  power_all_disable ();  // Power off ADC, Timer 0 and 1, serial interface
  sleep_enable();
  sleep_cpu();
}

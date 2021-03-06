/*
originally Created by probonopd
Version 5-WM2 wooby6

  Implements a subset of the iTach API Specification Version 1.5
  http://www.globalcache.com/files/docs/API-iTach.pdf
  Tested with
  - Anymote Itach
**************************
CHANGELOG
  fork of ESP8266iTachEmulatorGUI
  WifiManager version
  added OTA
  Removed unneeded reciever code
  removed unneeded RCswitch
  Removed unneeded LIRC
  Removed debug telnet server, converted DebugSend back to serial for easier debugging
  removed unnessary raw ir code conversion
  updated code to work with IRremoteESP8266 v2.1.1
  
TODO
revert serial to DebugSend tor free up pins for esp-01
add reciver code back (sent over Debug)
add static IP, https://github.com/alexkirill/WiFiManager already has this but it dosn't work on reboot

Circuit
  reffer to google "ESP infared"
  # can also be used with an IR repeater via a TRRS/audio Jack
  be sure to check the pinout of your repeater before doing this

  #Im using a cheap unbranded china one with 3 reciver jacks, 6 Emitter jacks, USB/12v power
  with the reciver end being wired with Tip - DATA, Ring - 5v, Sleeve - GND
  my circuit only uses data & GND
*/

//	PIN SETUP

//nodemcu- 0=D3,1=U0TX,2=D4/LED/U1TX,4=D2,3=U0RX,5=D1,6=CLK,7=SD0,8=SD1,9=SD2,10=SD3,11=CMD,12=D6,13=D7,14=D5,15=D8,16=D0
//Wemos D1mini- 0=D3,1=U0TX,2=D4/LED/U1TX,4=D2,3=U0RX,5=D1,12=D6,13=D7,14=D5,15=D8,16=D0
//esp-01- 1=TXl/LED,2=GPIO2/tx2,0=GPIO0/spi,3=RX

// Set IR Blaster Pin
#define infraredLedPin 0 //esp-01


//	LED
//#define PIN_LED 2  //uncomment enable nodemcu/Wemos_LED = 2
#define LED_invert 2 // uncomment this for LED that have a inverted singal. also for an ESP-01 LED is attached to TX line so cant use TX at the same time
#define HOSTNAME "Wifi2IR-ESP-"  //mdns hostname
//end setup

// For Wifi2IR + inital
#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
// For OTA
#include "ArduinoOTA.h"
// For DRD
#include <DoubleResetDetector.h>  //https://github.com/datacute/DoubleResetDetector
// For WifiManager

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/kentaylor/WiFiManager

// VAR - DRD
#define DRD_TIMEOUT 10 // Number of seconds after reset during which a subseqent reset will be considered a double reset.
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
bool initialConfig = false; // Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
// VAR - DRD //

//VAR - Wifi2IR
extern const String MYVAL;
IRsend irsend(infraredLedPin);
unsigned int irrawCodes[99];
int rawCodeLen; // The length of the code
int freq;
// A WiFi server for wifi2IR communication on port 4998
#define MAX_SRV_CLIENTS 10 // How many clients may connect at the same time
WiFiServer server(4998);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Interval for sending UDP Discovery Beacons
unsigned long previousMillis = 0;
const long interval = random(10000, 60000); // Send every 10...60 seconds like the original
// const long interval = 3000; // for debugging
// VAR - Wifi2IR //

// ESP SDK
extern "C" {
#include "user_interface.h"
}
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  
  //	S -	WiFiManager
  #ifdef PIN_LED
    pinMode(PIN_LED, OUTPUT);
    Serial.println("LED Confugured - Normal Mode");
  #endif
  #ifdef LED_invert
    pinMode(LED_invert, OUTPUT);
    Serial.println("LED Confugured - Inverted Mode");
  #endif
    //WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed
  if (WiFi.SSID()==""){
    Serial.println("We haven't got any access point credentials, so get them now");   
    initialConfig = true;
  }
  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    initialConfig = true;
  }
  if (initialConfig) {
    Serial.println("Starting configuration portal.");
    #ifdef PIN_LED
      digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
      Serial.println("LED ON");
    #endif
    #ifdef LED_invert
      digitalWrite(LED_invert, HIGH);
      Serial.println("LED ON");    
    #endif
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);

    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    #ifdef PIN_LED
      digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
      Serial.println("LED OFF");
    #endif
    #ifdef LED_invert
      digitalWrite(LED_invert, LOW);
      Serial.println("LED OFF");    
    #endif
    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }
  #ifdef PIN_LED
    digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
    Serial.println("LED OFF");
  #endif
  #ifdef LED_invert
    digitalWrite(LED_invert, LOW);
    Serial.println("LED OFF");    
  #endif
  WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
  unsigned long startedAt = millis();
  Serial.print("After waiting ");
  int connRes = WiFi.waitForConnectResult();
  float waited = (millis()- startedAt);
  Serial.print(waited/1000);
  Serial.print(" secs in setup() connection result is ");
  Serial.println(connRes);
  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("failed to connect, finishing setup anyway");
  } else{
    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());
  }
  //  wifi Connected
  //  S -  WiFiManager //
  //	S - OTA
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.setPassword("wifi2ir");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("\n OTA Ready");
  // S - OTA //

  // Start Wifi2IR server
  irsend.begin();
  server.begin();
  server.setNoDelay(true);

  Serial.print("\n Wifi2IR Ready!");
  Serial.print("\n ");
  // Send first UDP Discovery Beacon during setup; then do it periodically below
  sendDiscoveryBeacon();
}

// S - OTA //

String inData;

void loop() {
  // L - DRD 
  drd.loop();
  // L - DRD //
  
  // L - OTA
  ArduinoOTA.handle();
  // L - OTA //

  // L - Wifi2IR
  // Periodically send UDP Discovery Beacon
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    sendDiscoveryBeacon();
  }

  uint8_t i;

  // Check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      // Find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        // Serial.print("New client: "); // Serial.println(i);
        continue;
      }
    }
    // No free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  // Check Global Caché clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected())
    {
      while (serverClients[i].available() > 0)

      {
        char recieved = serverClients[i].read();
        if (recieved != '\n' && recieved != '\r') // Actually we get "\n" from IRScrutinizer
        {
          inData += recieved;
        }
        else
        {
          inData.trim(); // Remove extraneous whitespace; this is important
          //Serial.print("> ");
          //Serial.println(inData);

          if (inData == "getdevices") {
            send(i, "device,1,3 IR");
            send(i, "endlistdevices");
          }

          if (inData == "getversion")
            send(i, "1.0");
            
          if (inData == "getversion,0")
            send(i, "1.0");

          // To see what gets sent to the device, enable "Verbose" in the "Options" menu of IRScrutinizer

          if (inData.startsWith("sendir,"))
          {
            int numberOfStringParts = getNumberOfDelimiters(inData, ',');
            int numberOfIrElements = numberOfStringParts - 2;

            int Array[numberOfStringParts];
            char *tmp;
            int z = 0;
            tmp = strtok(&inData[0], ",");
            while (tmp) {
              Array[z++] = atoi(tmp);
              tmp = strtok(NULL, ",");
            }

            uint16_t RawArray[numberOfIrElements];
            for (int iii = 0; iii < numberOfIrElements; iii++)
            {
              RawArray[iii] = (Array[3 + iii]);
              //Serial.print(RawArray[iii]);
              //Serial.print(" ");
            }

            send(i, "completeir,1:1," + String(Array[2], DEC));
            irsend.sendGC(RawArray, numberOfIrElements);
            Serial.println("sent IR"); 
          }
          inData = ""; // Clear recieved buffer
        }
      }
    }
  }
  // L - Wifi2IR //
}
// CF - Wifi2IR 
void send(int client, String str)
{
  if (serverClients[client] && serverClients[client].connected()) {
    serverClients[client].print(str + "\r");
    // Serial.print("< ");
    // Serial.println(str + "\n");
    delay(1);
  }
}


// Count the number of times separator character appears in the text
int getNumberOfDelimiters(String data, char delimiter)
{
  int stringData = 0;        //variable to count data part nr
  String dataPart = "";      //variable to hole the return text
  for (int i = 0; i < data.length() - 1; i++) { //Walk through the text one letter at a time
    if (data[i] == delimiter) {
      stringData++;
    }
  }
  return stringData;
}


void sendDiscoveryBeacon()
{
  IPAddress ipMulti(239, 255, 250, 250); // 239.255.250.250
  unsigned int portMulti = 9131;      // local port to listen on
  // Serial.println("Sending UDP Discovery Beacon");
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientName = macToStr(mac);
  clientName.replace(":", "");
  Udp.beginPacket(ipMulti, portMulti);
  Udp.print("AMXB<-UUID=WF2IR_");
  Udp.print(clientName);
  Udp.print("><-SDKClass=Utility><-Make=GlobalCache><-Model=WF2IR><-Config-URL=http://");
  Udp.print(WiFi.localIP());
  Udp.print("><-Status=Ready>\r");
  // AMX beacons needs to be terminated by a carriage return (‘\r’, 0x0D)
  Udp.endPacket();
}

// Convert the MAC address to a String
// This kind of stuff is why I dislike C. Taken from https://gist.github.com/igrr/7f7e7973366fc01d6393
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

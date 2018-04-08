/*
  This a simple example of the aREST UI Library for the ESP8266.
  See the README file for more details.

  Written in 2014-2016 by Marco Schwartz under a GPL license.
*/

// Import required libraries
#include <OneWire.h>
#include "DHTesp.h"
#include <WiFi.h>
#include <aREST.h>
#include <aREST_UI.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "SSD1306.h"

DHTesp dhtKitchen;
// Initialize the OLED display using Wire library
SSD1306  display(0x3c,5, 4);

IPAddress ip(192, 168, 1, 116);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

#define ONE_WIRE_BUS 15   // was 3
#define HeatingRelayPin  12  // this is the digital pin controlling the heating relay
#define KitchenTempPin 13  // this is the sensor in the kitchen

// Create aREST instance
aREST_UI rest = aREST_UI();

// WiFi parameters
const char* ssid = "TP-LINK_A005D1";
const char* password = "137KincoraRoad";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
int settemp = 0;
float KitchenTemp = 17;
float KitchenHumid = 67;


int ledControl(String command);

void setup(void) {
  // Start Serial
  Serial.begin(115200);

  dhtKitchen.setup(KitchenTempPin); 
  pinMode(HeatingRelayPin, OUTPUT);

  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  // Set the title
  rest.title("Heating");

  // Create button to control pin 5
  //rest.label("Digital Pin 5");
  rest.button(5);

  // Init variables and expose them to REST API
  settemp = 15;
  rest.variable("settemp",&settemp);
  rest.variable("KitchenTemp",&KitchenTemp);
  rest.variable("KitchenHumid",&KitchenHumid);

  // Labels
  rest.label("KitchenTemp");
  rest.label("KitchenHumid");
  rest.label("settemp");

  // Function to be exposed
  rest.function("led", ledControl);
  rest.function("DoSetTemp",DoSetTemp);

  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("esp8266");

  // Connect to WiFi
  WiFi.config(ip, gateway, subnet);  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
  ArduinoOTA.setPort(8084);
  ArduinoOTA.begin();

  showstatus();

}

void loop() {

  showstatus();

  ArduinoOTA.handle();
  
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }
  rest.handle(client);

}


void showstatus()
{  

display.clear();

int HeatingStatus = digitalRead(HeatingRelayPin);

 String statheat = "";
 display.setTextAlignment(TEXT_ALIGN_LEFT);
 display.setFont(ArialMT_Plain_16);

 if (HeatingStatus == 1) { statheat = "Off"; }
else { statheat = "On"; }

 String Ktemp = " --- ";
 String Khumid = " --- ";
 
 if(isnan(KitchenTemp)) { Ktemp = " --- ";  }
 else { Ktemp = String(KitchenTemp) + " C"; }

 if(isnan(KitchenHumid)) { Khumid = " --- ";  }
 else { Khumid = String(KitchenHumid) + "%"; }

  String myIP = WiFi.localIP().toString();
 String stemp = String(settemp);

 display.setFont(ArialMT_Plain_10);
 display.drawString(0, 0, "IP: " + myIP);
 display.setFont(ArialMT_Plain_16);
 display.drawString(0, 10,  "Temp: " +  Ktemp);
 display.drawString(0, 28, "Humid: " + Khumid);
 display.drawString(0, 46, "Set: " + stemp + " C" + " ("+ statheat+ ")" );
 //display.drawString(0, 55, "Status: " + statheat);
 display.display(); 
}

// Custom function accessible by the API
int DoSetTemp(String command) {
  
  // Get state from command
  int bcmd = command.toInt();
  
  settemp = bcmd;
  Serial.println("DoSetTemp: ");
  Serial.print(settemp);
  
  return 1;
}

int ledControl(String command) {
  // Print command
  Serial.println(command);

  // Get state from command
  int state = command.toInt();

  digitalWrite(5, state);
  return 1;
}

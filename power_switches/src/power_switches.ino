#include <Arduino.h>

/*
 * This is the code for the loft sensors and power switches
 *
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "env_vars.h"
#define DEBUG 1
#define PRINTDEBUG(STR) \
  {  \
    if (DEBUG) Serial.println(STR); \
  }

//  static const uint8_t D0   = 16;
//  static const uint8_t D1   = 5;
//  static const uint8_t D2   = 4;
//  static const uint8_t D3   = 0;
//  static const uint8_t D4   = 2;
//  static const uint8_t D5   = 14;
//  static const uint8_t D6   = 12;
//  static const uint8_t D7   = 13;
//  static const uint8_t D8   = 15;
//  static const uint8_t D9   = 3;
//  static const uint8_t D10  = 1;
const int POWER[] = { 0, 2, 14, 12 };
// boolean ontest[] = { 0 , 0 };

String state[] = { "OPEN", "CLOSED"};
#define membersof(x) (sizeof(x) / sizeof(x[0]))
const int pinCount = membersof(POWER);
int powerInt;
boolean ontest[pinCount];
String clientName;
String powerTopicName;
String statePublish;

const char* ssid     = MY_SSID;
const char* password = MY_PWD;
const char* mqtt_server = MY_MQTT_HOST;
const char* mqtt_user = MY_MQTT_USER;


WiFiClient espClient;
PubSubClient mqttclient(mqtt_server, 1883, 0, espClient);
//   unsigned long keepalivetime=0;
//   unsigned long MQTT_reconnect=0;
long lastMsg = 0;
char msg[100];
int LedPin = D1;

//////////// Start of wifi function ///////////////
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  PRINTDEBUG();
  PRINTDEBUG("Connecting to ");
  PRINTDEBUG(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    PRINTDEBUG(".");
  }
  PRINTDEBUG("Debug mode on"); PRINTDEBUG(""); PRINTDEBUG("WiFi connected"); PRINTDEBUG("IP address: "); PRINTDEBUG(WiFi.localIP());
}
///////// End of wifi setup ///////

////// Use MAC Address to create client and MQTT topic names /////
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
  }
  return result;
 }

////////////////////////// MQTT callback ////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {

  PRINTDEBUG("Received mqtt data");
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
//  String msgString = String(length);
   int i = 0;
  for(i=0; i<length; i++) {
    msg[i] = payload[i];

  }
  msg[i] = '\0';
  String msgString = String(msg);
  String msgStringN = String(msg[5]);


  // Free the memory
  free(p);


//////// Power outlet section to control relays ///////
if (msgString.indexOf("POWER") >= 0){
       power_loop(msgString);
    }

/////// End of power section, needs cleaining up ///////////////
  }
//////////////// end of callback - very messy //////////////////

//////////////// MQTT reconnect //////////////////////////
void reconnect() {
  while (!espClient.connected()) {
    PRINTDEBUG("Attempting MQTT connection...");
    if (mqttclient.connect((char*) clientName.c_str(), mqtt_user, MY_MQTT_PWD)) {
      PRINTDEBUG("connected");
      String subTopicName;
      uint8_t mac[6];
      WiFi.macAddress(mac);
      subTopicName += macToStr(mac);
      // sub_topic = clientName;
      PRINTDEBUG("subTopicNamename : ");
      PRINTDEBUG(subTopicName);
      mqttclient.subscribe((char*) subTopicName.c_str());
      mqttclient.publish("/register", (char*) subTopicName.c_str());
    } else {
      PRINTDEBUG("failed, rc=");
      PRINTDEBUG(mqttclient.state());
      PRINTDEBUG(" try again in 5 seconds");
      delay(5000);
    }
  }
}
///////////////// End of MQTT reconnect ///////////////

//////////////////Setup//////////////////////////
void setup() {
  if (DEBUG) Serial.begin(115200);
  setup_wifi();
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);
  reconnect();
  for(int count = 0; count < pinCount; count++) {
  pinMode(POWER[count], OUTPUT);
  digitalWrite(POWER[count], HIGH);
  ontest[count] = 0;
}
//  ProbeName += deviceToStr(Probe01);
//  PRINTDEBUG(ProbeName);
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
//  PRINTDEBUG("Number of elements in array");
//  PRINTDEBUG(COMMAND_QUEUE_MAX);
  // mqttclient.subscribe((char*)  clientName.c_str(), 0);



}

////////////////// End of Setup ///////////////


///////////////// Start of loop() ///////////////
void loop() {

//  if (millis() - Timer1 > 6000) {
//      PRINTDEBUG("MQTT Reconnects and publishes every 600 secs");
if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();
 yield();  // or delay(0);
  }
////////////// End of loop() //////////////////

////////////// Start of power loop ///////////
void power_loop(String msgString) {
PRINTDEBUG("Entering power loop");
int powerInt = atoi(&msgString[5]);
    if ((powerInt <= pinCount) && (powerInt > 0)) {
PRINTDEBUG(powerInt);
powerInt--;
if (ontest[powerInt] == 1) {
    statePublish = state[0];
  }
  if (ontest[powerInt] == 0) {
    statePublish = state[1];
  }
powerTopicName = clientName;
powerTopicName += "/";
powerTopicName += msgString;
  digitalWrite(POWER[powerInt], ontest[powerInt]);
  mqttclient.publish((char*) powerTopicName.c_str(), (char*) statePublish.c_str());
  ontest[powerInt] = !ontest[powerInt];
}
else {
  mqttclient.publish((char*) powerTopicName.c_str(), "Error: Requested value out of range");
}
}
////////////// End of power loop ///////////

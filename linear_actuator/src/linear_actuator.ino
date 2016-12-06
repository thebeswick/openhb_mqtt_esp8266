#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #define PROTOCOL_NAMEv311
#include "env_vars.h"
const char* ssid     = MY_SSID;
const char* password = MY_PWD;
const char* mqtt_server = MY_MQTT_HOST;
const char* mqtt_user = MY_MQTT_USER;
const char* mqtt_pwd = MY_MQTT_PWD;
#define DEBUG 1
#define PRINTDEBUG(STR) \
  {  \
    if (DEBUG) Serial.println(STR); \
  }
// int ledState = HIGH;
const int buttonPin = D2;
const char MirrorUp = D5;
const char MirrorDown = D6;  // Blue cables tags
const char MotorPinA = D3;
const char MotorPinB = D4;
boolean MirrorPos; //closed_down = 0 open_up = 1
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
int UpState;
int DownState;
String clientName;
String powerTopicName;
String statePublish;
String subTopicName;
WiFiClient espClient;
PubSubClient mqttclient(mqtt_server, 1883, 0, espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

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

  PRINTDEBUG("");
  PRINTDEBUG("WiFi connected");
  PRINTDEBUG("IP address: ");
  PRINTDEBUG(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  PRINTDEBUG("Message arrived [");
  PRINTDEBUG(topic);
  PRINTDEBUG("] ");
  for (int i = 0; i < length; i++) {
    PRINTDEBUG((char)payload[i]);
  }
  PRINTDEBUG();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    actuate();
 //   digitalWrite(MotorPinA, LOW);
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  //  digitalWrite(MotorA, HIGH);
  }

}
////// Use MAC Address to create client and MQTT topic names /////
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
  }
  return result;
 }

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    PRINTDEBUG("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect("tvlift", mqtt_user, mqtt_pwd)) {
      PRINTDEBUG("connected");

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
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(MirrorUp, INPUT);
  pinMode(MirrorDown, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(MotorPinA, OUTPUT);
  pinMode(MotorPinB, OUTPUT);
  digitalWrite(MotorPinA, LOW);
  digitalWrite(MotorPinB, LOW);
  DownState = digitalRead(MirrorDown);
  UpState = digitalRead(MirrorUp);
  Serial.begin(115200);
  setup_wifi();
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);
  clientName += "esp8266-";
uint8_t mac[6];
WiFi.macAddress(mac);
clientName += macToStr(mac);
}

void loop() {

  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();
  // read the state of the switch into a local variable:
int reading = digitalRead(buttonPin);

// check to see if you just pressed the button
// (i.e. the input went from LOW to HIGH),  and you've waited
// long enough since the last press to ignore any noise:

// If the switch changed, due to noise or pressing:
if (reading != lastButtonState) {
  // reset the debouncing timer
  lastDebounceTime = millis();
}

if ((millis() - lastDebounceTime) > debounceDelay) {
  // whatever the reading is at, it's been there for longer
  // than the debounce delay, so take it as the actual current state:

  // if the button state has changed:
  if (reading != buttonState) {
    buttonState = reading;

    // only toggle the LED if the new button state is HIGH
    if (buttonState == HIGH) {
      // this is the mirror code should go
      // ledState = !ledState;
      mqttclient.publish((char*) subTopicName.c_str(), "1");
    }

}
}

// set the LED:
// digitalWrite(ledPin, ledState);

// save the reading.  Next time through the loop,
// it'll be the lastButtonState:
lastButtonState = reading;

}

void actuate() {
      DownState = digitalRead(MirrorDown);
      UpState = digitalRead(MirrorUp);
      PRINTDEBUG("Up pos = ");
      PRINTDEBUG(UpState);
      PRINTDEBUG("Down pos = ");
      PRINTDEBUG(DownState);
// This part will test states and open mirror if true
if (UpState == 0 && DownState == 0) {
  PRINTDEBUG("Cannot determine state of hall effect sensors, check they are connected");
  PRINTDEBUG("one or other of the sensors should be 1 or 0 or 1 and 1 if the mirror is halfway");
  mqttclient.publish("mmotw/state", "sensor error");
}
if ((DownState == 0) || (UpState == 1 && DownState == 1)){
    PRINTDEBUG("moving to Up position");

  while (UpState == 1) {

    UpState = digitalRead(MirrorUp);
 //   PRINTDEBUG(DownState);
    digitalWrite(MotorPinA, LOW);
    digitalWrite(MotorPinB, HIGH);
    digitalWrite(BUILTIN_LED, LOW);
    yield();
  }
  digitalWrite(MotorPinA, LOW);
  digitalWrite(MotorPinB, LOW);
  mqttclient.publish("mmotw/state", "OPEN");
  PRINTDEBUG("Up Done");
  return;
//  DownState = digitalRead(MirrorDown);

}
// This part will test states and close mirror if true
if (UpState == 0) {
     PRINTDEBUG("Down");
  while (DownState == 1) {
    DownState = digitalRead(MirrorDown);
 //   PRINTDEBUG(UpState);
    digitalWrite(MotorPinA, HIGH);
    digitalWrite(MotorPinB, LOW);
    digitalWrite(BUILTIN_LED, LOW);
    yield();
  }
  digitalWrite(MotorPinA, LOW);
  digitalWrite(MotorPinB, LOW);
  mqttclient.publish("mmotw/state", "CLOSED");
  PRINTDEBUG("Down Done");
  UpState = digitalRead(MirrorUp);
  return;

}

      ledState = !ledState;
      PRINTDEBUG("Exited loop");
      digitalWrite(MotorPinA, LOW);
      digitalWrite(MotorPinB, LOW);
      digitalWrite(BUILTIN_LED, HIGH);
      return;
    }  // if reading function end

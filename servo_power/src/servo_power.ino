#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>
// #include "DHT.h"
#include <Servo.h>
#include "env_vars.h"
// extern "C" {
//#include "user_interface.h"

// }
#define DEBUG 1
#define PRINTDEBUG(STR) \
  {  \
    if (DEBUG) Serial.println(STR); \
  }
int ledState = HIGH;
const char FanPin = D3;
const char ServoPin1 = D4;
const char ServoPin2 = D5;
Servo myservo1;
Servo myservo2;
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
// const int servoPins[] = { 5, 4 };

int ServoState;
int pos = 0;

// Update these with values suitable for your network.

const char* ssid = MY_SSID;
const char* password = MY_PWD;
const char* mqtt_server = MY_MQTT_HOST;
const char* mqtt_user = MY_MQTT_USER;
const char* mqtt_pwd = MY_MQTT_PWD;


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
String clientName;

void setup_wifi() {
//  wifi_set_sleep_type(NONE_SLEEP_T);
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
  PRINTDEBUG("Debug mode on");
  PRINTDEBUG("");
  PRINTDEBUG("WiFi connected");
  PRINTDEBUG("IP address: ");
  PRINTDEBUG(WiFi.localIP());
}
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    // if (i < 5)
    //   result += ':';
  }
  return result;
}

int actuate() {
 myservo1.attach(ServoPin1);
 myservo2.attach(ServoPin2);
 if(ServoState == 0) {
   PRINTDEBUG("Open");
 for (pos = 0; pos <= 60; pos += 1) { // goes from 0 degrees to 180 degrees
   // in steps of 1 degree
  myservo1.write(pos);
  myservo2.write(pos);
        // tell servo to go to position in variable 'pos'
   yield();
   delay(15); // waits 15ms for the servo to reach the position
 }
 digitalWrite(FanPin, LOW);
 client.publish("ventState", "OPEN");
 }
if(ServoState == 1) {
   PRINTDEBUG("Close");
 for (pos = 60; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
   myservo1.write(pos);              // tell servo to go to position in variable 'pos'
   yield();
   delay(15);                       // waits 15ms for the servo to reach the position
 }
 digitalWrite(FanPin, HIGH);
 client.publish("ventState", "CLOSED");
 }
  myservo1.detach();
  myservo2.detach();
  ServoState = !ServoState;
  return ServoState;
}


void callback(char* topic, byte* payload, unsigned int length) {
/// From standard code //

  PRINTDEBUG("Received mqtt data");
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
   int i = 0;

  for(i=0; i<length; i++) {
    msg[i] = payload[i];
  }
  msg[i] = '\0';
  String msgString = String(msg);

  // Free the memory
  free(p);



// End Standard code

  // Switch on the LED if an 1 was received as first character

  if (msgString.equals("ventilate")) {
    digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    actuate();
    // ServoState = !ServoState;
    PRINTDEBUG("ServoState = ");
    PRINTDEBUG(ServoState);
 //   digitalWrite(MotorPinA, LOW);
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  //  digitalWrite(MotorA, HIGH);
  }

}

void reconnect() {
  // Loop until we're reconnected

  while (!client.connected()) {
    PRINTDEBUG("Attempting MQTT connection...");
    // Attempt to connect
//    if (client.connect("ServoClient", mqtt_user, mqtt_pwd)) {
if (client.connect((char*) clientName.c_str(), mqtt_user, mqtt_pwd)) {
      PRINTDEBUG("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // client.publish("test", (char*) ProbeName.c_str());
      // ... and resubscribe
      client.subscribe("Servos/Bedrooms");
    } else {
      PRINTDEBUG("failed, rc=");
      PRINTDEBUG(client.state());
      PRINTDEBUG(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  if (DEBUG) Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(FanPin, OUTPUT);
  ServoState = 0;

  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  // clientName += String(micros() & 0xff, 16);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  }

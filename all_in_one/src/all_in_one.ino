#include <Arduino.h>

/*
 * This is the code for the loft sensors and power switches
 *
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <macToStr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <Servo.h>
#include "env_vars.h"
#define DEBUG 1
#define PRINTDEBUG(STR) \
  {  \
    if (DEBUG) Serial.println(STR); \
  }
#define DHTPIN D5
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

#define ONE_WIRE_BUS D6
// #define POWER1 D1
// #define POWER2 D2
const int POWER[] = { 5, 4, 0, 2 };
String state[] = { "OPEN", "CLOSED"};
#define membersof(x) (sizeof(x) / sizeof(x[0]))
const int pinCount = membersof(POWER);
int powerInt;
boolean ontest[pinCount];
String clientName;
String powerTopicName;
String statePublish;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temp = 0.0;
float tempC;

const char* ssid     = MY_SSID;
const char* password = MY_PWD;
const char* mqtt_server = MY_MQTT_HOST;
const char* mqtt_user = MY_MQTT_USER;
const byte* temp_device_address;
String ProbeName;
String dhtTopicName;
int sensor_count;
/// Servo variables
const char ServoPin1 = D2;
const char ServoPin2 = D3;
Servo myservo1;
Servo myservo2;
int ServoState;
int pos = 0;
String servoTopic;
/// End of Servo Variables /////
WiFiClient espClient;
PubSubClient mqttclient(mqtt_server, 1883, 0, espClient);
//   unsigned long keepalivetime=0;
//   unsigned long MQTT_reconnect=0;
long lastMsg = 0;
char msg[100];
int LedPin = D1;
boolean onoff1 = 0;
boolean onoff2 = 0;
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
  PRINTDEBUG("Debug mode on");
  PRINTDEBUG("");
  PRINTDEBUG("WiFi connected");
  PRINTDEBUG("IP address: ");
  PRINTDEBUG(WiFi.localIP());
}
///////// End of wifi setup ///////
////// creates a string from the MAC address, useful for all sorts of things
// String macToStr(const uint8_t* mac)
// {
//  String result;
//  for (int i = 0; i < 6; ++i) {
//    result += String(mac[i], 16);
//  }
//  return result;
// }
/////// End of MAC string ////////
///// Used for creating dallas temp topics ///////
String deviceToStr(const uint8_t* Probe01)
{
  String result;
  result += clientName;
  result += '/';
  for (int i = 0; i < 6; ++i) {
    result += String(Probe01[i], 16);
  //  if (i < 5)
  //    result += ':';

  }
  PRINTDEBUG(result);
  return result;
}
String servoTopicStr()
{
  String result;
  result += clientName;
  result += '/';
  result += "servo";
  PRINTDEBUG(result);
  return result;
}
//// End of create device string /////
/// Servo actuate function
int actuate(String msgString) {
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
 // digitalWrite(FanPin, LOW);
 mqttclient.publish((char*) servoTopic.c_str(), "OPEN");
 }
if(ServoState == 1) {
   PRINTDEBUG("Close");
 for (pos = 60; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
   myservo1.write(pos);              // tell servo to go to position in variable 'pos'
   yield();
   delay(15);                       // waits 15ms for the servo to reach the position
 }
// digitalWrite(FanPin, HIGH);
 mqttclient.publish((char*) servoTopic.c_str(), "CLOSED");
 }
  myservo1.detach();
  myservo2.detach();
  ServoState = !ServoState;
  return ServoState;
}
////////////////////////// callback ////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {

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

//  String topicString = String(topic);
//  if (topic == "ds18b20") {

    if(msgString.equals("ds18c20"))
    {
      int c = 0;
      for(c=0; c<sensor_count; c++) {
        sensors.requestTemperatures();
        tempC = sensors.getTempCByIndex(c);
        DeviceAddress tempAddress;
        sensors.getAddress(tempAddress, c);
        ProbeName = deviceToStr(tempAddress);
        PRINTDEBUG(ProbeName);
      String tempC_str;
      char dstemp_topic[20];
      char tempC_msg[10];
      tempC_str = String(tempC); //converting ftemp (the float variable above) to a string
      tempC_str.toCharArray(tempC_msg, tempC_str.length() + 1);  //packaging up the data to publish to mqtt whoa..
      mqttclient.publish((char*) ProbeName.c_str(), tempC_msg);
      }
      PRINTDEBUG("ds18c20 function executed");
    }
  if(msgString.equals("dht")) {
    dhtTopicName = clientName;
    dhtTopicName += "/dht";
    float humidity = dht.readHumidity();
    float dht_temp = dht.readTemperature();
    if (isnan(humidity) || isnan(dht_temp)) {
      PRINTDEBUG("Failed to read from DHT sensor!");
      return;
    }
    PRINTDEBUG("Humidity: ");
    PRINTDEBUG(humidity);
    PRINTDEBUG(" %\t");
    PRINTDEBUG("Temperature: ");
    PRINTDEBUG(dht_temp);
    PRINTDEBUG(" *C ");
    String dhtTopicNameTemp;
    dhtTopicNameTemp = dhtTopicName;
    dhtTopicNameTemp += "/temp";
    String temp_str;
    char temp1[10];
    temp_str = String(dht_temp); //converting ftemp (the float variable above) to a string
    temp_str.toCharArray(temp1, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa..
    mqttclient.publish((char*) dhtTopicNameTemp.c_str(), temp1);
    String dhtTopicNameHumidity;
    dhtTopicNameHumidity = dhtTopicName;
    dhtTopicNameHumidity += "/humidity";
    String humidity_str;
    char humidity_char[10];
    humidity_str = String(humidity); //converting ftemp (the float variable above) to a string
    humidity_str.toCharArray(humidity_char, humidity_str.length() + 1);
    mqttclient.publish((char*) dhtTopicNameHumidity.c_str(),humidity_char);
}
//////// Power outlet section to control relays ///////
if (msgString.indexOf("POWER") >= 0){
     power_loop(msgString);
   }

/////// End of power section /////////
if (msgString.equals("SERVO")){
     actuate(msgString);
   }
}
//////////////// end of callback //////////////////

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
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
 PRINTDEBUG("Starting Dallas Sensors")
 sensors.begin();
 sensor_count = sensors.getDeviceCount();
 int c = 0;
       for(c=0; c<sensor_count; c++) {
       }
PRINTDEBUG("DHTxx test!");
 dht.begin();
 servoTopic = servoTopicStr();
}

////////////////// End of Setup ///////////////


///////////////// Start of loop() ///////////////
void loop() {
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

/*
   Simpleton Sonoff Ceiling Light firmware with MQTT support
   Supports OTA update
   David Pye (C) 2016 GNU GPL v3
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define NAME "yourname"
#define SSID "yourssid"
#define PASS "yourwifipw"

//Defaults to DHCP, if you want a static IP, uncomment and
//configure below
//#define STATIC_IP
#ifdef STATIC_IP
IPAddress ip(192, 168, 0, 6);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
#endif

#define MQTT_SERVER "192.168.1.50"
#define MQTT_PORT 1883

#define OTA_PASS "OTA_PW"
#define OTA_PORT 8266

#define CMNDTOPIC1 "cmnd/" NAME "/light"
#define CMNDTOPIC2 "cmnd/group/upstairs_lights"
#define STATUSTOPIC "status/" NAME "/light"

//#define BUTTON_PIN 0 //To use the onboard button
#define BUTTON_PIN 14 //To use the extra GPIO on the programming header to wire to the domestic light switch
#define RELAY_PIN 12
#define LED_PIN 13

volatile int desiredRelayState = 0;
volatile int relayState = 0;

unsigned long lastMQTTCheck = 0;

//Used by the GPIO debounce code (https://www.arduino.cc/en/Tutorial/Debounce)
bool lastButtonState = false;
bool buttonState = false;
unsigned long lastDebounceTime = -1;

WiFiClient espClient;
PubSubClient client(espClient);

void initWifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
#ifdef STATIC_IP
  WiFi.config(ip, gateway, subnet);
#endif

  WiFi.begin(SSID, PASS);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else Serial.println("WiFi not connected...");
}

void checkMQTTConnection() {
  Serial.print("Establishing MQTT connection: ");
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    if (client.connect(NAME)) {
      Serial.println("connected");
      client.subscribe(CMNDTOPIC1);
      client.subscribe(CMNDTOPIC2);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
    }
  }
  if (client.connected()) {
    //Keep LED off, it's in the ceiling!
  }
  else {
    digitalWrite(LED_PIN, HIGH);
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  if (!strcmp(topic, CMNDTOPIC1) || !strcmp(topic, CMNDTOPIC2)) {
    if ((char)payload[0] == '1' || ! strcasecmp((char *)payload, "on")) {
        desiredRelayState = 1;
    }
    else if ((char)payload[0] == '0' || ! strcasecmp((char *)payload, "off")) {
      desiredRelayState = 0;
    }
    else if ( ! strcasecmp((char *)payload, "toggle")) {
      desiredRelayState = !desiredRelayState;
    }
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH); //LED off.

  Serial.begin(115200);
  Serial.println("Init");
  initWifi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(MQTTcallback);

  //OTA setup
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(NAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();

  //Connect to MQTT server.
  checkMQTTConnection();
  lastMQTTCheck = millis();
}

void loop() {
  if (millis() - lastMQTTCheck >= 5000) {
    checkMQTTConnection();
    lastMQTTCheck = millis();
  }

  client.loop(); //Process MQTT client events

  //Handler for over-the-air SW updates.
  ArduinoOTA.handle();

  //Read and debounce the button pin signal.
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > 100) {
    if (reading != buttonState) {
      buttonState = reading;
      desiredRelayState = !desiredRelayState;
    }
  }
  lastButtonState = reading;

  if (relayState != desiredRelayState) {
      digitalWrite(RELAY_PIN, desiredRelayState);
      relayState = desiredRelayState;
      client.publish(STATUSTOPIC, relayState == 0 ? "0" : "1");
  }
  delay(50);
}

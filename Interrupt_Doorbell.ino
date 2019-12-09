#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//All config is now in the secrets file. file is in same directory and looks like: #define SECRET_SSID "Stringvalue"
#include "arduino_secrets.h"

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASSWORD;

const char* mqtt_server = SECRET_MQTT_IP;
const int   mqtt_port = SECRET_MQTT_PORT;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_password = SECRET_MQTT_PASSWD;
String mqttClient = "client";

const char* mqtt_topic = "SD18/Interrupt";
const char* mqtt_debug = "SD18/Debug";




const int buttonPin = 0;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin

// variables will change:
volatile int buttonState = 0;         // variable for reading the pushbutton status
//timer
unsigned long ts = 0, new_ts = 0, loopts = 0, new_loopts = 0; //timestamp

void publishMQTT(String topic, String incoming) {
  Serial.println("MQTT: " + mqttClient + ": " + topic + ":" + incoming);
  char charBuf[incoming.length() + 1];
  incoming.toCharArray(charBuf, incoming.length() + 1);

  char topicBuf[topic.length() + 1];
  topic.toCharArray(topicBuf, topic.length() + 1);
  client.publish(topicBuf, charBuf);
}

void publish_gated(String topic, String incoming) {
  new_loopts = millis();
  if (new_loopts - loopts > 10000) {
    loopts = new_loopts;
    Serial.println("MQTT: " + mqttClient + ": " + topic + ":" + incoming);
    char charBuf[incoming.length() + 1];
    incoming.toCharArray(charBuf, incoming.length() + 1);

    char topicBuf[topic.length() + 1];
    topic.toCharArray(topicBuf, topic.length() + 1);
    client.publish(topicBuf, charBuf);
  }
}
void setup_wifi() {
  delay(10);
  //Connect to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect using a random clientid
    String clientId = "Interrupt-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected as " + clientId);
      // Once connected, publish an announcement...
      publishMQTT(mqtt_debug, "hoi");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  setup_wifi();

  //Init MQTT Client
  client.setServer(mqtt_server, mqtt_port);
  // initialize the LED pin as an output:
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
  // Attach an interrupt to the ISR vector
  attachInterrupt(0, pin_ISR, CHANGE);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  //MQTT Loop
  new_ts = millis();
  if (new_ts - ts > 10000) {
    publishMQTT(mqtt_debug, "hoi");
    ts = new_ts;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void pin_ISR() {
  buttonState = digitalRead(buttonPin);
  publish_gated(mqtt_topic, "dring");
  digitalWrite(LED_BUILTIN, buttonState);

}

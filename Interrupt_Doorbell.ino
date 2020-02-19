#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//All config is now in the secrets file. file is in same directory and looks like: #define SECRET_SSID "Stringvalue"
#include "arduino_secrets.h"
#include <OneWire.h>


OneWire  ds(D4);  // on pin D4 (a 4.7K resistor is necessary)
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASSWORD;
const char* mqtt_server = SECRET_MQTT_IP;
const int   mqtt_port = SECRET_MQTT_PORT;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_password = SECRET_MQTT_PASSWD;
String mqttClient = "client"; //this will be overwritten when connecting.

// Things to change
const char* mqtt_topic = "SD18/Interrupt";
const char* mqtt_debug = "SD18/Debug";

const int buttonPin = 0;     // D3
const int ledPin =  13;      // the number of the LED pin
const int relayPin = D1; //relay



// variables will change:
volatile int buttonState = 0;         // variable for reading the pushbutton status
//timer
unsigned long ts = 0, new_ts = 0, loopts = 0, new_loopts = 0; //timestamps

//
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
    publishMQTT(topic, incoming);
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
      publishMQTT(mqtt_debug, "Checking in");

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

  //define our relay pin as output and pull it low.
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  setup_wifi();

  //Init MQTT Client
  client.setServer(mqtt_server, mqtt_port);
  // initialize the LED pin as an output:
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);
  // Attach an interrupt to the ISR vector
  attachInterrupt(buttonPin, pin_ISR, CHANGE);

  digitalWrite(LED_BUILTIN, HIGH);
}
//get Temp
void getTemp() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if ( !ds.search(addr))
  {
    ds.reset_search();
    delay(250);
    return;
  }


  if (OneWire::crc8(addr, 7) != addr[7])
  {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  // the first ROM byte indicates which chip
  switch (addr[0])
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

  }

  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  publishMQTT(mqtt_value, String(celsius));
  Serial.print(" Celsius, ");


}




void loop() {
  //MQTT Loop
  new_ts = millis();
  if (new_ts - ts > 10000) { //check in every ten secs.
    publishMQTT(mqtt_debug, "Checking In");
    ts = new_ts;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  buttonState = digitalRead(buttonPin);
  //Serial.println(String(buttonState));
}
bool isLow = false;
// will detect every change. When a button press occurs it will be LOW (0), so we're sending an MQTT message when that happens.
// I mimic the relay status with the button state (because else you'd only have the half of the chime)
// I use the gated function because I don't want 200 messages on every button press ;)
void pin_ISR() {
  buttonState = digitalRead(buttonPin);
  Serial.println(String(buttonState));
  digitalWrite(LED_BUILTIN, buttonState);
  if (buttonState == 1) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
    publish_gated(mqtt_topic, "Push");
  }


}

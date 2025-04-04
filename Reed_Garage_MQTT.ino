#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Define the pin connected to the reed switch.
// Adjust this pin to one available on your board.
#define REED_PIN D2

// WiFi credentials to connect to the stoplight's AP.
const char* ssid = "ESP8266_Stoplight";
const char* password = "12345678";

// MQTT broker details (set to your Mac's IP on the ESP8266 AP network).
// For example, if your Mac’s IP on this network is 192.168.4.2:
const char* mqtt_server = "192.168.4.4";
const int mqtt_port = 1884;

// MQTT topic for enabling/disabling the system.
const char* enable_topic = "system/enable";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

bool lastReedState = false;  // Track the previous reed state

void setup() {
  Serial.begin(115200);
  
  // Set up the reed switch pin.
  pinMode(REED_PIN, INPUT);
  // If you need a pull-up resistor (depending on your wiring), use:
  // pinMode(REED_PIN, INPUT_PULLUP);
  // Adjust the logic below if you use INPUT_PULLUP.
  
  // Connect to WiFi (the ESP8266 AP hosted by your stoplight).
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  
  // Configure MQTT client.
  client.setServer(mqtt_server, mqtt_port);
  
  // Read initial state of the reed switch.
  lastReedState = digitalRead(REED_PIN);
  Serial.print("Initial reed state: ");
  Serial.println(lastReedState);
}

void reconnect() {
  // Loop until we're reconnected to the MQTT broker.
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ReedSwitchClient-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected");
      // (Optional) Subscribe to topics if needed.
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - trying again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Read the current state of the reed switch.
  bool currentReedState = digitalRead(REED_PIN);
  
  // If the state has changed, publish the new state.
  if (currentReedState != lastReedState) {
    lastReedState = currentReedState;
    
    if (currentReedState) {
      Serial.println("Reed switch HIGH: System Enabled");
      client.publish(enable_topic, "1");  // Enable stoplight and sensor
    } else {
      Serial.println("Reed switch LOW: System Disabled");
      client.publish(enable_topic, "0");  // Disable stoplight and sensor
    }
  }
  
  delay(100);  // Small delay to help debounce the reed switch
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

// AP credentials for the ESP8266 stoplight device.
const char* apSSID = "ESP8266_Stoplight";
const char* apPassword = "12345678";

// MQTT broker details: set this to your Mac's IP address on the ESP8266 network.
// For example, if your Mac's IP is 192.168.4.2, use that.
const char* mqtt_server = "192.168.4.4";

const int RED_PIN = D1;
const int YELLOW_PIN = D2;
const int GREEN_PIN = D3;

WiFiClient espClient;
PubSubClient client(espClient);
Ticker redTicker;          // For red LED blinking
Ticker greenYellowTicker;  // For blinking green and yellow together
Ticker autoBlinkTicker;    // For auto blinking all LEDs (optional)

bool redState = LOW;       // State used for red blinking
bool autoBlinkState = LOW; // State used for auto blinking (all LEDs)
const unsigned long blinkInterval = 500; // Blink interval in milliseconds

// Global flag for system enable/disable.
bool systemEnabled = true;

// Toggle function for red LED blinking.
void toggleRed() {
  redState = !redState;
  digitalWrite(RED_PIN, redState);
}

// Toggle function for blinking green and yellow LEDs simultaneously.
void toggleGreenYellow() {
  static bool state = false;
  state = !state;
  digitalWrite(GREEN_PIN, state);
  digitalWrite(YELLOW_PIN, state);
}

// Toggle function for auto blinking all three LEDs (optional).
void toggleAll() {
  autoBlinkState = !autoBlinkState;
  digitalWrite(RED_PIN, autoBlinkState);
  digitalWrite(YELLOW_PIN, autoBlinkState);
  digitalWrite(GREEN_PIN, autoBlinkState);
}

// MQTT callback function – processes messages from subscribed topics.
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to a string.
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Process system enable/disable commands.
  if (strcmp(topic, "system/enable") == 0) {
    if (message == "1") {
      systemEnabled = true;
      Serial.println("System enabled");
    } else if (message == "0") {
      systemEnabled = false;
      Serial.println("System disabled");
      // Immediately turn off all LEDs.
      digitalWrite(RED_PIN, LOW);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
    }
    return;
  }
  
  // If system is disabled, ignore other commands.
  if (!systemEnabled) {
    Serial.println("System disabled, ignoring command.");
    return;
  }
  
  // Stop any active tickers before executing a new command.
  redTicker.detach();
  greenYellowTicker.detach();
  autoBlinkTicker.detach();

  // Handle stoplight commands based on topic.
  if (strcmp(topic, "stoplight/green_on") == 0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(YELLOW_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
  } else if (strcmp(topic, "stoplight/yellow_on") == 0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(YELLOW_PIN, HIGH);
  } else if (strcmp(topic, "stoplight/red_on") == 0) {
    digitalWrite(YELLOW_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, HIGH);
  } else if (strcmp(topic, "stoplight/red_blink_start") == 0) {
    redTicker.attach_ms(blinkInterval, toggleRed);
  } else if (strcmp(topic, "stoplight/red_blink_stop") == 0) {
    digitalWrite(RED_PIN, LOW);
  } else if (strcmp(topic, "stoplight/green_yellow_blink") == 0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(YELLOW_PIN, LOW);
    greenYellowTicker.attach_ms(blinkInterval, toggleGreenYellow);
  }
}

// Reconnect function: tries to reconnect to the MQTT broker if disconnected.
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected");
      // Subscribe to all necessary topics.
      client.subscribe("stoplight/green_on");
      client.subscribe("stoplight/yellow_on");
      client.subscribe("stoplight/red_on");
      client.subscribe("stoplight/red_blink_start");
      client.subscribe("stoplight/red_blink_stop");
      client.subscribe("stoplight/green_yellow_blink");
      client.subscribe("system/enable");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - trying again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize LED pins.
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);

  // Set up the ESP8266 as an Access Point.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword, 1, true);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());  // Typically 192.168.4.1

  // Set up the MQTT client.
  client.setServer(mqtt_server, 1884);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // (Optional) Add logic for auto blinking or other features if desired.
}
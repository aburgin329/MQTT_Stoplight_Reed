#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// HC-SR04 sensor pins (adjust as needed)
#define TRIG_PIN D1
#define ECHO_PIN D2

// WiFi credentials for connecting to the stoplight's AP.
const char* ssid = "ESP8266_Stoplight";      
const char* password = "12345678";

// MQTT broker details: set this to your Mac’s IP address on the ESP8266 network.
const char* mqtt_server = "192.168.4.4";

// Define stoplight states.
enum StoplightState {
  GREEN,
  YELLOW,
  RED,
  BACKUP,              // Flashing red (backup)
  GREEN_YELLOW_BLINK   // Blinking green & yellow
};

StoplightState currentState = GREEN;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Global flag for system enable/disable.
bool systemEnabled = true;

void setup() {
  Serial.begin(115200);
  
  // Setup sensor pins.
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Connect to the stoplight's WiFi network (AP mode).
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Setup MQTT client.
  client.setServer(mqtt_server, 1884);
  client.setCallback(callback);
}

long measureDistance() {
  // Clear the trigger pin.
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Trigger the sensor: send a 10-microsecond HIGH pulse.
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pulse duration in microseconds.
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance in centimeters.
  long distance = (duration * 0.0343) / 2;
  return distance;
}

void reconnect() {
  // Loop until reconnected to the MQTT broker.
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "HC-SR04Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected");
      // Subscribe to the system enable/disable topic.
      client.subscribe("system/enable");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - trying again in 5 seconds");
      delay(5000);
    }
  }
}

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
  
  // Handle system enable/disable commands.
  if (strcmp(topic, "system/enable") == 0) {
    if (message == "1") {
      systemEnabled = true;
      Serial.println("System enabled");
    } else if (message == "0") {
      systemEnabled = false;
      Serial.println("System disabled");
    }
  }
}

void sendStateCommand(StoplightState state) {
  // If system is disabled, do not send any commands.
  if (!systemEnabled) {
    Serial.println("System disabled, not sending command.");
    return;
  }
  
  const char* topic;
  
  // Map each state to the corresponding MQTT topic.
  switch(state) {
    case GREEN:
      topic = "stoplight/green_on";
      break;
    case YELLOW:
      topic = "stoplight/yellow_on";
      break;
    case RED:
      topic = "stoplight/red_on";
      break;
    case BACKUP:
      topic = "stoplight/red_blink_start";
      break;
    case GREEN_YELLOW_BLINK:
      topic = "stoplight/green_yellow_blink";
      break;
    default:
      return;
  }
  
  Serial.print("Publishing to topic: ");
  Serial.println(topic);
  
  // Publish a simple payload ("on") to the topic.
  client.publish(topic, "on");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Measure the distance using the HC-SR04.
  long distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // Define distance thresholds (in centimeters).
  StoplightState newState;
  if (distance == 0) {                     // Error or no valid measurement.
    newState = GREEN_YELLOW_BLINK;
  } else if (distance > 15) {              // Far away.
    newState = GREEN;
  } else if (distance > 10 && distance <= 15) {  // Getting close.
    newState = YELLOW;
  } else if (distance > 7 && distance <= 10) {   // Perfect distance (park).
    newState = RED;
  } else {                               // Too close: backup state.
    newState = BACKUP;
  }
  
  // If the state has changed, publish the appropriate MQTT command.
  if (newState != currentState) {
    currentState = newState;
    sendStateCommand(newState);
  }
  
  // Update every 500ms.
  delay(500);
}
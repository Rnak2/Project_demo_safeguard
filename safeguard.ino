#include <SPI.h>
#include <WiFiNINA.h>
#include <Servo.h>
#include <PubSubClient.h>
#include "secret.h"

//WiFi and MQTT 
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = "192.168.86.182"; 
const int mqtt_port = 1883; 

//IFTTT  
char HOST_NAME[] = "maker.ifttt.com";
String PATH_NAME_ON = "/trigger/Secure_guard/with/key/b0GGmp9DzmjuUjPq81sZ1I";

//MQTT topics
const char* commandTopic = "safeguard/command";
const char* alertTopic = "safeguard/alert";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo myServo; 
int servoPin = 9; 

//Ultrasonic sensor
const int trigPin = 7; 
const int echoPin = 8; 
long duration, distance;
bool safeMode = false; 
unsigned long lastCheckTime = 0; 
const unsigned long checkInterval = 200; 

// Buzzer Pin
const int buzzerPin = 6; 

void setup_wifi();
void connectToMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void lockSafe();
void unlockSafe();
void checkDistance();
void triggerIFTTTNotification(String PATH_NAME, String value1);
void activateBuzzer();
void deactivateBuzzer();

void setup_wifi() {
    Serial.begin(9600);
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts++;
        if (attempts >= 20) { 
            Serial.println("Failed to connect to WiFi.");
            return;
        }
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void connectToMqtt() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect("ArduinoClient")) {
            Serial.println("Connected to MQTT");
            if (mqttClient.subscribe(commandTopic)) {
                Serial.println("Subscribed to topic: safeguard/command");
            } else {
                Serial.println("Failed to subscribe to topic");
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0';
    String message((char*)payload);

    if (String(topic) == commandTopic) {
        if (message == "lock") {
            Serial.println("Received command: lock");
            lockSafe();
        } else if (message == "unlock") {
            Serial.println("Received command: unlock");
            unlockSafe();
        } else if (message == "toggle_safe_mode") {
            Serial.println("Received command: toggle_safe_mode");
            safeMode = !safeMode; 
            if (safeMode) {
                Serial.println("Safe mode is ON");
            } else {
                Serial.println("Safe mode is OFF");
            }
        }
    }
}

void lockSafe() {
    myServo.write(0); 
    Serial.println("Safe is locked");
    triggerIFTTTNotification(PATH_NAME_ON, "lock");
}

void unlockSafe() {
    myServo.write(90); 
    Serial.println("Safe is unlocked");
    triggerIFTTTNotification(PATH_NAME_ON, "unlock");
}

void checkDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 20000); 
    if (duration == 0) {
        Serial.println("No pulse received.");
    } else {
        distance = duration * 0.034 / 2;
        Serial.print("Distance: ");
        Serial.println(distance);
        if (distance < 20) { 
            Serial.println("Intruder detected! Sending IFTTT alert...");
            triggerIFTTTNotification(PATH_NAME_ON, "Intruder alert!");
            activateBuzzer(); 
        } else {
            deactivateBuzzer(); 
        }
    }
}

void triggerIFTTTNotification(String PATH_NAME, String value1) {
    if (wifiClient.connect(HOST_NAME, 80)) {
        String request = "GET " + PATH_NAME + "?value1=" + value1 + " HTTP/1.1\r\n" +
                         "Host: " + String(HOST_NAME) + "\r\n" +
                         "Connection: close\r\n\r\n";
        
        wifiClient.print(request);
        Serial.println("Sending message to IFTTT: " + request);

        unsigned long timeout = millis();
        while (!wifiClient.available()) {
            if (millis() - timeout > 5000) {
                Serial.println("Timeout");
                wifiClient.stop();
                return;
            }
        }

       
        while (wifiClient.available()) {
            char c = wifiClient.read();
            Serial.print(c);
        }

        wifiClient.stop();
        Serial.println("IFTTT request sent successfully.");
    } else {
        Serial.println("Failed to connect to IFTTT server.");
    }
}

void activateBuzzer() {
    digitalWrite(buzzerPin, HIGH); // Turn the buzzer on
}

void deactivateBuzzer() {
    digitalWrite(buzzerPin, LOW); // Turn the buzzer off
}

void setup() {
    Serial.begin(9600);
    setup_wifi();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection failed. Restarting...");
        while (true) { // Stop here to avoid running the rest of the code without WiFi
            delay(1000);
        }
    }
    pinMode(trigPin, OUTPUT); 
    pinMode(echoPin, INPUT); 
    pinMode(buzzerPin, OUTPUT);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    connectToMqtt();
    myServo.attach(servoPin); 
    deactivateBuzzer(); 
}

void loop() {
    if (!mqttClient.connected()) {
        connectToMqtt();
    }
    mqttClient.loop(); 

    // Check distance 
    if (safeMode && millis() - lastCheckTime >= checkInterval) {
        checkDistance();
        lastCheckTime = millis();
    }
}

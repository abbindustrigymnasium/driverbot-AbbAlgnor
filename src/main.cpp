#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <config.h>

// controll pins
#define MOTOR_OUTPUT_PIN D1
#define MOTOR_DIRECTION_PIN D3
#define SERVO_PIN D5
#define BATTERY_CONTROLL_PIN D4

// input pins
#define TACHO_PIN D7

// constants
#define MOTOR_RATIO 1 / 48
#define WHEEL_DIAMETER 36.0 // unit in mm
#define DIFFERENTIAL_RATIO 1.0 / 1.4

StaticJsonDocument<1024> doc;
unsigned long int LastPulse[3];
unsigned long int LastMeasure;

long int Rotations = 0;

bool tempDir;

Servo servo;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void IRAM_ATTR pulseCallback() {

  LastPulse[2] = LastPulse[1];
  LastPulse[1] = LastPulse[0];
  LastPulse[0] = micros() - LastMeasure;

  LastMeasure = micros();

  int tempDir = 0;
  tempDir += (LastPulse[0] < LastPulse[1]);
  tempDir += (LastPulse[1] < LastPulse[2]);

  if (tempDir - 1) {
    Rotations += tempDir - 1;
    Serial.printf("rot: %li, dir: %i\n", Rotations, tempDir - 1);
  }

  // Serial.printf("val: %8i, arr: [ %8li, %8li, %8li ]\n", tempDir,
  // LastPulse[0], LastPulse[1], LastPulse[2]);
}

void callback(char *topic, byte *payload, unsigned int length) {
  deserializeJson(doc, payload);
}

void setup() {
  Serial.begin(921600);

  pinMode(MOTOR_OUTPUT_PIN, OUTPUT);
  pinMode(MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(BATTERY_CONTROLL_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(TACHO_PIN), pulseCallback, FALLING);

  servo.attach(SERVO_PIN);

  WiFi.begin(ssid, password);

  int tries = 1;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(ssid);
    Serial.println(password);
    Serial.println();
    Serial.print("Connecting: X");
    Serial.println(tries);
    tries++;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  digitalWrite(BATTERY_CONTROLL_PIN, true);

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    Serial.println("Connecting to mqtt");
    digitalWrite(LED_BUILTIN, HIGH);

    if (mqttClient.connect(mqttClientName, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed With state: ");
      Serial.print(mqttClient.state());
      delay(1000);
    }
  }

  digitalWrite(LED_BUILTIN, LOW);

  mqttClient.publish(mqttPubTopic, "Hello from ESP8266");
  mqttClient.subscribe(mqttSubTopic);
}

void loop() {
  mqttClient.loop();

  tempDir = !tempDir;

  analogWrite(MOTOR_OUTPUT_PIN, 256);
  digitalWrite(MOTOR_DIRECTION_PIN, tempDir);
  Serial.println((double)Rotations * WHEEL_DIAMETER * 3.141 *
                 DIFFERENTIAL_RATIO * MOTOR_RATIO);

  delay(1000);
}

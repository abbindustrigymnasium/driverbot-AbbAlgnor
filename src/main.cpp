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
#define MOTOR_RATIO 1.0 / 48.0
#define WHEEL_DIAMETER 36.0 // unit in mm
#define DIFFERENTIAL_RATIO 1.0 / 1.4

// tickspeeds
#define TICK_SPEED 40        // main tickseed
#define STATUS_TICK_SPEED 100 // status post tickspeed

StaticJsonDocument<1024> doc;
StaticJsonDocument<1024> post;

unsigned long int LastPulse[3];
unsigned long int LastMeasure;
unsigned long int LastStatusUpdate;
unsigned long int LastTick;

long int P, I, D, Integral, PreviousError;
double LastDistance;

int Turning, ServoOffset;
int Speed, TargetSpeed;

long int Rotations;

double Distance = 0;

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
  }

  // Serial.printf("val: %8i, arr: [ %8li, %8li, %8li ]\n", tempDir,
  // LastPulse[0], LastPulse[1], LastPulse[2]);
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.println("mqtt recieved");
  deserializeJson(doc, payload);
  serializeJsonPretty(doc, Serial);
  if (doc["pid"]) {
    P = doc["pid"]["p"];
    I = doc["pid"]["i"];
    D = doc["pid"]["d"];
  }
  if (doc["Servo"]) {
    ServoOffset = doc["Servo"]["servoOffset"];
  }

  if (doc["drive"]) {
    int Forward = doc["drive"]["Forward"];
    int Backward = doc["drive"]["Backward"];
    Turning = doc["drive"]["Turning"];
    TargetSpeed = Forward - Backward;
    Serial.println("updated target speed");
  }
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

int pid(int current, int target, int P, int I, int D) {
  int error = target - current;
  Integral += error * (TICK_SPEED / 1000.0);
  int derivative = (error - PreviousError) / (TICK_SPEED / 1000.0);
  long int tempOutput = (P * error + I * Integral + D * derivative);
  int output = (tempOutput < 256) ? ((tempOutput > -256) ? tempOutput: -255): 255;

  Serial.printf(
      "current: %8i, target, %8i, output:  %8i, p: %8i, i: %8li, d: %8i\n",
      current, target, output, error * P, Integral * I, derivative * D);

  PreviousError = error;
  return output;
}

void loop() {
  mqttClient.loop();

  if ((millis() - LastTick) > TICK_SPEED) {
    LastTick = millis();

    Distance = ((double)Rotations * WHEEL_DIAMETER * 3.141 *
                DIFFERENTIAL_RATIO * MOTOR_RATIO) *
               -1;

    Speed = (Distance - LastDistance) / (TICK_SPEED / 1000.0);
    LastDistance = Distance;

    int output = pid(Speed, TargetSpeed, P, I, D);

    analogWrite(MOTOR_OUTPUT_PIN, abs(output));
    digitalWrite(MOTOR_DIRECTION_PIN, 0 > output);

    servo.write(Turning + 90 + ServoOffset);

    Serial.println(Distance);
    Serial.println(TargetSpeed);
  }

  if ((millis() - LastStatusUpdate) > STATUS_TICK_SPEED) {
    LastStatusUpdate = millis();
    post["Distance"] = Distance;
    post["Speed"] = Speed;
    char buffer[256];
    serializeJson(post, buffer);
    mqttClient.publish(mqttStatusTopic, buffer);
  }
}

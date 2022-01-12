#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PID_v1.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <config.h>
#include <math.h>

#define motorOutputPin D1
#define motorDirectionPin D3

#define TACHO_PIN D7
#define servoPin D5
#define batteryControllPin D4

const float motorRatio = (1.0 / 60.0) / 3.0;
const float wheelDiameter = 36.0; // unit in mm
const float differentialRatio = 1.0 / 1.4;

double Kp = 1, Ki = 1, Kd = 1;

bool Direction;
int8 servoOffset = 0;
double Output;
double Input;
StaticJsonDocument<512> doc;

uint32 LastMeasures[3] = {0, 0, 0};
uint64 MicrosAtLast = 0;
uint64 TimeAtLastRotation = 0;

int8 Dir;

float MotorRatio = 1.0 / 60.0;
float DiffRatio = 1.0 / 1.40;
float WheelDiameter = 31.0;
float Speed = 0;
float Distance = 0;


bool temp_dir = 1;

// PID pid(&ActualSpeed, &Output, &targetSpeed, Kp, Ki, Kd, DIRECT);
// PID myPID(&ActualSpeed, &Output, &TargetSpeed, Kp, Ki, Kd, DIRECT);
Servo servo;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void IRAM_ATTR interrupt() {

  if ((micros() - LastMeasures[0]) < 200) {
    return;
  }

  LastMeasures[2] = LastMeasures[1];
  LastMeasures[1] = LastMeasures[0];

  LastMeasures[0] = micros() - MicrosAtLast;

  int dirSum = 0;

  dirSum += LastMeasures[0] > LastMeasures[1];
  dirSum += LastMeasures[1] > LastMeasures[2];
  dirSum += LastMeasures[2] > LastMeasures[0];
  MicrosAtLast = micros();

  int tempDir;

  Serial.println(dirSum - 1);

  //if (dirSum - 1) {
  //  Dir = dirSum - 1;
  //  Speed = ((float)(micros() - TimeAtLastRotation) / 1.0);
  //  Distance += Speed;

  //  Serial.printf("%8i, %8i, %10i, %8i, %8lli, %8.1f, %f\n", LastMeasures[0],
  //                LastMeasures[1], LastMeasures[2], dirSum - 1,
  //                micros() - TimeAtLastRotation, Speed, Distance);
  //  TimeAtLastRotation = micros();
  //}
}

void callback(char *topic, byte *payload, unsigned int length) {
  long unsigned int startTime = micros();

  // Serial.println((char *)payload);
  deserializeJson(doc, payload);
  // serializeJsonPretty(doc, Serial);

  JsonVariant servoData = doc["Servo"];
  if (!servoData.isNull()) {
    servoOffset = servoData["servoOffset"];
    servoOffset = servoOffset * -1;
  }

  JsonVariant pidData = doc["PID"];
  if (!pidData.isNull()) {
    Kp = pidData["p"];
    Ki = pidData["i"];
    Kd = pidData["d"];
  }

  JsonVariant driveData = doc["drive"];
  if (!driveData.isNull()) {
    uint16 forward = doc["Forward"];
    uint16 backward = doc["Backward"];
    uint8 turning = int(doc["Turning"]) + 90;

    Input = forward - backward;
    Direction = (Input < 0 ? LOW : HIGH);

    Serial.println(Input);
    // Serial.println(turning);

    servo.write(turning + servoOffset);
    // Serial.println(micros() - startTime);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(motorDirectionPin, OUTPUT);
  pinMode(motorOutputPin, OUTPUT);
  pinMode(batteryControllPin, OUTPUT);
  pinMode(TACHO_PIN, INPUT);

  // myPID.SetMode(AUTOMATIC);

  attachInterrupt(digitalPinToInterrupt(TACHO_PIN), interrupt, RISING);

  digitalWrite(batteryControllPin, 0);

  delay(2000);

  digitalWrite(batteryControllPin, 1);

  Serial.begin(921600);

  servo.attach(D5);

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

  // ActualSpeed =
  //    (Count / 0.100) * motorRatio * differentialRatio * wheelDiameter
  //    * 3.141;
  // Count = 0;

  // if (myPID.Compute()) {
  //  Serial.println("PID updated");
  //} else {
  //  Serial.println("PID LOOP FAILED");
  //  mqttClient.publish(mqttPubTopic, "PID LOOP FAILED");
  //}
  //

  // Serial.println(Input);
  analogWrite(motorOutputPin, 128);
  digitalWrite(motorDirectionPin, temp_dir);
  temp_dir = !temp_dir;

  StaticJsonDocument<128> status;
  status["Distance"] = Distance;
  status["Speed"] = Speed;

  char statusSerialized[128];
  serializeJson(status, statusSerialized);
  mqttClient.publish(mqttStatusTopic, statusSerialized);

  delay(2000);
}

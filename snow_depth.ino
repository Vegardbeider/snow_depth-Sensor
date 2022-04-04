#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

#include "Arduino_LSM6DS3.h"
#include <Wire.h>
#include <DS3231.h>
#include <SPI.h>

#include "functions.h"
#include "arduino_secrets.h"

#define MM 5 //31 //Number of messurments to take before getting a result that is sent via MQTT
#define sensor_height 200 // (cm) Height of the sensor, to calculate snow_depth
#define trigPin 3 // D3
#define echoPin 4 // D4
#define SDA 18 // A4SDA
#define SCL 19 // A5SCL

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char username[] = USERNAME;
const char password[] = PASSWORD;

const char broker[] = "192.168.20.10";
const int  port     = 1883;
const char topic[]  = "snow_sensor/snow_depth";
const char topic2[] = "snow_sensor/temperature";
const char topic3[] = "snow_sensor/duration";
const char topic4[] = "snow_sensor/distance";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

DS3231 Clock;

void setup() {
  // Setting up serial
  /*Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/

  // Get pins for sensor ready
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Prepare UI
  pinMode(LED_BUILTIN, OUTPUT);

  // Start the I2C interface
  Wire.begin();
  
  // Initialize the RTC clock
  // Disable alarm
  Clock.turnOffAlarm(2);
  // Disable oscialltor
  Clock.enableOscillator(false, false, 0);
  // Disable 32kHz output
  Clock.enable32kHz(false);
  // Set in 24-hour mode
  Clock.setClockMode(false);

  // Prepare the next execution (defined by the day, hour, minute, second) + interval
  // Interval:
  //   0b1111 // each second
  //   0b1110 // Once per minute (when second matches)
  //   0b1100 // Once per hour (when minute and second matches)
  //   0b1000 // Once per day (when hour, minute and second matches)
  //   0b0000 // Once per month when date, hour, minute and second matches. Once per week if day of the week and A1Dy=true
  
  // Set alarm to happen every minute (change to your wanted interval)
  // Every time the seconds match 4 (every minute at the 4th second, the system wakes up)
  Clock.setA1Time(1, 2, 3, 4, 0b1100, false, false, false);
  Clock.turnOnAlarm(1);

  // Empty the I2C buffer
  while(Wire.available()) {
    Wire.read();
  }

  // Setup WiFi
  
  //Serial.print("Attempting to connect to WPA SSID: ");
  //Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    //Serial.print(".");
    delay(5000);
  }

  //Serial.println("You're connected to the network");
  //Serial.println();

  // Setup Mqtt

  // mqttClient.setUsernamePassword("username", "password");
  
  //Serial.print("Attempting to connect to the MQTT broker: ");
  //Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    //Serial.print("MQTT connection failed! Error code = ");
    //Serial.println(mqttClient.connectError());

    while (1);
  }

  //Serial.println("You're connected to the MQTT broker!");
  //Serial.println();

  if(!IMU.begin()) {
    //Serial.println("Failed to initialize IMU");
    while(1);
  }
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  mqttClient.poll();

  float measurements[MM] = { 0 };
  
  float temperature = 20;
  //float humidity = 20;

  IMU.readTemperature(temperature);

  //Serial.print("Temperature: ");
  //Serial.println(temperature);
 

  for (int i = 0; i < MM; i++) {
    float duration;
    
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH);

    measurements[i] = duration;
    //Serial.print("Duration: ");
    //Serial.println(duration);

    delay(10);
  }

  sort(measurements, MM);

  float duration, distance, snow_depth, speed;

  duration = median(measurements, MM);
  speed = 331.4 + (0.606 * temperature);// + (0.0124 * humidity);
  distance = (duration / 2) * (speed / 10000);
  snow_depth = sensor_height - distance;

  //Serial.print("Snow Depth: ");
  //Serial.println(snow_depth);
  
  mqttClient.beginMessage(topic);
  mqttClient.print(snow_depth);
  mqttClient.endMessage();

  //Serial.print("Temperature: ");
  //Serial.println(temperature);

  mqttClient.beginMessage(topic2);
  mqttClient.print(temperature);
  mqttClient.endMessage();

  //Serial.print("Duration: ");
  //Serial.println(duration);

  mqttClient.beginMessage(topic3);
  mqttClient.print(duration);
  mqttClient.endMessage();

  //Serial.print("Distance: ");
  //Serial.println(distance);

  mqttClient.beginMessage(topic4);
  mqttClient.print(distance);
  mqttClient.endMessage();

  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  // Reset alarm is the last instruction of the program
  // When we reset the alarm, the power is shutdown for the system
  Clock.checkIfAlarm(1);

  // We never arrive here because the alarm is cleared
  // on the RTC which cause the shutdown of the circuit
  while(1);

  // delay(5000);
  
}

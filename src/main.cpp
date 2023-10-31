#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h> 
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <secret.h>

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define ONE_WIRE_BUS 2
#define DHT_PIN 4
#define DHT_TYPE DHT22

const int PH_SENSOR_PIN = 36;
const int UV_SENSOR_PIN = 6;
const int TDS_SENSOR_PIN = 8;

WiFiClientSecure wifi_client = WiFiClientSecure();
MQTTClient mqtt_client = MQTTClient(256);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);

void connectToAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("done!\n");

  wifi_client.setCACert(AWS_CERT_CA);
  wifi_client.setCertificate(AWS_CERT_CRT);
  wifi_client.setPrivateKey(AWS_CERT_PRIVATE);

  mqtt_client.begin(AWS_IOT_ENDPOINT, 8883, wifi_client);
  mqtt_client.onMessage(incomingMessageHandler);

  Serial.print("Connecting to AWS IOT");

  //Wait for connection to AWS IoT
  while (!mqtt_client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  Serial.print("done!\n");

  if(!mqtt_client.connected()){ 
    Serial.println("AWS IoT timeout");
    return;
  }

  mqtt_client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage() {
  StaticJsonDocument<200> REQUEST_BODY;

  // Sensor de temperatura da solução PIN 2
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  REQUEST_BODY["Solution °C"] = temperatureC;

  // Sensor de temperatura do ambiente PIN 4
  float temperature = dht.readTemperature();
  REQUEST_BODY["Environment °C"] = temperature;

  // Sensor de temperatura do ambiente PIN 4
  float humidity = dht.readHumidity();
  REQUEST_BODY["Humidity"] = humidity;

  // Sensor de pH PIN indefinido
  int rawValue = analogRead(PH_SENSOR_PIN);

  // Convert the analog value to pH using your sensor's calibration data.
  // You'll need to calibrate your sensor to get accurate pH values.
  float pHValue = map(rawValue, 0, 4095, 0, 14);
  REQUEST_BODY["pH"] = pHValue;

  // Sensor raios ultravioleta PIN indefinido
  int uvValue = analogRead(UV_SENSOR_PIN);
  float uvIntensity = uvValue / 1024.0 * 5.0;
  REQUEST_BODY["UV"] = uvIntensity;

  // Sensor de condutividade elétrica
  int tdsValue = analogRead(TDS_SENSOR_PIN);
  REQUEST_BODY["EC"] = tdsValue;

  char payload[512];
  serializeJson(REQUEST_BODY, payload);

  mqtt_client.publish(AWS_IOT_PUBLISH_TOPIC, payload);
  Serial.println("Sent message");
}

void incomingMessageHandler(String &topic, String &payload) {
  Serial.println("Message received!");
  Serial.println("Topic: " + topic);
  Serial.println("Payload: " + payload);
}

void setup() {
  Serial.begin(115200);
  sensors.begin();
  dht.begin();
  connectToAWS();
}

void loop() {
  publishMessage();
  mqtt_client.loop();
  delay(1500);
}

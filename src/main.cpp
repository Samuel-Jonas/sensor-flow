#include <OneWire.h>
#include <DallasTemperature.h>
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "private/secret.h"

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define ONE_WIRE_BUS 2
#define DHT_SENSOR_PIN 4
#define PH_SENSOR_PIN 32
#define UV_SENSOR_PIN 35
#define TDS_SENSOR_PIN 33
#define LDR_SENSOR_PIN 34

#define DHT_TYPE DHT22

WiFiClientSecure wifi_client = WiFiClientSecure();
MQTTClient mqtt_client = MQTTClient(256);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, TIME_ZONE * 3600, 60000);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHT_SENSOR_PIN, DHT_TYPE);

void incomingMessageHandler(String &topic, String &payload) {
  Serial.println("Message received!");
  Serial.println("Topic: " + topic);
  Serial.println("Payload: " + payload);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print(" done!\n");

  timeClient.begin();
  timeClient.update();
}

void connectToAWS() {
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

  Serial.print(" done!\n");

  if(!mqtt_client.connected()){ 
    Serial.println("AWS IoT timeout");
    return;
  }

  mqtt_client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Core was connected!");
}

void publishMessage() {
  StaticJsonDocument<200> REQUEST_BODY;
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm timeInfo;
  gmtime_r(&epochTime, &timeInfo);

  char formattedTime[20];
  strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeInfo);

  REQUEST_BODY["Timestamp"] = formattedTime;

  // Sensor de temperatura da solução PIN 2
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  REQUEST_BODY["Temperature H2O (°C)"] = temperatureC;

  // Sensor de temperatura do ambiente PIN 4
  float temperature = dht.readTemperature();
  REQUEST_BODY["Temperature Zone (°C)"] = temperature;

  // Sensor de temperatura do ambiente PIN 4
  float humidity = dht.readHumidity();
  REQUEST_BODY["Humidity (%)"] = humidity;

  // Sensor de pH PIN indefinido
  int rawValue = analogRead(PH_SENSOR_PIN);

  // Convert the analog value to pH using your sensor's calibration data.
  // You'll need to calibrate your sensor to get accurate pH values.
  float pHValue = map(rawValue, 0, 4095, 0, 14);
  REQUEST_BODY["pH Level"] = pHValue;

  // Sensor raios ultravioleta PIN indefinido
  int uvValue = analogRead(UV_SENSOR_PIN);
  float uvIntensity = uvValue / 1024.0 * 5.0;
  REQUEST_BODY["Ultraviolet Light"] = uvIntensity;

  // Sensor de condutividade elétrica
  int tdsValue = analogRead(TDS_SENSOR_PIN);
  REQUEST_BODY["Electrical Conductivity"] = tdsValue;

  // Sensor LDR verificar se está de dia ou a noite
  int ldrValue = analogRead(LDR_SENSOR_PIN);
  REQUEST_BODY["Bright Level"] = ldrValue;

  char payload[512];
  serializeJson(REQUEST_BODY, payload);

  mqtt_client.publish(AWS_IOT_PUBLISH_TOPIC, payload);
  Serial.println("Message sent sucessfully");
}

void logInfo() {
  StaticJsonDocument<200> REQUEST_BODY;
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm timeInfo;
  gmtime_r(&epochTime, &timeInfo);

  char formattedTime[20];
  strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeInfo);

  REQUEST_BODY["Timestamp"] = formattedTime;

  // Sensor de temperatura da solução PIN 2
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  REQUEST_BODY["Temperature H2O (°C)"] = temperatureC;

  // Sensor de temperatura do ambiente PIN 4
  float temperature = dht.readTemperature();
  REQUEST_BODY["Temperature Zone (°C)"] = temperature;

  // Sensor de temperatura do ambiente PIN 4
  float humidity = dht.readHumidity();
  REQUEST_BODY["Humidity (%)"] = humidity;

  // Sensor de pH PIN indefinido
  int rawValue = analogRead(PH_SENSOR_PIN);

  // Convert the analog value to pH using your sensor's calibration data.
  // You'll need to calibrate your sensor to get accurate pH values.
  float pHValue = map(rawValue, 0, 4095, 0, 14);
  REQUEST_BODY["pH Level"] = pHValue;

  // Sensor raios ultravioleta PIN indefinido
  int uvValue = analogRead(UV_SENSOR_PIN);
  float uvIntensity = uvValue / 1024.0 * 5.0;
  REQUEST_BODY["Ultraviolet Light"] = uvIntensity;

  // Sensor de condutividade elétrica
  int tdsValue = analogRead(TDS_SENSOR_PIN);
  REQUEST_BODY["Electrical Conductivity"] = tdsValue;

  // Sensor LDR verificar se está de dia ou a noite
  int ldrValue = analogRead(LDR_SENSOR_PIN);
  REQUEST_BODY["Bright Level"] = ldrValue;

  char payload[512];
  serializeJson(REQUEST_BODY, payload);

  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  sensors.begin();
  dht.begin();
  connectToWiFi();
  connectToAWS();
}

void loop() {
  publishMessage();
  mqtt_client.loop();
  //logInfo();
  delay(1500);
}

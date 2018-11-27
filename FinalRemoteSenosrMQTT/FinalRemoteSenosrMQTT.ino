#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BME280I2C.h>
#include <Wire.h>
#define ssid  "BlastOff"
#define password   "AA$JJ@KK"
#define mqttServer  "192.168.86.94"
const int mqttPort =1883;
#define mqttUser "theonlymqtt"
#define mqttPassword  "thereisnotpassword"
#define deviceName "Indoor0" // (Outdoor0, Outdoor1, Attic, Inside0, Inside1)
#define tempTopic "sensor/temperature/indoor0" // (outdoor0, outdoor1, attic, inside0, inside1)
#define humTopic "sensor/humidity/indoor0" // (outdoor0, outdoor1, attic, inside0, inside1)
#define presTopic "sensor/pressure/indoor0" // (outdoor0, outdoor1, attic, inside0, inside1)
#define minutesAFK 5 // 10 for outside, 5 for inside

float temperature = 0;
float humidity = 0;
float pressure = 0;


WiFiClient espClient;
PubSubClient client(espClient);
BME280I2C bme;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if(client.connect(deviceName, mqttUser, mqttPassword )) 
{
     Serial.println("connected");
    }else{
      Serial.print("failed state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  Wire.begin();
  if (!bme.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  client.loop();
  calcBME280(&Serial);
  client.publish(tempTopic, String(temperature).c_str());
  delay(50);
  client.publish(humTopic, String(humidity).c_str());
  delay(50);
  client.publish(presTopic, String(pressure).c_str());
  delay(10);

delay(minutesAFK * 1000); //Uncomment for indoor and comment out ESP.deepSleep
//ESP.deepSleep(minutesAFK * 60 * 1e6); // MAKE SURE D0 is connected to RST. PLEASE DO THIS AGAIN

}


void loop() {} // Nothing here cause runs setup, goes to sleep, repeat
 
void calcBME280(Stream* client) {
  float temp(NAN), hum(NAN), pres(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);

  Serial.print(temp * 9/5 + 32); Serial.println("F");
  Serial.print(hum); Serial.println("%");
  Serial.print(pres); Serial.println("Pa");
  
  temperature = temp * 9/5 + 32;
  humidity = hum;
  pressure = pres;
}

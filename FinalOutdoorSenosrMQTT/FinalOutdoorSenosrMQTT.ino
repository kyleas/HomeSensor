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
#define deviceName "OutdoorSensor0" // Change to OutdoorSensor1 for second
#define tempTopic "sensor/temperature/outdoor0" //Change to outdoor1 for second
#define humTopic "senosr/humidity/outdoor0" // Change to outdoor1 for second
#define minutesAFK 0.1
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
  client.publish(tempTopic, String(calcBME280(&Serial, "temp")).c_str());
  client.publish(humTopic, String(calcBME280(&Serial, "hum")).c_str());
  Serial.print(String(calcBME280(&Serial, "temp")));
  Serial.println("FFFFFF");

ESP.deepSleep(minutesAFK * 60 * 1e6); // MAKE SURE D0 is connected to RST. PLEASE DO THIS AGAIN

}


void loop() {} // Nothing here cause runs setup, goes to sleep, repeat
 
float calcBME280(Stream* client, String whichOne) {
  float temp(NAN), hum(NAN), pres(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  
  if (whichOne == "temp") { Serial.print(temp * 9/5 + 32); Serial.println("F");
  return (temp * 9/5 + 32); }
  
  else if (whichOne == "hum") { Serial.print(hum); Serial.println("%");
  return (hum); }
  else { return (hum); }
}

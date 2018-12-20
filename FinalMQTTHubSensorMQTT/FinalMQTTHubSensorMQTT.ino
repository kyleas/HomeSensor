#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <time.h>
#include <ESP8266SMTP.h>
#define ssid  "BlastOff"
#define password   "AA$JJ@KK"
#define mqttServer  "192.168.86.94"
const int mqttPort =1883;
#define mqttUser "theonlymqtt"
#define mqttPassword  "thereisnotpassword"
#define deviceName "Inside1"
WiFiClient espClient;
PubSubClient client(espClient);
BME280I2C bme;

//Values
float avgintemp;
float avgouttemp;
float avginhum;
float avgouthum;

//Time
int timezone = 16;
int dst = 0;
int sending = 0;
int minute;
int hour;
long currentMillis;
long lastMillis;

//Email
boolean hasSent = false;

// Receive with start- and end-markers combined with parsing
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use by strtok() function
char topic[numChars] = {0}; // variables to hold the parsed data
float value = 0.0; // variables to hold the parsed data
boolean newData = false;
String temporary;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if(client.connect(deviceName, mqttUser, mqttPassword )) 
{
     //Serial.println("connected");
    }else{
      Serial.print("failed state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  reconnect();
  Wire.begin();
  if (!bme.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  client.subscribe("sensor/temperature/outdoor0");
  client.subscribe("sensor/temperature/outdoor1");
  client.subscribe("sensor/temperature/attic");
  client.subscribe("sensor/temperature/indoor0");
  client.subscribe("sensor/humidity/outdoor0");
  client.subscribe("sensor/humidity/outdoor1");
  client.subscribe("sensor/humidity/attic");
  client.subscribe("sensor/humidity/indoor0");
  
}
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Messaged in topic: ");
  Serial.println(topic);
  Serial.print("Message "); 

  char message[length + 1];
  for(int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }

  String theTopic(topic);
  String theMessage(message);
  Serial.write(("<" + theTopic + "," + theMessage + ">").c_str()); 
   
   
}
long lastMsg = 0;

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 50000) {
    lastMsg = now; 
  float newTemp = calcBME280(&Serial, "temp");
  float newHum = calcBME280(&Serial, "hum");
  }
  recvWithStartEndMarkers();
    if (newData == true) { 
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() replaces the commas with \0
        parseData(); 
        calcData();
        //sendEmail("IT WORKED", String(avgouttemp));
        newData = false;
    }
  currentMillis = millis();
  if (currentMillis - lastMillis > 1000) {
    getTime();
    calcEmail();
  }
  
}

//============

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============

void parseData() {

      // split the data into its parts
    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(topic, strtokIndx); // copy it to topic

    strtokIndx = strtok(NULL, ",");
    value = atof(strtokIndx);     // convert this part to a float

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (client.connect(deviceName, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  client.subscribe("sensor/temperature/outdoor0");
  client.subscribe("sensor/temperature/outdoor1");
  client.subscribe("sensor/temperature/attic");
  client.subscribe("sensor/temperature/indoor0");
  client.subscribe("sensor/humidity/outdoor0");
  client.subscribe("sensor/humidity/outdoor1");
  client.subscribe("sensor/humidity/attic");
  client.subscribe("sensor/humidity/indoor0");
}
float calcBME280(Stream* client, String whichOne) {
  
  float temp(NAN), hum(NAN), pres(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  
  if (whichOne == "temp") { Serial.print(temp * 9/5 + 32); Serial.println("F");   
  String thisTemp(temp *9/5 +32);
  Serial.write(("<sensor/temperature/indoor1," + thisTemp + ">").c_str()); 
  return (temp * 9/5 + 32); 
  } else if (whichOne == "hum") { Serial.print(hum); Serial.println("%");
  String thisHum(hum);
    Serial.write(("<sensor/humidity/indoor1," + thisHum + ">").c_str()); 
  return (hum); }
  else { return (hum); }
}

//======================
void calcData() {
  if (String(topic) == "avgintemp") { avgintemp = value; }
  else if (String(topic) == "avgouttemp") { avgouttemp = value; } 
  else if (String(topic) == "avginhum") { avginhum = value; }
  else if (String(topic) == "avgouthum") { avgouthum = value; }
}

//======================
void getTime() {
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  hour = timeinfo->tm_hour;
  minute = timeinfo->tm_min;

  if (hasSent == true && hour == 11 && minute >= 25 && minute <= 30) { hasSent = false; }
}

//====================
void calcEmail() {
  if (hour == 10 && minute >= 0 && minute <= 5 && hasSent == false) {
    hasSent = true;
    if (avgouttemp <= 38) {
      sendEmail("Very Cold Today. Have fun :)",
        "If you are biking today, you should wear PANTS, MITTENS, WOOL SOCKS, SLEEVES ON REFLECTIVE VEST and a LONG SLEEVE SHIRT. Current temperature is " + String(avgouttemp));
    } else if (avgouttemp <= 43) {
      sendEmail("It's kinda cold today",
        "If you are biking today, you should wear MITTENS, WOOL SOCKS, SLEEVES ON REFLECTIVE VEST and a LONG SLEEVE SHIRT. Current temperature is " + String(avgouttemp));
    } else if (avgouttemp <= 47) {
      sendEmail("It's starting on a cold path",
        "If you are biking today, you should wear SLEEVES ON REFLECTIVE VEST and a LONG SLEEVE SHIRT. Current temperature is " + String(avgouttemp));      
    } else if (avgouttemp <= 52) {
      sendEmail("It's a bit brisk",
        "If you are biking today, you should wear a LONG SLEEVE SHIRT. Current temperature is " + String(avgouttemp));      
    } else {
      sendEmail("It is a nice day today",
        "If you are biking today, don't worry about temperature. It's not cold out today :). Current temperature is " + String(avgouttemp));
    }
  }
}

//===================
void sendEmail(String header, String body) {
    int lengthOf = header.length() + 1;
    char subject[lengthOf];
    header.toCharArray(subject, lengthOf);
    
    SMTP.setEmail("youreverydayalert@gmail.com")
    .setPassword("T6dQg8T715qE")
    .Subject(subject)
    .setFrom("A daily head's up")
    .setForGmail();           // simply sets port to 465 and setServer("smtp.gmail.com");  

    if(SMTP.Send("kylenotshoe@gmail.com", body)) {
    Serial.println(F("Message sent"));
  } else {
    Serial.print(F("Error sending message: "));
    Serial.println(SMTP.getError());
  } 
}


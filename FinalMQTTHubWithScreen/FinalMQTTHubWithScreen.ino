// https://forum.arduino.cc/index.php?topic=288234.0
//https://howtomechatronics.com/tutorials/arduino/arduino-tft-lcd-touch-screen-tutorial/

#include <UTFT.h>
#include <URTouch.h>

// Receive with start- and end-markers combined with parsing
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use by strtok() function
char topic[numChars] = {0}; // variables to hold the parsed data
float value = 0.0; // variables to hold the parsed data
boolean newData = false;


String topics[10] = { "sensor/temperature/outdoor0", "sensor/temperature/outdoor1", 
             "sensor/temperature/attic", "sensor/temperature/indoor0",
             "sensor/temperature/indoor1", "sensor/humidity/outdoor0",
             "sensor/humidity/outdoor1", "sensor/humidity/attic", 
             "sensor/humidity/indoor0", "sensor/humidity/indoor1" };

float values[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long lastUpdated[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int updatePins[10] = { 27, 29, 31, 33, 35, 37, 39, 41, 43, 45 };
//boolean updatedRecently[10] = {};
float avgs[4] = { 0, 0, 0, 0 };
int updateNum = -1;
int sensors = 5; // How many sensors are there?
int measuring = 2; // How many things are you measuring
unsigned int minutesAllowedAFK = 20;

//=========== For LCD Display
UTFT myGLCD(SSD1289,38,39,40,41); // CHANGE BASED ON LCD
URTouch myTouch(6, 5, 4, 3, 2); 

//============

void setup() {
    Serial.begin(9600);
    Serial1.begin(115200);
}

//============

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) { 
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() replaces the commas with \0
        parseData(); 
        showParsedData();
        calcData();
        updatedRecent();
        newData = false;
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

//============

void showParsedData() {
    Serial.print("Topic ");
    Serial.println(topic);
    Serial.print("Value ");
    Serial.println(value);
}

//===========

void calcData() {
  if (String(topic) == topics[0] ) { updateNum = 0; }
  else if (String(topic) == topics[1]) {updateNum = 1;}
  else if (String(topic) == topics[2]) {updateNum = 2;}
  else if (String(topic) == topics[3]) {updateNum = 3;}
  else if (String(topic) == topics[4]) {updateNum = 4;}
  else if (String(topic) == topics[5]) {updateNum = 5;}
  else if (String(topic) == topics[6]) {updateNum = 6;}
  else if (String(topic) == topics[7]) {updateNum = 7;}
  else if (String(topic) == topics[8]) {updateNum = 8;}
  else if (String(topic) == topics[9]) {updateNum = 9;}
  else { updateNum = -1; }

  if (updateNum > -1) {
    values[updateNum] = value;
    lastUpdated[updateNum] = millis();
    
    Serial.print("Values: ");
    for(int i = 0; i < sensors*measuring; i++) { Serial.print(values[i]); Serial.print(",");}
    Serial.println("");
    Serial.print("Last updated ");
    for(int i=0;i<sensors*measuring; i++) { Serial.print(lastUpdated[i]); Serial.print(",");}
    Serial.println("");
    
    switch (updateNum) {
      case 0: case 1: // Avg Outdoor Temperature
        avgs[0] = (values[0] + values[1]) / 2;
      case 3: case 4: // Avg Indoor Temperature
        avgs[1] = (values[3] + values[4]) / 2;
      case 5: case 6: // Avg Outdoor Humidity
        avgs[2] = (values[5] + values[6]) / 2;
      case 8: case 9: // Avg Indoor Humidity
        avgs[3] = (values[8] + values[9]) / 2; 

      for (int i=0; i<4; i++) { Serial.print(avgs[i]); Serial.print(","); }
    }
  } else { Serial.println("ERROR: UpdateNum == -1"); }
}

//=============

void updatedRecent() {
  for (int i=0; i<10; i++) {
    if (millis() - lastUpdated[i] >= minutesAllowedAFK * 1000) {
      //updatedRecently[i] = false;
      digitalWrite(updatePins[i], HIGH);
    } else {
      //updatedRecently[i] = true;
      digitalWrite(updatePins[i], LOW);
    }
  }
}

//================

void updateESP8266() {
  
}


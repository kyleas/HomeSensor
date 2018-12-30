// https://forum.arduino.cc/index.php?topic=288234.0
//https://howtomechatronics.com/tutorials/arduino/arduino-tft-lcd-touch-screen-tutorial/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
MCUFRIEND_kbv tft;
#define MINPRESSURE 200
#define MAXPRESSURE 1000

// Receive with start- and end-markers combined with parsing
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use by strtok() function
char topic[numChars] = {0}; // variables to hold the parsed data
float value = 0.0; // variables to hold the parsed data
boolean newData = false;
String temporary;

//For storing and getting data from different sensors
String topics[10] = { "sensor/temperature/outdoor0", "sensor/temperature/outdoor1", 
             "sensor/temperature/attic", "sensor/temperature/indoor0",
             "sensor/temperature/indoor1", "sensor/humidity/outdoor0",
             "sensor/humidity/outdoor1", "sensor/humidity/attic", 
             "sensor/humidity/indoor0", "sensor/humidity/indoor1" };
float values[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long lastUpdated[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int updatePins[10] = { 27, 29, 31, 33, 35, 37, 39, 41, 43, 45 };
float avgs[4] = { 0, 0, 0, 0 };
int updateNum = -1;
int sensors = 5; // How many sensors are there?
int measuring = 2; // How many things are you measuring

//For calulations
unsigned int minutesAllowedAFK = 15;
unsigned int lastSaftey = 0;

//=========== For LCD Display
//const int XP = 6, XM = A2, YP = A1, YM = 7; //ID=0x9341
//const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;
const int XP=7,XM=A1,YP=A2,YM=6; //320x480 ID=0x6814
const int TS_LEFT=153,TS_RT=877,TS_TOP=119,TS_BOT=921;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
int pixel_x, pixel_y;     //Touch_getXY() updates global vars

//Custom screen stuff
int screen = 0;
int prevScreen = 0;
long lastUpdate = 0;
int lengthOf = 0;
Adafruit_GFX_Button intemp, inhum, outtemp, outhum, temp_btn, hum_btn, pres_btn, update_btn, home_btn;
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

//===========
bool Touch_getXY(void) {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed) {
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
  }
  return pressed;
}

//============
void setup(void) {
    Serial.begin(115200);
    Serial1.begin(115200);
    uint16_t ID = tft.readID();
    Serial.print("TFT ID = 0x");
    Serial.println(ID, HEX);
    Serial.println("Calibrate for your touch panel");
    if (ID == 0xD3D3) ID = 0x9486;
    tft.begin(ID);
    tft.setRotation(0);
    tft.fillScreen(BLACK);
    updateDisplay();
}

//============
Adafruit_GFX_Button *buttons[] = {&intemp, &inhum, &outtemp, &outhum, &temp_btn, &hum_btn, &pres_btn, &update_btn, &home_btn, NULL};
bool update_button(Adafruit_GFX_Button *b, bool down) {
  b->press(down && b->contains(pixel_x, pixel_y));
  if (b->justReleased())
//    b->drawButton(false);
  if (b->justPressed())
//    b->drawButton(true);
  return down;
}

//============
bool update_button_list(Adafruit_GFX_Button **pb) {
  bool down = Touch_getXY();
  for (int i = 0 ; pb[i] != NULL; i++) {
    update_button(pb[i], down);
  }
  return down;
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
    update_button_list(buttons);  //use helper function
    updateScreen();
    button();
}

//============

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial1.available() > 0 && newData == false) {
        rc = Serial1.read();

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
    if (updateNum == 4) {
      if (values[4] - 2 >= values[3]) {
        values[4] = values[3];
      }
    }
    
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

  Serial1.write(("<avgouttemp," + String(avgs[0]) + ">").c_str());
  Serial.println(("<avgouttemp," + String(avgs[0]) + ">").c_str());
}

//=============

void updatedRecent() {
  for (int i=0; i<10; i++) {
    if (millis() - lastUpdated[i] >= minutesAllowedAFK * 60000) {
      //updatedRecently[i] = false;
      digitalWrite(updatePins[i], HIGH);
      if (i == 0) { values[0] = values[1]; }
      else if (i == 1) { values[1] = values[0]; }
      else if (i == 3) { values[3] = values[4]; }
      else if (i == 4) { values[4] = values[3]; }
      else if (i == 5) { values[5] = values[6]; }
      else if (i == 6) { values[6] = values[5]; }
      else if (i == 8) { values[8] = values[9]; }
      else if (i == 9) { values[9] = values[8]; }
    } else {
      //updatedRecently[i] = true;
      digitalWrite(updatePins[i], LOW);
    }
  }
}

//===========
void safetyCheck() {
 if (millis() - lastSaftey >= 5000) {
    if ((values[0] != 0) && (values[1] !=0)) {
      if ((values[0] - values [1] >= 5) || (values[0] - values[1] <= -5)) {
        digitalWrite(updatePins[5], HIGH);
        digitalWrite(updatePins[6], HIGH);
        Serial.println("Outdoor sensors are wack!");
      } else { digitalWrite(updatePins[5], LOW); digitalWrite(updatePins[6], LOW); }
    }
    if ((values[3] != 0) && (values[4] !=0)) {
      if ((values[3] - values [4] >= 5) || (values[3] - values[4] <= -5)) {
        digitalWrite(updatePins[8], HIGH);
        digitalWrite(updatePins[9], HIGH);
        Serial.println("Indoor sensors are wack!");
      } else { digitalWrite(updatePins[8], LOW); digitalWrite(updatePins[9], LOW); }
    }
 }

}

//=========== UPDATE DISPLAY
void updateDisplay() {
  String avgs1 = (String(avgs[1]) + "F");
  String avgs2 = (String(avgs[2]) + "%");
  String avgs3 = (String(avgs[3]) + "%");
  String avgs0 = (String(avgs[0]) + "F");
  
  int avgs0_ln = avgs0.length() + 1;
  int avgs1_ln = avgs1.length() + 1;
  int avgs2_ln = avgs2.length() + 1;
  int avgs3_ln = avgs3.length() + 1;
  
  char avgs_0[avgs0_ln];
  char avgs_1[avgs1_ln];
  char avgs_2[avgs2_ln];
  char avgs_3[avgs3_ln];

  avgs0.toCharArray(avgs_0, avgs1_ln);
  avgs1.toCharArray(avgs_1, avgs1_ln);
  avgs2.toCharArray(avgs_2, avgs1_ln);
  avgs3.toCharArray(avgs_3, avgs1_ln);

  Serial.println(avgs1);
  
  if (screen == 0) { // RESET SCREEN 
    tft.setRotation(0 );
    tft.fillScreen(BLACK);

    temp_btn.initButton(&tft, 40, 440, 60, 40, BLACK, RED, BLACK, "TEMP", 2);
    hum_btn.initButton(&tft, 120, 440, 60, 40, BLACK, BLUE, BLACK, "HUM", 2);
    pres_btn.initButton(&tft, 200, 440, 60, 40, BLACK, YELLOW, BLACK, "PRES", 2);
    update_btn.initButton(&tft, 280, 440, 60, 40, BLACK, GREEN, BLACK, "UPD", 2);

    tft.setTextSize(3);
    tft.setCursor(110,10);
    tft.setTextColor(CYAN);
    tft.println("INSIDE");
    tft.setTextColor(RED);
    tft.setCursor(110, 200);
    tft.println("OUTSIDE");
  } 
  if (screen == 0 || screen == 1) { // JUST UPDATE HOME VALUES
 
    intemp.initButton(&tft, 160, 80, 300, 70, BLACK, CYAN, BLACK, avgs_1, 5);
    inhum.initButton(&tft, 160, 160, 300, 70, BLACK, CYAN, BLACK, avgs_3, 5);
    outtemp.initButton(&tft, 160, 280, 300, 70, BLACK, RED, BLACK, avgs_0, 5);
    outhum.initButton(&tft, 160, 360, 300, 70, BLACK, RED, BLACK, avgs_2, 5);

  
   intemp.drawButton(false);
   inhum.drawButton(true);
   outtemp.drawButton(false);
   temp_btn.drawButton(false);
   hum_btn.drawButton(false);
   pres_btn.drawButton(false);
   update_btn.drawButton(false);
   outhum.drawButton(true);
  }
  if (screen == 2) { // TEMPERATURE SCREEN
    tft.fillScreen(BLACK);
    tft.setTextSize(3);

    tft.setTextColor(RED);
    tft.setCursor(100, 50);
    tft.println("TEMPERARURE");
    
    tft.setTextColor(WHITE);
    tft.setCursor(200, 150);
    tft.println(values[0]);
    tft.setCursor(200, 200);
    tft.println(values[1]);
    tft.setCursor(200, 250);
    tft.println(values[2]);
    tft.setCursor(200, 300);
    tft.println(values[3]);
    tft.setCursor(200, 350);
    tft.println(values[4]);

    tft.setCursor(50, 150);
    tft.println("OUT0");
    tft.setCursor(50, 200);
    tft.println("OUT1");
    tft.setCursor(50, 250);
    tft.println("ATTIC");
    tft.setCursor(50, 300);
    tft.println("IN0");
    tft.setCursor(50, 350);
    tft.println("IN1");

    temp_btn.initButton(&tft, 40, 440, 60, 40, BLACK, RED, BLACK, "TEMP", 2);
    hum_btn.initButton(&tft, 120, 440, 60, 40, BLACK, BLUE, BLACK, "HUM", 2);
    pres_btn.initButton(&tft, 200, 440, 60, 40, BLACK, YELLOW, BLACK, "PRES", 2);
    update_btn.initButton(&tft, 280, 440, 60, 40, BLACK, GREEN, BLACK, "UPD", 2);
    temp_btn.drawButton(false);
    hum_btn.drawButton(false);
    pres_btn.drawButton(false);
    update_btn.drawButton(false);

    home_btn.initButton(&tft, 50, 50, 60, 60, BLACK, CYAN, BLACK, "HOME", 2);
    home_btn.drawButton(false);
  }
  if (screen == 3) { // HUMIDITY SCREEN
    tft.fillScreen(BLACK);
    tft.setTextSize(3);

    tft.setTextColor(BLUE);
    tft.setCursor(100, 50);
    tft.println("HUMIDITY");
    
    tft.setTextColor(WHITE);
    tft.setCursor(200, 150);
    tft.println(values[5]);
    tft.setCursor(200, 200);
    tft.println(values[6]);
    tft.setCursor(200, 250);
    tft.println(values[7]);
    tft.setCursor(200, 300);
    tft.println(values[8]);
    tft.setCursor(200, 350);
    tft.println(values[9]);

    tft.setCursor(50, 150);
    tft.println("OUT0");
    tft.setCursor(50, 200);
    tft.println("OUT1");
    tft.setCursor(50, 250);
    tft.println("ATTIC");
    tft.setCursor(50, 300);
    tft.println("IN0");
    tft.setCursor(50, 350);
    tft.println("IN1");

    temp_btn.initButton(&tft, 40, 440, 60, 40, BLACK, RED, BLACK, "TEMP", 2);
    hum_btn.initButton(&tft, 120, 440, 60, 40, BLACK, BLUE, BLACK, "HUM", 2);
    pres_btn.initButton(&tft, 200, 440, 60, 40, BLACK, YELLOW, BLACK, "PRES", 2);
    update_btn.initButton(&tft, 280, 440, 60, 40, BLACK, GREEN, BLACK, "UPD", 2);
    temp_btn.drawButton(false);
    hum_btn.drawButton(false);
    pres_btn.drawButton(false);
    update_btn.drawButton(false);

    home_btn.initButton(&tft, 50, 50, 60, 60, BLACK, CYAN, BLACK, "HOME", 2);
    home_btn.drawButton(false);
  }
  if (screen == 4) { // PRES
    tft.fillScreen(BLACK);
    tft.setTextSize(3);    
    tft.setTextColor(GREEN);
    tft.setCursor(100, 50);
    tft.println("PRESSURE"); 

    tft.setTextColor(WHITE);
    tft.setCursor(100, 100);
    tft.println("IN PROGRESS");

    temp_btn.initButton(&tft, 40, 440, 60, 40, BLACK, RED, BLACK, "TEMP", 2);
    hum_btn.initButton(&tft, 120, 440, 60, 40, BLACK, BLUE, BLACK, "HUM", 2);
    pres_btn.initButton(&tft, 200, 440, 60, 40, BLACK, YELLOW, BLACK, "PRES", 2);
    update_btn.initButton(&tft, 280, 440, 60, 40, BLACK, GREEN, BLACK, "UPD", 2);
    temp_btn.drawButton(false);
    hum_btn.drawButton(false);
    pres_btn.drawButton(false);
    update_btn.drawButton(false);

    home_btn.initButton(&tft, 50, 50, 60, 60, BLACK, CYAN, BLACK, "HOME", 2);
    home_btn.drawButton(false);   
  }
  if (screen == 5) { // LAST UPDATED
    tft.fillScreen(BLACK);
    tft.setTextSize(3);

    tft.setTextColor(GREEN);
    tft.setCursor(100, 50);
    tft.println("LAST UPDATED");
    
    tft.setTextColor(WHITE);
    tft.setCursor(100, 100);
    tft.println((millis() - lastUpdated[0]) / 60000);
    tft.setCursor(100, 150);
    tft.println((millis() - lastUpdated[1]) / 60000);
    tft.setCursor(100, 200);
    tft.println((millis() - lastUpdated[2]) / 60000);
    tft.setCursor(100, 250);
    tft.println((millis() - lastUpdated[3]) / 60000);
    tft.setCursor(100, 300);
    tft.println((millis() - lastUpdated[4]) / 60000);

    tft.setTextColor(WHITE);
    tft.setCursor(210, 100);
    tft.println((millis() - lastUpdated[5]) / 60000);
    tft.setCursor(210, 150);
    tft.println((millis() - lastUpdated[6]) / 60000);
    tft.setCursor(210, 200);
    tft.println((millis() - lastUpdated[7]) / 60000);
    tft.setCursor(210, 250);
    tft.println((millis() - lastUpdated[8]) / 60000);
    tft.setCursor(210, 300);
    tft.println((millis() - lastUpdated[9]) / 60000);

    tft.setCursor(0, 100);
    tft.println("OUT0");
    tft.setCursor(0, 150);
    tft.println("OUT1");
    tft.setCursor(0, 200);
    tft.println("ATTIC");
    tft.setCursor(0, 250);
    tft.println("IN0");
    tft.setCursor(0, 300);
    tft.println("IN1");
    tft.setCursor(100, 350);
    tft.println("TEMP");
    tft.setCursor(210, 350);
    tft.println("HUM");

    temp_btn.initButton(&tft, 40, 440, 60, 40, BLACK, RED, BLACK, "TEMP", 2);
    hum_btn.initButton(&tft, 120, 440, 60, 40, BLACK, BLUE, BLACK, "HUM", 2);
    pres_btn.initButton(&tft, 200, 440, 60, 40, BLACK, YELLOW, BLACK, "PRES", 2);
    update_btn.initButton(&tft, 280, 440, 60, 40, BLACK, GREEN, BLACK, "UPD", 2);
    temp_btn.drawButton(false);
    hum_btn.drawButton(false);
    pres_btn.drawButton(false);
    update_btn.drawButton(false);

    home_btn.initButton(&tft, 50, 50, 60, 60, BLACK, CYAN, BLACK, "HOME", 2);
    home_btn.drawButton(false);
  } 
  lastUpdate = millis(); 
}

//=============
void button() {
  if (screen == 1 || screen == 0) {
    if (temp_btn.justPressed() || intemp.justPressed() || outtemp.justPressed()) { screen = 2; }
    else if (hum_btn.justPressed() || inhum.justPressed() || outhum.justPressed()) { screen = 3; }
    else if (pres_btn.justPressed()) { screen = 4; }
    else if (update_btn.justPressed()) { screen = 5; } 
  } else {
    if (home_btn.justPressed()) { screen = 0; }
    else if (hum_btn.justPressed()) { screen = 3; }
    else if (temp_btn.justPressed()) { screen = 2; }
    else if (pres_btn.justPressed()) { screen = 4; }
    else if (update_btn.justPressed()) { screen = 5; }
  }
  if (screen != prevScreen) { 
    prevScreen = screen;
    updateDisplay();
    Serial.println("SCREEN IS " + String(screen));
  }
}

//=============
void updateScreen() {
  if (millis() - lastUpdate >= 60000) {
    if (screen != 1 && screen != 0) {
      screen = 0;
    } else if (screen == 1) {
    updateDisplay();
  } else if (screen == 0) { screen = 1; }
  Serial.println("UPDATING!!!!!!");
  lastUpdate = millis();
  }
}


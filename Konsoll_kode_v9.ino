// Libraries
#include <LiquidCrystal.h>  // Importing library for LCD display
#include <TimeLib.h>        // Importing library for time and converting unix to human time
#include <GyverEncoder.h>   // Importing library for encoder
#include <WiFi.h>           // Importing library for WiFi
#include <HTTPClient.h>     // Importing library for HTTP
#include <ArduinoJson.h>    // Importing library for Json

String TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0";  // CoT token
String bookingKEY = "15741";  // CoT signal key for booking
String roomKEY = "7284";  // CoT signal key for room settings
String bellKEY = "1613";  // CoT signal key for the bell
String dataKEY = "10893"; // CoT signal key for data
String guestKEY = "11623";

String bellRegister;
String guestRegister;

const char* ntpServer = "pool.ntp.org"; // NTP server for current unix timestamp

const char* ssid = "FD-96"; // SSID for WiFi hotpost
const char* password = "77087708";  // Password for WiFi hotpost

// Variables
int outdoor;  // Temparture outside
int indoor;  // Temperature inside

int httpResponseCodeWrite;  // Stores response code from HTTP write function
int httpResponseCodeRead; // Stores response code from HTTP reed function

int Li;   // var for scrolling text
int Lii;  // var for scrolling text 

bool lcdWrt; // Defines boolean for screen updates

unsigned long resetMillis;  // Stores millis value from last action

unsigned long currentUnix;  // Stores current time in unix
unsigned long closestUnix;  // Stores closest 5 min timestamp 
unsigned long bookingUnix;  // Stores time in unix, that user defined in "Booking time" screen
unsigned long guestUnix;    // Stores time in unix, that user defined in "Guest time" screen

unsigned long previousMillis = millis();  // Defines and stores current millis
unsigned long currentMillis = millis();   // Defines and stores current millis

int tempMode;     // Stores current mode for the thermostat
int fanMode;      // Stores current mode for the fan
int windowMode;   // Stores current mode for the window
int lightMode;    // Stores current mode for the light

int bellValue;  // Stores current balue for the bell
String bellValueBefore; // Stores all values before "bellValue"
String bellValueAfter;  // Stores all values after "bellValue"

int guestValue;  // Stores current balue for the guests
String guestValueBefore; // Stores all values before "guestValue"
String guestValueAfter;  // Stores all values after "guestValue"

int roomStatus; // Stores current status for room status

int bookingMinutes = 5;  // Stores amount of booking time in minutes
int bookingPeople = 1;   // Stores number of people who will be book

String menu = "room";         // Stores which menu will be opened in menu screen
String booking = "Kitchen";   // Stores which room will be opened in booking screen
String settings = "temp";     // Stores which setting will be opened in settings screen

byte celsius[] = { B01000, B10100, B01000, B00011, B00100, B00100, B00100, B00011 };  // Defines celsius symbol for display
byte arrows[] = { B00100, B01110, B11111, B00000, B00000, B11111, B01110, B00100 };   // Defines up/down arrows symbol for display
byte wifi[] = { B00000, B00001, B00001, B00101, B00101, B10101, B10101, B10101};      // Defines WiFi sumbol for display
byte wifiError[] = {B00000, B10001, B01010, B00100, B00100, B01010, B10001, B00000};  // Defines WiFi error sumbol for display

int freq = 2000;  // Defines frequence for buzzer
int channel = 3;  // Defines channel for buzzer
int res = 12;     // Defines resolution for buzzer

// Pinouts
const int buttonLeft = 16;    // Pin for left button (ESP32)
const int buttonMiddle = 4;   // Pin for middle button (ESP32)
const int buttonRight = 17;   // Pin for right button (ESP32)

const int LEDGreen = 25;     // Pin for green LED (ESP32)
const int LEDWhite = 26;     // Pin for white LED (ESP32)
const int LEDRed = 27;       // Pin for red LED (ESP32)

const int buzzer = 33;  // Pin for the buzzer (ESP32)

const int CLK = 35; // Encoder CLK pin
const int DT = 34;  // Encoder DT pin
const int SW = 32;  // Encoder SW (pushbutton)     

Encoder enc1(CLK, DT, SW);  // Encoder initialization

const int rs = 22, en = 21, d4 = 5, d5 = 18, d6 = 23, d7 = 19; // Defines pins for display (ESP32)
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  // Initialization of display

// Functions
void timeout() {  // Function for checking for timeouts
  unsigned long currentMillis = millis(); // Stores current millis

  enc1.tick(); // Call encoder tick function

  if (enc1.isTurn()) {  // If encoder is turning
    resetMillisFunc();  // Calls "resetMillisFunc" function
  }

  if (currentMillis - resetMillis > 60000) {  // If time since last action is more than 30 seconds
    mainScreen(); // Calls "loop" function
  }
}

void resetMillisFunc() {  // Function that resets "resetMillis" variable with current millis
  resetMillis = millis(); // Stores current millis into "resetMillis"
}

void debounce() { // Function for debouncing buttons
  while (digitalRead(buttonLeft) == LOW || digitalRead(buttonRight) == LOW || digitalRead(buttonMiddle) == LOW) { // While any button is pressed, while loop will hold the program
  }
}

void getPinMode() { // Initialization of buttons
  pinMode(buttonLeft, INPUT_PULLUP);    // Sets left button to input pullup mode
  pinMode(buttonMiddle, INPUT_PULLUP);  // Sets middle button to input pullup mode
  pinMode(buttonRight, INPUT_PULLUP);   // Sets right button to input pullup mode
  
  pinMode(LEDGreen, OUTPUT);    // Sets green LED to output mode
  pinMode(LEDWhite, OUTPUT);    // Sets white LED to output mode
  pinMode(LEDRed, OUTPUT);      // Sets red LED to output mode

  pinMode(buzzer, OUTPUT);        // Sets buzzer to output mode
  ledcSetup(channel, freq, res);  // Defines configuration for buzzer
  ledcAttachPin(buzzer, channel);
}

void getWiFi() {  // Sets up WiFi
  WiFi.begin(ssid, password); // Begins WiFi
  Serial.println("Connecting"); // Prints "Connecting"
  while(WiFi.status() != WL_CONNECTED) {  // While WiFi isnt connected
    lcd.clear();  // Clears lcd screen

    delay(100); // Delay 100 ms

    Serial.print(".");  // Prints "."

    lcd.setCursor(1,0); // Prints "Connecting WiFi" on LCD
    lcd.print("Connecting WiFi");

    delay(500);
  }
  if(WiFi.status() == WL_CONNECTED){  // If WiFi connected
    Serial.println("Success");  // Prints "Success"
  }
}

void getCharacter() { // Initialization of custom symbols for the display
  lcd.createChar(0, celsius); // Sets "celsius" to index 0
  lcd.createChar(1, arrows);  // Sets "arrows" to index 1
  lcd.createChar(2, wifi);  // Sets "wifi" to index 2
  lcd.createChar(3, wifiError);  // Sets "wifiError" to index 3
}

unsigned long getTime() { // Gets current unix timestamp from NTP server
  time_t now; // Gets time from internett
  struct tm timeinfo; // Checks timeinfo
  if (!getLocalTime(&timeinfo)) { // If data isnt right
    return(0);  // Returns 0
  }
  time(&now); 
  return now; // Else returns "now"
}

void getCurrentTime() { // Function for gathering unix time, writing it into "currentUnix" and updates internal clock
  currentUnix = getTime();  // Stores value from "getTime()" function into "currentUnix"

  currentUnix = currentUnix + 7200; // Adds 2 hour as timezone

  setTime(currentUnix); // Sets internal clock to given value
}

void getClosestTimestamp() {  // Finds closest 5 min timestamp 
  int corrector;  // Defines corrector variable for futher calcultating
  int minutes = minute(); // Gathering minutes from converted time from unix 
  int seconds = second(); // Gathering seconds from converted time from unix 
  int SS_t = (minutes * 60) + seconds;  // Finds sum of current minutes and seconds in seconds



  if ((SS_t >= 0) && (SS_t <= 299)) { // If sentence for time between 00.00 and 4.59 minutes
    corrector = 300;                  // Storing corrector variable for futher calculating
  }
  else if ((SS_t >= 300) && (SS_t <= 599)) {  // If sentence for time between 05.00 and 9.59 minutes
    corrector = 600;                          // Storing corrector variable for futher calculating
  }
  else if ((SS_t >= 600) && (SS_t <= 899)) {          
    corrector = 900;
  }
  else if ((SS_t >= 900) && (SS_t <= 1199)) {           
    corrector = 1200;
  }
  else if ((SS_t >= 1200) && (SS_t <= 1499)) {      
    corrector = 1500;                          
  }
  else if ((SS_t >= 1500) && (SS_t <= 1799)) {     
    corrector = 1800;
  }
  else if ((SS_t >= 1800) && (SS_t <= 2099)) {     
    corrector = 2100;
  }
  else if ((SS_t >= 2100) && (SS_t <= 2399)) {    
    corrector = 2400;
  }
  else if ((SS_t >= 2400) && (SS_t <= 2699)) {     
    corrector = 2700;                      
  }
  else if ((SS_t >= 2700) && (SS_t <= 2999)) {      
    corrector = 3000;
  }
  else if ((SS_t >= 3000) && (SS_t <= 3299)) { 
    corrector = 3300;
  }
  else if ((SS_t >= 3300) && (SS_t <= 3599)) { 
    corrector = 3600;
  }

  unsigned long seconds_t = corrector - SS_t; // Finding amount of seconds from now to the closest timestamp with 00/15/30/45 minutes
  closestUnix = now() + seconds_t;            // Finding unix timestamp for closest timestamp with 00/15/30/45 minutes
  
}

void CoTWrite(String KEY, String TOKEN, String value) { // Sends a given value to CoT with given key and token
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/WriteValue?Key="+String(KEY)+"&Value="+String(value)+"&Token="+String(TOKEN); // Gets together key, value and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeWrite = http.GET(); // Gets http response and stores it into "httpResponseCodeWrite"

    http.end(); // Ends http
  }
}

String CoTRead(String KEY, String TOKEN) {  // Reads values from CoT and returns the value from given key and token
  const size_t bufferSize = JSON_OBJECT_SIZE(13); // Defines buffer size for json object
  DynamicJsonBuffer jsonBuffer(bufferSize); // Sets up dynamic json buffer from previously defined buffer

  const char* httpResponse; // Defines char for data from CoT
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  // Gets together key and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeRead = http.GET();  // Gets http response and stores it into "httpResponseCodeRead"

    String payload = http.getString();  // Stores payload from http as string

    JsonObject& root = jsonBuffer.parseObject(payload); // Using Json to decode payload
    httpResponse = root["Value"]; // Stores "Value" from "payload" into "httpResponse"

    String httpResponse = httpResponse; // Defines "httpResponse" as String

    http.end(); // Ends http
  }

  return httpResponse;  // Returns "httpResponse"
} 

void roomCoTWrite(String KEY, String TOKEN) { // Sends a given value to CoT with given key and token
  lcd.setCursor(0,1); // Prints "Wait" on LCD
  lcd.print("      Wait      ");
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    char temp[16];  // Creates  char "temp" variable
    char light[16]; // Creates  char "light" variable

    sprintf(temp,"%02d", tempMode);   // Defines number of digits
    sprintf(light,"%03d", lightMode); // Defines number of digits

    String value = "1"+String(temp)+String(fanMode)+String(windowMode)+String(light)+String(roomStatus); // Gets together "value" string from variables

    String serverPath = "https://circusofthings.com/WriteValue?Key="+String(KEY)+"&Value="+String(value)+"&Token="+String(TOKEN); // Gets together key, value and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeWrite = http.GET(); // Gets http response and stores it into "httpResponseCodeWrite"

    http.end(); // Ends http

    if (httpResponseCodeWrite == 200) { // If "httpResponseCodeWrite" is 200
      lcd.setCursor(0,1); // Prints "Set" on LCD
      lcd.print("       Set      ");      

      lcdWrt = true;  // Sets lcdWrt to true

      delay(2000);  // Delay 2000 ms
    }
  }

  else {  // Else 
    lcd.setCursor(0,1); // Prints "WiFi Error"
    lcd.print("   WiFi Error   ");

    lcdWrt = true;  // Sets lcdWrt to true

    delay(2000);  // Delay 2000 ms
  }
}

void roomCoTRead(String KEY, String TOKEN) {  // Reads values from CoT (room settings) and stores values into given variables
  const size_t bufferSize = JSON_OBJECT_SIZE(13); // Defines buffer size for json object
  DynamicJsonBuffer jsonBuffer(bufferSize); // Sets up dynamic json buffer from previously defined buffer
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  // Gets together key and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeRead = http.GET();  // Gets http response and stores it into "httpResponseCodeRead"

    String payload = http.getString();  // Stores payload from http as string

    JsonObject& root = jsonBuffer.parseObject(payload); // Using Json to decode payload
    double httpResponse = root["Value"]; // Stores "Value" from "payload" into "httpResponse"

    String roomRegister = String(httpResponse); // Defines roomRegister as string of "httpResponse"

    tempMode = (roomRegister.substring(1, 3).toInt());    // Stores current mode for the thermostat
    fanMode = (roomRegister.substring(3, 4).toInt());     // Stores current mode for the fan
    windowMode = (roomRegister.substring(4, 5).toInt());  // Stores current mode for the window
    lightMode = (roomRegister.substring(5, 8).toInt());   // Stores current mode for the light
    roomStatus = (roomRegister.substring(8, 9).toInt());   // Stores current 

    http.end(); // Ends http
  }
}

void bellCoTRead(String KEY, String TOKEN) {  // Reads values from CoT (bell) and stores values into given variables
  const size_t bufferSize = JSON_OBJECT_SIZE(13); // Defines buffer size for json object
  DynamicJsonBuffer jsonBuffer(bufferSize); // Sets up dynamic json buffer from previously defined buffer
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  // Gets together key and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeRead = http.GET();  // Gets http response and stores it into "httpResponseCodeRead"

    String payload = http.getString();  // Stores payload from http as string

    JsonObject& root = jsonBuffer.parseObject(payload); // Using Json to decode payload
    double httpResponse = root["Value"]; // Stores "Value" from "payload" into "httpResponse"

    bellRegister = String(httpResponse); // Defines bellRegister as string of "httpResponse"

    bellValueBefore = (bellRegister.substring(0, 1));    // Stores all values before "bellValue"
    bellValue = (bellRegister.substring(1, 2).toInt());    // Stores current mode for the bell
    bellValueAfter = (bellRegister.substring(2, 7));    // Stores all values after "bellValue"

    http.end(); // Ends http
  }
}

void guestCoTRead(String KEY, String TOKEN) {  // Reads values from CoT (guest) and stores values into given variables
  const size_t bufferSize = JSON_OBJECT_SIZE(13); // Defines buffer size for json object
  DynamicJsonBuffer jsonBuffer(bufferSize); // Sets up dynamic json buffer from previously defined buffer
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  // Gets together key and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeRead = http.GET();  // Gets http response and stores it into "httpResponseCodeRead"

    String payload = http.getString();  // Stores payload from http as string

    JsonObject& root = jsonBuffer.parseObject(payload); // Using Json to decode payload
    double httpResponse = root["Value"]; // Stores "Value" from "payload" into "httpResponse"

    guestRegister = String(httpResponse); // Defines bellRegister as string of "httpResponse"

    guestValueBefore = (guestRegister.substring(0, 1));    // Stores all values before "bellValue"
    guestValue = (guestRegister.substring(1, 2).toInt());    // Stores current mode for the bell
    guestValueAfter = (guestRegister.substring(2, 7));    // Stores all values after "bellValue"

    http.end(); // Ends http
  }
}

void dataCoTRead(String KEY, String TOKEN) {  // Reads values from CoT (room data) and stores values into given variables
  const size_t bufferSize = JSON_OBJECT_SIZE(13); // Defines buffer size for json object
  DynamicJsonBuffer jsonBuffer(bufferSize); // Sets up dynamic json buffer from previously defined buffer
  
  if(WiFi.status() == WL_CONNECTED){  // If WifI is established
    HTTPClient http;  // Sets HTTPClient to http;

    String serverPath = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  // Gets together key and token into one http server path

    http.begin(serverPath.c_str()); // Starts http with giver server path

    httpResponseCodeRead = http.GET();  // Gets http response and stores it into "httpResponseCodeRead"

    String payload = http.getString();  // Stores payload from http as string

    JsonObject& root = jsonBuffer.parseObject(payload); // Using Json to decode payload
    double httpResponse = root["Value"]; // Stores "Value" from "payload" into "httpResponse"

    String dataRegister = String(httpResponse); // Defines dataRegister as string of "httpResponse"

    indoor = (dataRegister.substring(1, 4).toInt());    // Stores current indoor temperature 
    outdoor = (dataRegister.substring(4, 7).toInt());   // Stores current outdoor temperature 
    outdoor = outdoor - 100;  // Calculates real temperature

    http.end(); // Ends http
  }
}

String Scroll_LCD_Left(String StrDisplay) { // Function for scrolling text, expects string with text into it
  String result;  // Defines sting "result"
  String StrProcess = "                " + StrDisplay + "                "; // Adding spaces before and after text
  result = StrProcess.substring(Li,Lii);  // Updates "result" for futher use
  Li++;   // Adds 1 to "Li"
  Lii++;  // Adds 1 to "Lii"
  if (Li>StrProcess.length()){  // If string is finished to display
    Li=16;  // Resets values of "Li" and "Lii"
    Lii=0;
  }
  return result;  // Returns value of function from "result" variable
}

void LEDtest(){ // Tests LEDs before main screen
  digitalWrite(LEDGreen, HIGH); // Sets LED to HIGH
  digitalWrite(LEDWhite, HIGH); // Sets LED to HIGH
  digitalWrite(LEDRed, HIGH);   // Sets LED to HIGH

  delay(500); // Delay 500 ms

  digitalWrite(LEDGreen, LOW);  // Sets LED to LOW
  digitalWrite(LEDWhite, LOW);  // Sets LED to LOW
  digitalWrite(LEDRed, LOW);    // Sets LED to LOW
}

void bellScreen() { // Function for ring bell controll and response
  bellCoTRead(bellKEY, TOKEN);  // Calls "bellCoTRead" function

  if (bellValue == 1) { // If "bellValue" is 1
    lcdWrt = true;  // "lcdWrt" is set to true
    unsigned long previousMillis1;  // Defines "previousMillis1"
    unsigned long previousMillis2;  // Defines "previousMillis2"
    int ledState = LOW; // Sets "ledState" to LOW

    while (1){  // Starts infinite while loop for the screen
      currentMillis = millis(); // Stores current millis into "currentMillis"

      if (lcdWrt) { // If "lcdWrt" is true
        lcd.clear();  // Clears the display

        lcd.setCursor(4,0);
        lcd.print("Ring bell"); // Prints "Ring bell" on LCD

        lcd.setCursor(0,1);
        lcd.print("Open Guest Deny"); // Prints "Open Ignore Deny" on LCD

        lcdWrt = false; // Sets "lcdWrt" to flase
      }

      if (currentMillis - previousMillis1 > 200) {  // If "currentMillis" - "previousMillis1" is larger than 200 ms
        previousMillis1 = currentMillis;  // Stores "currentMillis" in "previousMillis1"

        if (ledState == LOW) {  // If "ledState" is LOW
          ledState = HIGH;  // Sets "ledState" to HIGH
        } else {  // Else
          ledState = LOW; // Sets "ledState" to LOW
        }
      }      
      digitalWrite(LEDWhite, ledState); // Turns "LEDWhite" with given state

      if (currentMillis - previousMillis2 > 1000) { // If "currentMillis" - "previousMillis2" is larger than 1000 ms
        previousMillis2 = currentMillis;  // Stores "currentMillis" in "previousMillis2"

        ledcWriteTone(channel, 500);  // Turns buzzer with tone 500
        delay(200); // Delay 200 ms
        ledcWriteTone(channel, 0);  // Turns buzzer with tone 0
      }

      if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
        debounce(); // Calls "debounce" function

        digitalWrite(LEDWhite, LOW);

        String value = bellValueBefore + "2" + bellValueAfter;

        CoTWrite(bellKEY, TOKEN, value);

        mainScreen(); // Calls up "mainScreen" function
      }
      if (digitalRead(buttonMiddle) == LOW){  // If middle button is pressed down
        debounce(); // Calls "debounce" function

        digitalWrite(LEDWhite, LOW);

        guestCoTRead(guestKEY, TOKEN);

        if (guestValue > 1) {
          lcd.clear();  // Clears the display

          lcd.setCursor(0,0);
          lcd.print("Max guest number"); // Prints "Ring bell" on LCD

          String value = bellValueBefore + "3" + bellValueAfter;

          CoTWrite(bellKEY, TOKEN, value);

          delay(5000);

          mainScreen();
        }
        else {
          guestValue++;

          String value1 = guestValueBefore + String(guestValue) + guestValueAfter;

          CoTWrite(guestKEY, TOKEN, value1);

          String value2 = bellValueBefore + "2" + bellValueAfter;

          CoTWrite(bellKEY, TOKEN, value2);
        }
        
        mainScreen(); // Calls up "mainScreen" function
      }
      if (digitalRead(buttonRight) == LOW){  // If right button is pressed down
        debounce(); // Calls "debounce" function

        digitalWrite(LEDWhite, LOW);
        
        String value = bellValueBefore + "3" + bellValueAfter;

        CoTWrite(bellKEY, TOKEN, value);

        mainScreen(); // Calls up "mainScreen" function
      }
      
    }
  }

  if (bellValue == 4) { // If "bellValue" is 4
    roomCoTRead(roomKEY, TOKEN);

    if (roomStatus == 0) {
      roomStatus = 1;
      roomCoTWrite(roomKEY, TOKEN);
    }
    else if (roomStatus == 1) {
      roomStatus = 0;
      roomCoTWrite(roomKEY, TOKEN);
    }
    else if (roomStatus == 2) {
      roomStatus = 0;
      roomCoTWrite(roomKEY, TOKEN);
    }
    String value = bellValueBefore + "0" + bellValueAfter;

    CoTWrite(bellKEY, TOKEN, value);
  }
}

void mainScreen() { // Function for showing main screen
  lcdWrt = true; // Sets "lcdWrt" to true

  char currentClock[16]; // Defines variable "currentClock" that stores current time in format "00:00"
  String clock;                   // Defines "clock" variable
  unsigned long previousMillis1;  // Defines "previousMillis1" variable
  unsigned long previousMillis2;  // Defines "previousMillis2" variable

  // Resets values to default
  bookingMinutes = 5;  // Stores amount of booking time in minutes
  bookingPeople = 1;   // Stores number of people who will be book
  menu = "room";         // Stores which menu will be opened in menu screen
  booking = "Kitchen";   // Stores which room will be opened in booking screen
  settings = "temp";     // Stores which setting will be opened in settings screen

  int previousOutdoor;    // Defines "previousOutdoor" variable
  int previousIndoor;     // Defines "previousIndoor" variable
  String previousClock;   // Defines "previousClock" variable
  String roomStatusName;
  
  while (1){  // Starts infinite while loop for the screen
    currentMillis = millis(); // Stores current millis into "currentMillis"

    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears the display

      lcd.setCursor(0,0);
      lcd.print(outdoor); // Prints outdoor temperature
      lcd.write((byte)0);
      lcd.print("/");
      lcd.print(indoor);  // Prints indoor temperature
      lcd.write((byte)0);

      if(WiFi.status() == WL_CONNECTED){  // If WiFi status is connected
        lcd.setCursor(9,0); // Show WiFi symbol
        lcd.write((byte)2);
      }
      else {  // Else 
        lcd.setCursor(9,0); // Show Error symbol
        lcd.write((byte)3);
      }

      lcd.setCursor(11,0);  // Prints current clock on display
      lcd.print(clock);  

      lcd.setCursor(0,1);  // Prints current clock on display
      lcd.print(roomStatusName);  
      
      lcdWrt = false; // Sets "lcdWrt" to false
    }

    sprintf(currentClock,"%02u:%02u", hour(),minute()); // Getting time from internal clock and writes it in correct format
    clock = String(currentClock); // Stores "currentClock" as a string into "clock"

    if ((outdoor != previousOutdoor) or (indoor != previousIndoor) or (clock != previousClock)) { // If any of values has been updated
      previousOutdoor = outdoor;  // Updates "previousOutdoor" with "outdoor" value
      previousIndoor = indoor;    // Updates "previousIndoor" with "indoor" value
      previousClock = clock;      // Updates "previousClock" with "clock" value
      
      lcdWrt = true;  // Sets "lcdWrt" to true
    }

    if (currentMillis - previousMillis1 > 5000) { // If current millis minus previous millis are larger than 5000 ms
        previousMillis1 = currentMillis;  // Updates "previousMillis1" with "currentMillis" value

        bellScreen(); // Calls "bellScreen" function
    }

    if (currentMillis - previousMillis2 > 60000) {  // If current millis minus previous millis are larger than 60000 ms
        previousMillis2 = currentMillis;  // Updates "previousMillis2" with "currentMillis" value

        dataCoTRead(dataKEY, TOKEN);  // Calls "dataCoTRead" function
    }

    if (roomStatus == 0) {
      roomStatusName = "Home";
    }
    else if (roomStatus == 1) {
      roomStatusName = "Away";
    }
    else if (roomStatus == 2) {
      roomStatusName = "Vacation";
    }

    if (digitalRead(buttonRight) == LOW){  // If middle button is pressed down
      debounce(); // Calls "debounce" function
      menuScreen(); // Calls "menuScreen" function
    }
  }
}

void menuScreen() { // Function for showing menu screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true

  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
     
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(6,0); // Prints "Menu"
      lcd.print("Menu");

      if (menu == "room") { // If "menu" variable is set to "room"
        lcd.setCursor(1,1); // Prints "Room settings" with arrows
        lcd.print("Room settings ");
        lcd.write((byte)1);
      }

      else if (menu == "booking") { // If "menu" variable is set to "booking"
        lcd.setCursor(4,1); // Prints "Booking" with arrows
        lcd.print("Booking ");
        lcd.write((byte)1);
      }

      else if (menu == "guest") { // If "menu" variable is set to "guest"
        lcd.setCursor(5,1); // Prints "Guests" with arrows
        lcd.print("Guests ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false
    }
    
    if (menu == "room") { // If "menu" variable is set to "room"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        menu = "guest"; // Sets "menu" variable to "guest"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        menu = "booking"; // Sets "menu" variable to "booking"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }

      if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
        debounce(); // Calls "debounce" function
        roomScreen();  // Calls "roomSettingsScreen" function
      }
    }

    else if (menu == "booking") { // If "menu" variable is set to "booking"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        menu = "room";  // Sets "menu" variable to "room"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
      if (enc1.isRight()) {  // If encoder is rotated to the left
        menu = "guest"; // Sets "menu" variable to "guest"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
      if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
        debounce(); // Calls "debounce" function
        bookingScreen();  // Calls "bookingScreen" function
      }
    }

    else if (menu == "guest") { // If "menu" variable is set to "guest"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        menu = "booking"; // Sets "menu" variable to "booking"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
      if (enc1.isRight()) {  // If encoder is rotated to the right
        menu = "room";  // Sets "menu" variable to "room"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }

      if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
        debounce(); // Calls "debounce" function
        guestScreen();  // Calls "guestScreen" function
      }
    }

    if (digitalRead(buttonLeft) == LOW){ // If left button is pressed down
      debounce(); // Calls "debounce" function
      mainScreen(); // Calls "mainScreen"
    }
    
    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void roomScreen() { // Function for showing room settings screen
  roomCoTRead(roomKEY, TOKEN);

  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(2,0);   // Prints "Booking"
      lcd.print("Room settings");

      if (settings == "temp") { // If "settings" variable is set to "fan"
        Serial.println("temperature");
        lcd.setCursor(2,1); // Prints "Room settings" with arrows
        lcd.print("Temperature ");
        lcd.write((byte)1);
      }

      else if (settings == "fan") { // If "settings" variable is set to "fan"
        lcd.setCursor(2,1); // Prints "Room settings" with arrows
        lcd.print("Fan conrol ");
        lcd.write((byte)1);
      }

      else if (settings == "window") { // If "settings" variable is set to "window"
        lcd.setCursor(0,1); // Prints "Booking" with arrows
        lcd.print("Window control ");
        lcd.write((byte)1);
      }

      else if (settings == "light") { // If "settings" variable is set to "light"
        Serial.println("light");
        lcd.setCursor(5,1); // Prints "Guests" with arrows
        lcd.print("Light ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false
    }
    
    if (settings == "temp") { // If "settings" variable is set to "temp"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        settings = "light"; // Sets "settings" variable to "light"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        settings = "fan"; // Sets "settings" variable to "fan"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){
        debounce(); // Calls "debounce" function
        roomTempScreen();  // Calls "roomTempScreen" function
      }
    }
    
    else if (settings == "fan") { // If "settings" variable is set to "fan"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        settings = "temp"; // Sets "settings" variable to "temp"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        settings = "window"; // Sets "settings" variable to "window"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){
        debounce(); // Calls "debounce" function
        roomFanScreen();  // Calls "roomFanScreen" function
      }
    }

    else if (settings == "window") { // If "settings" variable is set to "window"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        settings = "fan";  // Sets "settings" variable to "fan"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        settings = "light"; // Sets "settings" variable to "light"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){
        debounce(); // Calls "debounce" function
        roomWindowScreen();  // Calls "roomWindowScreen" function
      }
    }

    else if (settings == "light") { // If "settings" variable is set to "light"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        settings = "window"; // Sets "settings" variable to "window"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        settings = "temp";  // Sets "settings" variable to "temp"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
        debounce(); // Calls "debounce" function
        roomLightScreen();  // Calls "bookingTimeScreen" function
      }
    }

    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      menuScreen(); // Calls "menuScreen" function
    }
    
    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void roomTempScreen() { // Function for showing room temperature control screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(3,0);   // Prints "Fan conrol"
      lcd.print("Temperature");

      if (tempMode == 0) {  // If "tempMode" variable is set to 0
        lcd.setCursor(6,1); // Prints "OFF" with arrows
        lcd.print("OFF ");
        lcd.write((byte)1);
      }

      else if (tempMode == 1) {  // If "tempMode" variable is set to 1
        lcd.setCursor(5,1); // Prints "AUTO" with arrows
        lcd.print("AUTO ");
        lcd.write((byte)1);
      }

      else {  // Else:
        lcd.setCursor(6,1); // Prints temperature to thermostat
        lcd.print(tempMode);
        lcd.write((byte)0);
        lcd.print(" ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false      
    }
  
    if (enc1.isLeft()) { // If encoder is rotated to the left
      tempMode = tempMode - 1;  // Subtracts 1 from "tempMode"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }
    else if (enc1.isRight()) { // If encoder is rotated to the right
      tempMode = tempMode + 1;  // Adds 1 to "tempMode"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }

    if (tempMode < 0) {  // If "tempMode" is less than 0
      tempMode = 0;  // Sets "tempMode" to 0
    }
    if (tempMode > 30) {  // If "tempMode" is more than 30
      tempMode = 30;  // Sets "tempMode" to 30
    }

    if (digitalRead(buttonMiddle) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomCoTWrite(roomKEY, TOKEN); // Calls "" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomScreen(); // Calls "roomScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void roomFanScreen() {  // Function for showing room fan control screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(3,0);   // Prints "Fan conrol"
      lcd.print("Fan conrol");

      if (fanMode == 0) { // If "fanMode" variable is set to 0
        lcd.setCursor(6,1);   // Prints "OFF" with arrows
        lcd.print("OFF ");
        lcd.write((byte)1);
      }
      
      if (fanMode == 1) { // If "fanMode" variable is set to 1
        lcd.setCursor(5,1);   // Prints "AUTO" with arrows
        lcd.print("AUTO ");
        lcd.write((byte)1);
      }
      
      if (fanMode == 2) { // If "fanMode" variable is set to 2
        lcd.setCursor(4,1);   // Prints "Medium" with arrows
        lcd.print("Medium ");
        lcd.write((byte)1);
      }

      if (fanMode == 3) { // If "fanMode" variable is set to 3
        lcd.setCursor(4,1);   // Prints "Maximum" with arrows
        lcd.print("Maximum ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false
    }

    if (fanMode == 0) { // If "fanMode" variable is set to 0
      if (enc1.isLeft()) { // If encoder is rotated to the left
        fanMode = 3; // Sets "fanMode" variable to 3
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        fanMode = 1; // Sets "fanMode" variable to 1
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (fanMode == 1) { // If "fanMode" variable is set to 1
      if (enc1.isLeft()) { // If encoder is rotated to the left
        fanMode = 0; // Sets "fanMode" variable to 0
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        fanMode = 2; // Sets "fanMode" variable to 2
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (fanMode == 2) { // If "fanMode" variable is set to 2
      if (enc1.isLeft()) { // If encoder is rotated to the left
        fanMode = 1; // Sets "fanMode" variable to 1
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        fanMode = 3; // Sets "fanMode" variable to 3
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (fanMode == 3) { // If "fanMode" variable is set to 3
      if (enc1.isLeft()) { // If encoder is rotated to the left
        fanMode = 2; // Sets "fanMode" variable to 2
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        fanMode = 0; // Sets "fanMode" variable to 0
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }
    
    if (digitalRead(buttonMiddle) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomCoTWrite(roomKEY, TOKEN); // Calls "" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomScreen(); // Calls "roomScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void roomWindowScreen() {  // Function for showing room window control screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(2,0);   // Prints "Fan conrol"
      lcd.print("Window conrol");

      if (windowMode == 0) {  // If "windowMode" variable is set to 0
        lcd.setCursor(6,1);   // Prints "OFF" with arrows
        lcd.print("OFF ");
        lcd.write((byte)1);
      }
      
      if (windowMode == 1) {  // If "windowMode" variable is set to 1
        lcd.setCursor(5,1);   // Prints "AUTO" with arrows
        lcd.print("AUTO ");
        lcd.write((byte)1);
      }
      
      if (windowMode == 2) {  // If "windowMode" variable is set to 2
        lcd.setCursor(1,1);   // Prints "Medium" with arrows
        lcd.print("Half opened ");
        lcd.write((byte)1);
      }

      if (windowMode == 3) {  // If "windowMode" variable is set to 3
        lcd.setCursor(4,1);   // Prints "Maximum" with arrows
        lcd.print("Opened ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false
    }

    if (windowMode == 0) { // If "windowMode" variable is set to 0
      if (enc1.isLeft()) { // If encoder is rotated to the left
        windowMode = 3; // Sets "windowMode" variable to 3
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        windowMode = 1; // Sets "windowMode" variable to 1
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (windowMode == 1) { // If "windowMode" variable is set to 1
      if (enc1.isLeft()) { // If encoder is rotated to the left
        windowMode = 0; // Sets "windowMode" variable to 0
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        windowMode = 2; // Sets "windowMode" variable to 2
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (windowMode == 2) { // If "windowMode" variable is set to 2
      if (enc1.isLeft()) { // If encoder is rotated to the left
        windowMode = 1; // Sets "windowMode" variable to 1
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        windowMode = 3; // Sets "windowMode" variable to 3
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (windowMode == 3) { // If "windowMode" variable is set to 3
      if (enc1.isLeft()) { // If encoder is rotated to the left
        windowMode = 2; // Sets "windowMode" variable to 2
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        windowMode = 0; // Sets "windowMode" variable to 0
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }
    
    if (digitalRead(buttonMiddle) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomCoTWrite(roomKEY, TOKEN); // Calls "" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomScreen(); // Calls "roomScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void roomLightScreen() {  // Function for showing room light control screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(2,0);   // Prints "Fan conrol"
      lcd.print("Light control");

      if (lightMode == 0) {
        lcd.setCursor(6,1);   // Prints "OFF" with arrows
        lcd.print("OFF ");
        lcd.write((byte)1);
      }

      else if (lightMode == 256) {
        lcd.setCursor(5,1);   // Prints "AUTO" with arrows
        lcd.print("AUTO ");
        lcd.write((byte)1);
      }

      else if (lightMode == 170) {
        lcd.setCursor(6,1);   // Prints "Low" with arrows
        lcd.print("Low ");
        lcd.write((byte)1);
      }

      else if (lightMode == 210) {
        lcd.setCursor(4,1);   // Prints "Medium" with arrows
        lcd.print("Medium ");
        lcd.write((byte)1);
      }

      else if (lightMode == 255) {
        lcd.setCursor(5,1);   // Prints "High" with arrows
        lcd.print("High ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false      
    }
  
    if (lightMode == 0) { // If "lightMode" variable is set to 0 (OFF)
      if (enc1.isLeft()) { // If encoder is rotated to the left
        lightMode = 255; // Sets "lightMode" variable to 255 (HIGH)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        lightMode = 256; // Sets "lightMode" variable to 256 (AUTO)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (lightMode == 256) { // If "lightMode" variable is set to 256 (AUTO)
      if (enc1.isLeft()) { // If encoder is rotated to the left
        lightMode = 0; // Sets "lightMode" variable to 0 (OFF)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        lightMode = 84; // Sets "lightMode" variable to 84 (Low)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }

    else if (lightMode == 170) { // If "lightMode" variable is set to Low
      if (enc1.isLeft()) { // If encoder is rotated to the left
        lightMode = 256; // Sets "lightMode" variable to 256 (AUTO)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        lightMode = 210; // Sets "lightMode" variable to 169 (Medium)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
    }

    else if (lightMode == 210) { // If "lightMode" variable is set to Medium
      if (enc1.isLeft()) { // If encoder is rotated to the left
        lightMode = 170; // Sets "lightMode" variable to 84 (Low)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        lightMode = 255; // Sets "lightMode" variable to 255 (High)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
    }

    else if (lightMode == 255) { // If "lightMode" variable is set to High
      if (enc1.isLeft()) { // If encoder is rotated to the left
        lightMode = 210; // Sets "lightMode" variable to 169 (Medium)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        lightMode = 0; // Sets "lightMode" variable to 0 (OFF)
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      
    }
    
    if (digitalRead(buttonMiddle) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomCoTWrite(roomKEY, TOKEN); // Calls "" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      roomScreen(); // Calls "roomScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void bookingScreen() { // Function for showing booking screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true

  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(4,0);   // Prints "Booking"
      lcd.print("Booking");

      if (booking == "Kitchen") { // If "menu" variable is set to "Kitchen"
        lcd.setCursor(4,1); // Prints "Kitchen" with arrows
        lcd.print("Kitchen ");
        lcd.write((byte)1);
      }

      else if (booking == "Bathroom") { // If "menu" variable is set to "Bathroom"
        lcd.setCursor(3,1); // Prints "Bathroom" with arrows
        lcd.print("Bathroom ");
        lcd.write((byte)1);
      }

      else if (booking == "Living room") { // If "menu" variable is set to "Living room"
        lcd.setCursor(2,1); // Prints "Living room" with arrows
        lcd.print("Living room ");
        lcd.write((byte)1);
      }

      lcdWrt = false; // Resets "lcdWrt" to false
    }
    
    if (booking == "Kitchen") { // If "booking" variable is set to "Kitchen"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        booking = "Living room"; // Sets "booking" variable to "Living room"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        booking = "Bathroom"; // Sets "booking" variable to "Bathroom"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){
        debounce(); // Calls "debounce" function
        bookingTimeScreen();  // Calls "bookingTimeScreen" function
      }
    }

    else if (booking == "Bathroom") { // If "booking" variable is set to "Bathroom"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        booking = "Kitchen";  // Sets "booking" variable to "Kitchen"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        booking = "Living room"; // Sets "booking" variable to "Living room"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){
        debounce(); // Calls "debounce" function
        bookingTimeScreen();  // Calls "bookingTimeScreen" function
      }
    }

    else if (booking == "Living room") { // If "booking" variable is set to "Living room"
      if (enc1.isLeft()) { // If encoder is rotated to the left
        booking = "Bathroom"; // Sets "booking" variable to "Bathroom"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (enc1.isRight()) {  // If encoder is rotated to the right
        booking = "Kitchen";  // Sets "booking" variable to "Kitchen"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
      if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
        debounce(); // Calls "debounce" function
        bookingTimeScreen();  // Calls "bookingTimeScreen" function
      }
    }

    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      menuScreen(); // Calls "menuScreen" function
    }
    
    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void bookingTimeScreen() {  // Function for showing booking time screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  char bookingClock[16];
  int timedif = 0;  // Defines how far is time from closest 5 min timestamp in seconds
  lcdWrt = true; // Sets "lcdWrt" to true

  getClosestTimestamp(); // Calls "getClosestTimestamp" function

  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function
    
    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen

      lcd.setCursor(2,0); // Prints "Booking time"
      lcd.print("Booking time");

      bookingUnix = closestUnix + timedif; // Finds booking time in unix out from closest 5 min timestamp and timedif variable

      time_t n = bookingUnix; // Converts "bookingUnix" to time format and stores it into "n" variable
  
      sprintf(bookingClock,"%02u:%02u", hour(n),minute(n)); // Getting time from "n" variable and writes it in correct format
  
      lcd.setCursor(5,1); // Prints "bookingClock" with arrows
      lcd.print(bookingClock);
      lcd.print(" ");
      lcd.write((byte)1);
    
      lcdWrt = false; // Resets "lcdWrt" to false
    }

    if (enc1.isLeft()) { // If encoder is rotated to the left
      timedif = timedif - 300;  // Subtracts 300 seconds (5 min) from "timedif"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }
    else if (enc1.isRight()) { // If encoder is rotated to the right
      timedif = timedif + 300;  // Adds 300 seconds (5 min) to "timedif"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }

    if (timedif < 0) {  // If "timedif" is less than 0
      timedif = 0;  // Sets "timedif" to 0
    }
    if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
      debounce(); // Calls "debounce" function
      bookingDurationScreen();  // Calls "bookingDurationScreen" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      bookingScreen();  // Calls "bookingScreen" function
    }

    timeout(); // Calls "timeout" function
    
    debounce(); // Calls "debounce" function
  }
}

void bookingDurationScreen() {  // Function for showing booking time screen
  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function

    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen
      
      lcd.setCursor(0,0); // Prints "Booking duration"
      lcd.print("Booking duration");

      lcd.setCursor(5,1); // Prints "minutes" with arrows
      lcd.print(bookingMinutes);
      lcd.print(" min ");
      lcd.write((byte)1);
    
      lcdWrt = false; // Resets "lcdWrt" to false
    }

    if (enc1.isLeft()) { // If encoder is rotated to the left
      bookingMinutes = bookingMinutes - 5;  // Subtracts 5 min from "bookingMinutes"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }
    else if (enc1.isRight()) { // If encoder is rotated to the right
      bookingMinutes = bookingMinutes + 5;  // Adds 5 min to "bookingMinutes"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }

    if (bookingMinutes < 5) {  // If "bookingMinutes" is less than 5
      bookingMinutes = 5;  // Sets "bookingMinutes" to 5
    }

    if (bookingMinutes > 240) {  // If "bookingMinutes" is more than 5
      bookingMinutes = 240;  // Sets "bookingMinutes" to 5
    }
    
    if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
      debounce(); // Calls "debounce" function
      bookingPeopleScreen();  // Calls "bookingPeopleScreen" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      bookingTimeScreen();  // Calls "bookingTimeScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void bookingPeopleScreen() {  // Function for showing booking time screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function

    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen
      
      lcd.setCursor(0,0); // Prints "Number of people"
      lcd.print("Number of people");

      lcd.setCursor(7,1); // Prints "bookingMinutes" with arrows
      lcd.print(bookingPeople);
      lcd.print(" ");
      lcd.write((byte)1);
    
      lcdWrt = false; // Resets "lcdWrt" to false
    }

    if (enc1.isLeft()) { // If encoder is rotated to the left
      bookingPeople = bookingPeople - 1;  // Subtracts 1 from "bookingMinutes"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }
    else if (enc1.isRight()) { // If encoder is rotated to the right
      bookingPeople = bookingPeople + 1;  // Adds 1 to "bookingMinutes"
      lcdWrt = true;  // Sets "lcdWrt" to true
    }

    if (bookingPeople < 1) {  // If "bookingPeople" is less than 1
      bookingPeople = 1;  // Sets "bookingPeople" to 1
    }
    if (bookingPeople > 3) {  // If "bookingPeople" is more than 3
      bookingPeople = 3;  // Sets "bookingPeople" to 3
    }
    if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
      debounce(); // Calls "debounce" function
      bookingOverviewScreen();  // Calls "bookingOverviewScreen" function
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      bookingDurationScreen();  // Calls "bookingDurationScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void bookingOverviewScreen() { // Function for showing booking overview screen
  resetMillisFunc(); // Calls "resetMillisFunc" function

  char clockStart[16];  // Defines variable "clockStart" that stores start of the booking in format "00:00"
  char clockStop[16];   // Defines variable "clockStop" that stores stop of the booking in format "00:00"

  Li = 16;  // Variable for scrolling text
  Lii = 0;  // Variable for scrolling text
  
  lcd.clear();  // Clears lcd screen

  lcd.setCursor(4,0); // Prints "Overview"
  lcd.print("Overview");

  time_t a = bookingUnix; // Converts "bookingUnix" to time format and stores it into "a" variable
  time_t b = bookingUnix + (bookingMinutes * 60);  // Converts "bookingUnix" with additional booking time to time format and stores it into "b" variable

  sprintf(clockStart,"%02u:%02u", hour(a),minute(a)); // Getting time from "a" variable and writes it in correct format
  sprintf(clockStop,"%02u:%02u", hour(b),minute(b));  // Getting time from "b" variable and writes it in correct format
  
  String overviewString = String(booking) + " | " + String(clockStart) + "->" + String(clockStop) + " | " + String(bookingMinutes) + " min | " + String(bookingPeople) + " person"; // Creates "overviewString" string from several values and adds some symbols

  while(1) {  // Starts infinite while loop for the rolling text
    currentMillis = millis(); // Stores current millis into "currentMillis"
    if (currentMillis - previousMillis > 500) { // If current millis minus previous millis are larger than 500 ms
      lcd.setCursor(0,1);
      lcd.print(Scroll_LCD_Left(overviewString)); // Prints string from the function "Scroll_LCD_Left" with defined string "overviewString"

      previousMillis = millis();  // Stores current millis into "previousMillis"
    }
    if (digitalRead(buttonRight) == LOW){ // If right button is pressed down
      debounce(); // Calls "debounce" function
      bookingRequestScreen();
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      bookingPeopleScreen();  // Calls "bookingPeopleScreen" function
    }

    timeout(); // Calls "timeout" function
  }
}

void bookingRequestScreen() { // Sends request to the server, waits for response and displays the answer
  char duration[16];  // Defines "duration" variable
  int roomID; // Defines "roomID" variable

  int i = 0;  // Defines "i" variable with value 0
  int counter = 0;  // Defines "counter" variable with value 0
  String loading = "."; // Defines "loading" string with value "."

  if (booking == "Kitchen") { // If "booking" is "Kitchen"
    roomID = 1; // Set "roomID" to 1
  }
  else if (booking == "Bathroom") { // If "booking" is "Bathroom"
    roomID = 2; // Set "roomID" to 2
  }
  else if (booking == "Living room") {  // If "booking" is "Living room"
    roomID = 3; // Set "roomID" to 3
  }

  sprintf(duration,"%03d", bookingMinutes); // Sets "duration" variable with value from "bookingMinutes" with 3 digits

  String requestRegister = String(roomID) + String (duration) + String(bookingUnix) + String(bookingPeople);  // Creates "requestRegister" string from several values and adds some symbols

  CoTWrite(bookingKEY, TOKEN, requestRegister); // Calls "CoTWrite" function with given variables as "bookingKEY", "TOKEN" and "requestRegister"

  if (httpResponseCodeWrite > 0) {  // If "httpResponseCodeWrite" code is larger than 0
    lcd.clear();  // Clears lcd screen

    lcd.setCursor(4,0); // Prints "Request" on LCD
    lcd.print("Request");

    lcd.setCursor(0,1); // Prints "Sent successful" on LCD
    lcd.print("Sent successful");

    delay(5000); // Delay 5000 ms
  }
  
  else {  // Else
    lcd.clear();  // Clears lcd screen

    lcd.setCursor(2,0); // Prints "Request" on LCD 
    lcd.print("Request");

    lcd.setCursor(5,1); // Prints "ERROR" on LCD
    lcd.print("ERROR");
    
    delay(5000);

    mainScreen();
  }

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function

    currentMillis = millis(); // Stores current millis into "currentMillis"

    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen
      
      lcd.setCursor(5,0); // Prints "Number of people"
      lcd.print("Waiting");

      lcd.setCursor(5,1); // Prints "Number of people"
      lcd.print(loading);

      lcdWrt = false; // Resets "lcdWrt" to false      
    }

    if (currentMillis - previousMillis > 1000) { // If current millis minus previous millis are larger than 500 ms
      previousMillis = millis();  // Stores current millis into "previousMillis"

      String response = CoTRead(bookingKEY, TOKEN); // Stores value from function "CoTRead" with given strings into "response" string

      if (response == "111") {  // If "response" is 111
        lcd.clear();  // Clears lcd screen

        lcd.setCursor(1,0); // Prints "Room is booked"
        lcd.print("Room is booked");

        digitalWrite(LEDGreen, HIGH); // Turns green LED on

        delay(10000); // Delay 10000 ms

        digitalWrite(LEDGreen, LOW);  // Turns green LED off

        mainScreen(); // Calls "mainScreen" function
      }
      else if (response == "333") { // If "response" is 333
        lcd.clear();  // Clears lcd screen

        lcd.setCursor(1,0); // Prints "Booking denied"
        lcd.print("Booking denied");

        digitalWrite(LEDRed, HIGH); // Turns red LED on

        delay(10000); // Delay 10000 ms

        digitalWrite(LEDRed, LOW);  // Turns red LED off

        mainScreen(); // Calls "mainScreen" function
      }
      else {  // Else
        loading = loading + ".";  // Adds "." to "loading" string

        if (i == 6) { // If "i" is 6 
          loading = ".";  // Sets "loading" to "."
          i = 0;  // Resets "i" to 0
        }
        if (counter >= 30) {  // If "counter" is 30 or larger
          lcd.clear();  // Clears lcd screen

          lcd.setCursor(5,0); // Prints "Timeout"
          lcd.print("Timeout");

          digitalWrite(LEDRed, HIGH); // Turns red LED on

          delay(10000); // Delay 10000 ms

          digitalWrite(LEDRed, LOW);  // Turns red LED off

          mainScreen(); // Calls "mainScreen" function
        }        

        i++;  // Adds 1 to "i"
        counter++;  // Adds 1 to "counter"
        lcdWrt = true;  // Sets "lcdWrt" to true
      }
    }

    if (digitalRead(buttonLeft) == LOW){ // If left button is pressed down
      debounce(); // Calls "debounce" function
      bookingOverviewScreen();  // Calls "" function
    }

    debounce(); // Calls "debounce" function
  }  
}

void guestScreen() {
  guestCoTRead(guestKEY, TOKEN);

  lcdWrt = true; // Sets "lcdWrt" to true
  
  while (1){  // Starts infinite while loop for the screen
    enc1.tick(); // Call encoder tick function

    if (lcdWrt) { // Checks if screen is needed to be updated or not
      lcd.clear();  // Clears lcd screen
      
      lcd.setCursor(1,0); // Prints "Booking duration"
      lcd.print("Guest leaving?");

      lcd.setCursor(6,1); // Prints "minutes" with arrows
      lcd.print("Yes");
    
      lcdWrt = false; // Resets "lcdWrt" to false
    }
    
    if (digitalRead(buttonMiddle) == LOW){ // If right button is pressed down
      debounce(); // Calls "debounce" function
      
      if (guestValue > 0) {
        guestValue--;

        String value = guestValueBefore + String(guestValue) + guestValueAfter;

        lcd.setCursor(0,1); // Prints "Wait" on LCD
        lcd.print("      Wait      ");

        CoTWrite(guestKEY, TOKEN, value);

        if (httpResponseCodeWrite == 200) { // If "httpResponseCodeWrite" is 200
          lcd.setCursor(0,1); // Prints "Set" on LCD
          lcd.print("       Set      ");      

          delay(2000);  // Delay 2000 ms
        }
      }
      else {
        lcd.clear();  // Clears lcd screen
      
        lcd.setCursor(1,0); // Prints "Booking duration"
        lcd.print("No guests found");

        delay(5000);

        menuScreen();
      }
    }
    if (digitalRead(buttonLeft) == LOW){  // If left button is pressed down
      debounce(); // Calls "debounce" function
      menuScreen();  // Calls "menuScreen" function
    }

    timeout(); // Calls "timeout" function

    debounce(); // Calls "debounce" function
  }
}

void setup() {
  lcd.begin(16, 2); // Starts lcd with parameters

  Serial.begin(115200); // Starts Serial port comunication for debugging

  getPinMode(); // Calls "getButtons" function

  LEDtest();  // Calls "LEDtest" function

  getWiFi();  // Calls "getWiFi" function

  getCharacter(); // Calls "getCharacter" function

  configTime(0, 0, ntpServer);  // Sets "ntpServer" as server for unix time
  getCurrentTime(); // Calls "getCurrentTime" function
  
  enc1.setType(TYPE2); // Set type of encoder (TYPE1 - one step, TYPE2 - two step)
}

void loop() {
  mainScreen(); // Calls "mainScreen" function
}

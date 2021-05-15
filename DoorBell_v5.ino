#include <SPI.h>            // Importing library for Serial Peripheral Interface Bus
#include <MFRC522.h>        // Importing library for RFID-reader
#include <WiFi.h>           // Importing library for WiFi
#include <HTTPClient.h>     // Importing library for HTTP
#include <ArduinoJson.h>    // Importing library for Json


String TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0";  // CoT token
String bellKEY = "1613";  // CoT signal key for the bell
String bellRegister; //stores bell  register

int httpResponseCodeWrite;  // Stores response code from HTTP write function
int httpResponseCodeRead; // Stores response code from HTTP reed function

int bellValue;  // Stores current value for the bell
String bellValueBefore; // Stores all values before "bellValue"
String bellValueAfter;  // Stores all values after "bellValue"

int bellTimeout = 180000; // defines timeout time for doorbell.

const char* ssid = "SkNisse"; // SSID for WiFi hotpost
const char* password = "c71abfee7311";  // Password for WiFi hotpost


char *keyCardUID[] = {"84 D1 D6 31", "82 EA 320D", "7304 E1 F9", "C2 B6 F6 10", "1D 9B04 B0", "44 84 D7 31"}; // UIDs from registred keycards
uint8_t numberOfCards = 6; // Number of registered KeyCards
 

uint8_t RST_PIN = 17; //MFRC522 RST pin  
uint8_t SS_PIN = 5;  //MFRC522 SS pin (SDA)
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// Led input pins
uint8_t greenLed = 15;
uint8_t whiteLed = 2;
uint8_t redLed = 0;


int buttonPins[] = {12,14,27,26,25,33}; // Create array with button pins
uint8_t pinCount = 6; // Number of button pins used

uint8_t buzzer = 32; // buzzer output pin
uint8_t freq = 2000; // buzzer frequency
uint8_t channel = 2; // buzzer PWM channel
uint8_t res = 12;

uint8_t sensor = 34; //motion sensor pin
unsigned long sleepTimer; // 

int sleepTimeout = 60000; // sets amount of time before esp goes to sleep

void getWiFi() {  // Sets up WiFi
  WiFi.begin(ssid, password); // Begins WiFi
  Serial.println("Connecting"); // Prints "Connecting"
  while(WiFi.status() != WL_CONNECTED) {  // While WiFi isnt connected
    
    Serial.print(".");  // Prints "."
    
    delay(500);
  }
  if(WiFi.status() == WL_CONNECTED){  // If WiFi connected
    Serial.println("Success");  // Prints "Success"
  }
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

void bellCoTRead(String KEY, String TOKEN,int roomIndex) {  // Reads values from CoT (bell) and stores values into given variables 
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

    bellValueBefore = (bellRegister.substring(0, roomIndex+1));    // Stores all values before "bellValue"
    bellValue = (bellRegister.substring(roomIndex+1, roomIndex+2).toInt());    // Stores current mode for the bell
    bellValueAfter = (bellRegister.substring(roomIndex+2, 7));    // Stores all values after "bellValue"

    http.end(); // Ends http
  }
}

void cardRead(){

  // Look for new RFID cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return ;
  }
  // Select one of the RFID cards
  if (! mfrc522.PICC_ReadCardSerial()){
    return ;
  }
  String cardID = "";
  
  buzzerTone(100); // play beep tone for 100ms on buzzer when scanning card

  //read UID from card
  for (byte i = 0; i < mfrc522.uid.size; i++){
    cardID.concat(String(mfrc522.uid.uidByte[i] <0x10 ? "0" : " "));
    cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  cardID.toUpperCase();
  
  //Serial.println(cardID);

  
  
  //check if cardID matches keyCards
  for (int i = 0; i < numberOfCards; i++){
    if(cardID.substring(1) == keyCardUID[i]) {
      
      digitalWrite(greenLed,HIGH); // Turn on green LED 
      
      
      bellCoTRead(bellKEY,TOKEN,i); //read bellergister from CoT
      
      
      bellValue = 4; // sets bellValue to 4 to register that user is home. 

      bellRegister = (bellValueBefore + bellValue + bellValueAfter); // Updates the register string with the new bellValue
      //Serial.println(bellRegister);
      
      CoTWrite(bellKEY,TOKEN,bellRegister); // sends updated register to CoT

      digitalWrite(greenLed,LOW); //Turn off green LED
      return;
    }
    
  }
  //lights red led if RFID card is not registered and plays a series of short beeps
  digitalWrite(redLed,HIGH); 
  buzzerTone(100);
  delay(100);
  buzzerTone(100);
  delay(100);
  buzzerTone(100);
  delay(100);
  buzzerTone(100);  
  delay(1000);
  digitalWrite(redLed,LOW);
}

void doorBell(){
  unsigned long currentMillis;
  unsigned long prev1Millis; 
  unsigned long prev2Millis; 
  bool ledState;

  
  for (int pin = 0; pin < pinCount; pin++){ 
    
    
    if (digitalRead(buttonPins[pin])==LOW){ //checks if button in current iteration is pressed 
      
      //turn on white LED
      ledState = HIGH;
      digitalWrite(whiteLed,ledState);
      
      //Serial.println(pin + 1);

      bellCoTRead(bellKEY,TOKEN,pin);
      Serial.println(bellRegister);
      
      bellValue = 1; //sets bell Value to 1 

      bellRegister = (bellValueBefore + bellValue + bellValueAfter); // Updates the register string with the new bellValue
       
      CoTWrite(bellKEY,TOKEN,bellRegister); // sends updated register to CoT
      currentMillis = millis();
      prev1Millis = currentMillis;
      prev2Millis = currentMillis;

      
      ledState = !ledState;
      digitalWrite(whiteLed,ledState);
      
      //
      while(currentMillis - prev1Millis < bellTimeout){
        currentMillis = millis();

        //blinks white LED while waiting for response
        if(currentMillis - prev2Millis > 500){
          ledState = !ledState;
          digitalWrite(whiteLed,ledState);
          prev2Millis = currentMillis;
          Serial.println(currentMillis - prev2Millis);
        }

        bellCoTRead(bellKEY,TOKEN,pin); //reads data from CoT
        
        // Turns on green LED and plays buzzer tone if open door signal is received 
        if(bellValue == 2){
          digitalWrite(whiteLed,LOW);
          digitalWrite(greenLed,HIGH);
          buzzerTone(500);
          delay(1000);
          digitalWrite(greenLed,LOW);
          break;
        }
        // Turns on red LED and plays a series of beeps if reject signal is received 
        if(bellValue == 3){
          digitalWrite(whiteLed,LOW);
          digitalWrite(redLed,HIGH); 
          buzzerTone(100);
          delay(100);
          buzzerTone(100);
          delay(100);
          buzzerTone(100);
          delay(100);
          buzzerTone(100);  
          delay(1000);
          digitalWrite(redLed,LOW);
          break;
        }
        
      }
     digitalWrite(whiteLed,LOW);

     
    }
  }
  
}
void buzzerTone(int duration){
  ledcWriteTone(channel, 500); //send signal to buzzer
    delay(duration);
    ledcWriteTone(channel, 0); //stop signal to buzzer
}

void sleep(){
  unsigned long currentMillis = millis();
  
  //reset sleepTimer if lightsensor is triggered
  if (digitalRead(sensor)==LOW){
    sleepTimer = currentMillis;
  }
  //disconnects wifi and enters deep sleep mode after 2 minutes
  if (currentMillis - sleepTimer > sleepTimeout){
    
    //Serial.println("sleepmode");
    WiFi.disconnect();
    esp_deep_sleep_start();
    }
  
}

void setup() {
  Serial.begin(9600);
  SPI.begin();          // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  
  //Set pinmode Led pins
  pinMode(greenLed,OUTPUT);
  pinMode(whiteLed,OUTPUT);
  pinMode(redLed,OUTPUT);

  // Set pinmode button pins
  for (int pin = 0; pin < pinCount; pin++){
    pinMode(buttonPins[pin],INPUT_PULLUP);  
  }
  // set pinMode and initialize buzzer
  pinMode(buzzer, OUTPUT);
  ledcSetup(channel, freq, res);
  ledcAttachPin(buzzer, channel);

  pinMode(sensor,INPUT);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_34,0); //enable sleep wakeup on IO34

  //power on all leds when initializing wifi
  digitalWrite(whiteLed,HIGH);
  digitalWrite(redLed,HIGH);
  digitalWrite(greenLed,HIGH);
  
  getWiFi(); // starts wifi

  //power off leds after wifi initialization
  digitalWrite(redLed,LOW);
  digitalWrite(greenLed,LOW);
  digitalWrite(whiteLed,LOW);
  
  sleepTimer = millis(); //start sleeptimer 


}

void loop() {
cardRead();

doorBell();

sleep();

//Serial.println(digitalRead(34));

}

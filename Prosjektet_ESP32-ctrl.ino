// Importering av bibloteker
#include <WiFi.h>            // Importering av bibliotek for WiFi
#include <HTTPClient.h>      // Importering av bibliotek for HTTP
#include <ArduinoJson.h>     // Importering av bibliotek for Json

#include <CircusESP32Lib.h>  // Importering av bibliotek for ESP32
#include <ESP32Servo.h>      // Importering av bibliotek for servomotor
#include <analogWrite.h>     // Importering av bibliotek for analogWrite
#include <Adafruit_Sensor.h> // Importering av bibliotek for temperatursensor
#include <Adafruit_BME280.h> // Importering av bibliotek for temperatursensor
#include <Wire.h>            // Importering av bibliotek for I2C-kommunikasjon med temperatursensor

// Verdier av temperaturgrenser
#define upper_limit_temp 28 // Setter øvre temperaturgrense (styring takvifte)
#define lower_limit_temp 22 // Setter nedre temperaturgrense (styring takvifte)

// Setup av WIFI og kommunikasjon til CoT via ESP32
char ssid[] = "AJ";
char password[] = "andreasj";
char server[] = "www.circusofthings.com";

// Token
char token[] = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0";

// Keys
char room_function_key[] = "10893";       // CoT-signal for lysforhold, inne- og utetemperatur
char room_setting_key[] = "7284";         // CoT-signal for ESP32-ctrl
char smartmetertemp_value_key[] = "3183"; // CoT-signal for utetemperatur

// Definerer biblotekene
CircusESP32Lib circusESP32(server, ssid, password);
Servo servo;
Adafruit_BME280 bme280;

// Pins
int servo_pin = 17;
int vifte_IN1_pin = 12;
int vifte_IN2_pin = 13;
int vifte_EN1_pin = 14;
int LED_pin = 25;
int photo_pin = 35;

// Verdier LED
const int off_LED = 0;   // Ingen lys fra LED
const int min_LED = 170; // Minste verdi LED skal lyse
const int mid_LED = 210; // Mellomste verdi LED skal lyse
const int max_LED = 255; // Høyeste verdi LED skal lyse

// Verdier photoresistor
const int min_photo_value = 2800; // Minste verdi til photoresistor (map)
const int max_photo_value = 4095; // Høyeste verdi til photoresistor (map)

// Verdier vifte
const int off_hastighet_vifte = 0; 
const int mid_hastighet_vifte = 75;
const int max_hastighet_vifte = 150;

// Verdier servo
const int off_servo = 0;  // Servoen rotert 0 grader
const int mid_servo = 40; // Servoen rotert 40 grader
const int max_servo = 80; // Servoen rotert 80 grader

// ledcSetup
int vifte_channel = 5;
int vifte_freq = 250;
int vifte_res = 8;

// Variabler for funksjonene
int roomtemp_value;
int outtemp_value;
int vinkel_vindu;
int hastighet_vifte;
int LED_konsoll;
int termostat_value;
int photo_value;
int LED_invers_photo;
int smartmetertemp_value;

String setting_signal;
String function_signal;
String oppdatert_function_signal;
String forrige_LED_signal;
String neste_LED_signal;
String forrige_temp_signal;
String neste_temp_signal;
String rom_nr;
String rom_status;
String oppdatert_smartmetertemp_value;

// http
int http_response_code_write;
int http_response_code_read;

void CoTWrite(String KEY, String TOKEN, String value) { // Sender en gitt verdi (value) til CoT basert på gitt key og token
  if (WiFi.status() == WL_CONNECTED){                   // Hvis det er kontakt med WiFi
    HTTPClient http;
    String server_path = "https://circusofthings.com/WriteValue?Key="+String(KEY)+"&Value="+String(value)+"&Token="+String(TOKEN);
    //Setter sammen key, token og value til en http-serverbane
    Serial.println("Server path: " + server_path);
    http.begin(server_path.c_str()); // Starter http med en gitt serverbane
    
    http_response_code_write = http.GET(); // Får en tallkode som sier om gjennomføringen er godkjent eller feil (Kode 200 betyr at den er godkjent) 
    Serial.println("http response: " + String(http_response_code_write));

    http.end();
  }
}

void roomCoTRead(String KEY, String TOKEN) {       // Leser verdi fra CoT og lagrer i gitte variabler
  const size_t buffer_size = JSON_OBJECT_SIZE(13); // Definerer buffer_size for json-objekt
  DynamicJsonBuffer jsonBuffer(buffer_size);       // Setter opp Dynamic json Buffer fra bufferen på linjen over

  if (WiFi.status() == WL_CONNECTED){              // Sjekker om det er kontakt med WiFi
    HTTPClient http;

    String server_path = "https://circusofthings.com/ReadValue?Key="+String(KEY)+"&%22&Token="+String(TOKEN);  
    //Setter sammen key og token til en http-serverbane

    http.begin(server_path.c_str()); // Starter http med en gitt serverbane

    http_response_code_read = http.GET();  // Får en tallkode som sier om gjennomføringen er godkjent eller feil

    String payload = http.getString();  // Lagrer payload fra http som en string

    JsonObject& root = jsonBuffer.parseObject(payload); // Bruker Json til å dekode payload
    double http_response = root["Value"]; // Lagrer verdien (value) fra payload i http_response

    String roomSetting = String(http_response); // Definerer roomSetting som en string av http_response

    neste_LED_signal = (roomSetting.substring(7, 10).toInt()); // Definerer neste_LED_signal ved å dele opp roomSetting og lagrer denne
    roomtemp_value = (roomSetting.substring(1, 4).toInt());    // Definerer roomtemp_value ved å dele opp roomSetting og lagrer denne

    http.end();
  }
}

void getWiFi() {  // Setter opp WiFi
  WiFi.begin(ssid, password); // Starter WiFi
  Serial.println("Connecting"); // Printer "Connecting"
  while (WiFi.status() != WL_CONNECTED) {  // Når det ikke er kontakt med WiFi

    delay(100);

    Serial.print(".");  // Printer "."

    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {  // Hvis det er kontakt med WiFi
    Serial.println("Success");  // Printer "Success"
  }
}

void room_setting_register_read_signal() {
  setting_signal = String(circusESP32.read(room_setting_key, token)); // Henter ut registeret til signalene fra CoT

  termostat_value = (setting_signal.substring(1, 3).toInt()); // Deler opp signalet. Tar ut verdi på termostat_value
  Serial.println("termostat_value. Signal fra konsollet: " + String(termostat_value));

  hastighet_vifte = (setting_signal.substring(3, 4).toInt()); // Deler opp signalet. Tar ut verdi for hastigheten på takviften
  Serial.println("Fart paa takvifte. Signal fra konsollet: " + String(hastighet_vifte));

  vinkel_vindu = (setting_signal.substring(4, 5).toInt()); // Deler opp signalet. Tar ut verdi for vinkelen på vinduet
  Serial.println("Vinkel paa vinduet. Signal fra konsollet: " + String(vinkel_vindu));

  LED_konsoll = (setting_signal.substring(5, 8).toInt()); // Deler opp signalet. Tar ut verdi for lysstyrke satt på konsollen
  Serial.println("Led. Signal fra konsollet: " + String(LED_konsoll));
}

void room_function_register_read_signal() {
  function_signal = String(circusESP32.read(room_function_key, token)); // Henter ut registeret til signalene fra CoT

  smartmetertemp_value = circusESP32.read(smartmetertemp_value_key, token); // Henter ut temperatursignalet fra smartmeteret fra CoT
  oppdatert_smartmetertemp_value = smartmetertemp_value + 100; // Legger til 100 på temperatursignalet fra smartmeteret for å få riktig struktur
  if (smartmetertemp_value < 0) {
    oppdatert_smartmetertemp_value = String("0") + oppdatert_smartmetertemp_value; // Legger til en null foran hvis det er minusgrader ute
  }

  outtemp_value = (function_signal.substring(4, 7).toInt()) - 100; // (Signalet - 100) for å få riktig verdi på signalet som skal sendes til CoT

  forrige_LED_signal = (function_signal.substring(7, 10).toInt()); // Deler opp signalet. Tar ut verdi for det forrige lyssignalet

  forrige_temp_signal = (function_signal.substring(1, 4).toInt()); // Deler opp signalet. Tar ut verdi for den forrige temperaturen

  rom_nr = (function_signal.substring(0, 1).toInt()); // Deler opp signalet. Tar ut verdi for romnummer

  rom_status = (function_signal.substring(10, 11).toInt()); // Deler opp signalet. Tar ut verdi for romstatus
}

// Funksjon for simulert lysmengde utenfra
void photoresistor() {
  photo_value = analogRead(photo_pin);
  Serial.println("Verdi photoresistor: " + String(photo_value)); // Verdiene photoresistoren registrerer 
  
  LED_invers_photo = map(photo_value, min_photo_value, max_photo_value, max_LED, min_LED); // Inverterer verdiene fra photoresistoren
  
  if (LED_invers_photo < 100) {
    neste_LED_signal = String("0") + String(LED_invers_photo); // Er den mindre enn 100 må det legges til en 0 foran for å opprettholde signalstrukturen
  }
  else {
    neste_LED_signal = String(LED_invers_photo);
  }
}

// Funksjon for kalibrering av lys utenfra opp mot lys inne
void lys_soverom() {
  if (LED_konsoll == 256) { // Verdien 256 initialiserer automatisk modus
    room_function_register_read_signal();
    analogWrite(LED_pin, LED_invers_photo);
  }
  else {
    analogWrite(LED_pin, LED_konsoll);
  }
}

// Funksjon for lesing av temperatur til CoT
void temp() {
  roomtemp_value = bme280.readTemperature(); // Lagrer temperaturen fra temperaturmåleren

  if (roomtemp_value < 10) {
    neste_temp_signal = String("00") + String(roomtemp_value); // Legger til to nuller hvis temperaturen inne er mindre enn 10 for å opprettholde signalstrukturen
  }
  if (roomtemp_value > 10) { 
    neste_temp_signal = String("0") + String(roomtemp_value); // Legger til en null hvis temperaturen inne er mer enn 10 for å opprettholde signalstrukturen
  }

  oppdatert_function_signal = rom_nr + neste_temp_signal + oppdatert_smartmetertemp_value + neste_LED_signal + rom_status; // Det nye registeret som sendes til CoT
}

// Funksjon for styring av takvifte
void takvifte() {
  digitalWrite(vifte_IN1_pin, LOW);
  digitalWrite(vifte_IN2_pin, HIGH); // Vifta roterer mot klokka

  //Verdien 1 initialiserer automatisk modus
  if (hastighet_vifte == 1) {
    if (roomtemp_value >= upper_limit_temp) { // Hvis romtemp er større enn upper_limit_temp, kjør vifte på maks fart
      ledcWrite(vifte_channel, max_hastighet_vifte);
    }
    else if (roomtemp_value >= lower_limit_temp && roomtemp_value < upper_limit_temp) { // Hvis romtemp er større enn upper_limit_temp, og samtidig mindre enn lower_limit_temp kjør vifte på medium fart
      ledcWrite(vifte_channel, mid_hastighet_vifte);
    }
    else {
      ledcWrite(vifte_channel, off_hastighet_vifte); // Hvis romtemp er under lower_limit_temp, ikke kjør vifte
    }
  }

  //Verdien != 1 initialiserer manuell modus
  if (hastighet_vifte != 1) {
    if (hastighet_vifte == 0) { // 0: Viften er avslått
      ledcWrite(vifte_channel, off_hastighet_vifte);
    }
    else if (hastighet_vifte == 2) { // 2: Viften går på medium hastighet
      ledcWrite(vifte_channel, mid_hastighet_vifte);
    }
    else if (hastighet_vifte == 3) { // 3: Viften går på maks hastighet
      ledcWrite(vifte_channel, max_hastighet_vifte);
    }
  }
}

// Funksjon for styring av luftevindu
void luftevindu() {
  // Verdien 1 initialiserer automatisk modus
  if (vinkel_vindu == 1) {
    if (outtemp_value >= roomtemp_value) { // Hvis det er varmere ute enn inne, skal vinduet være lukket
      servo.write(off_servo);
    }
    else if (outtemp_value < roomtemp_value) { // Hvis det er kaldere ute enn inne, skal vinduet være åpent
      servo.write(max_servo);
    }
    else if (roomtemp_value < lower_limit_temp) { // Hvis det er kladere ute enn nedre grense satt så skal vinduet være lukket
      servo.write(off_servo);
    }
  }

  // Verdien != 1 initialiserer manuell modus
  if (vinkel_vindu != 1) {
    if (vinkel_vindu == 0) { // 0: Viduet er lukket
      servo.write(off_servo);
    }
    else if (vinkel_vindu == 2) { // 2: Vinduet er halvveis åpent (40 grader)
      servo.write(mid_servo);
    }
    else if (vinkel_vindu == 3) { // 3: Vinduet er helt åpent (80 grader)
      servo.write(max_servo);
    }
  }
}

void setup() {
  Serial.begin(115200);               // Baud-rate 115200
  circusESP32.begin();                // Starter kommunikajson opp mot CoT
  bme280.begin(0x76);                 // Kommunikasjon for temperatursesnor
  servo.attach(servo_pin, 544, 2400); // "Fester" servo_pin, 544 = min. pulsbredde i mikrosekunder ved 0 grader, 2400 = max. pulsbredde i mikrosekunder ved 180 grader.

  pinMode(vifte_IN1_pin, OUTPUT); // Pinmode for DC-motor
  pinMode(vifte_IN2_pin, OUTPUT); // Pinmode for DC-motor
  pinMode(LED_pin, OUTPUT);       // Pinmode for LED
  pinMode(photo_pin, INPUT);      // Pinmode for Photoresistor

  ledcSetup(vifte_channel, vifte_freq, vifte_res);
  ledcAttachPin(vifte_EN1_pin, vifte_channel);

  getWiFi(); // Kaller på funksjonen som setter opp WiFi-kommunikasjonen
}

void loop() { 
  room_setting_register_read_signal();                           // Kaller på funksjonen som leser registersignalet fra CoT
  room_function_register_read_signal();                          // Kaller på funksjonen som leser registersignalet fra CoT
  photoresistor();                                               // Kaller på funksjonen som omhandler photoresistoren 
  temp();                                                        // Kaller på funksjonen som omhandler soveromstemperaturen  
  lys_soverom();                                                 // Kaller på funksjonen som omhandler lys innendørs basert på lys utendørs 
  takvifte();                                                    // Kaller på funksjonen som omhandler styring av takvifte
  luftevindu();                                                  // Kaller på funksjonen som omhandler styring av luftevinduet
  CoTWrite(room_function_key, token, oppdatert_function_signal); // Sender et oppdatert register tilbake til CoT
}
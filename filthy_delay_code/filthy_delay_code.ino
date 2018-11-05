#include <DHTesp.h>
#include <Ticker.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino -- need to look at
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecureAxTLS.cpp

// Static Json Object Encoder/Decoder
StaticJsonDocument<300> json;

Ticker ticker;
Ticker temperatureHumidityTimer;
Ticker motionTimer;
Ticker serviceStatusTimer;
Ticker debugTimer;

// Consts
const char* ssid     = "SSID";
const char* password = "SUPERSECRETPASSWORD";
const String host = "https://inft3970.azurewebsites.net";
const char* Id = "1";
const String fingerprint = "3A B0 B1 C2 7F 74 6F D9 0C 34 F0 D6 A9 60 CF 73 A4 22 9D E8";
//const uint8_t fingerprint[20] = {0x3A, 0xB0, 0xB1, 0xC2, 0x7F, 0x74, 0x6F, 0xD9, 0x0C, 0x34, 0xF0, 0xD6, 0xA9, 0x60, 0xCF, 0x73, 0xA4, 0x22, 0x9D, 0xE8};
//const uint8_t testing[20] = {0xBF, 0xF1, 0xB9, 0x95, 0x52, 0xAC, 0x69, 0xE5, 0x44, 0xA1, 0x42, 0x03, 0x31, 0xD3, 0xA0, 0xEF, 0x49, 0x44, 0xF9, 0xAB};

#define LED 2  //On board LED
#define MOTION 13 //Pin Motion sensor is associated with
#define TH 5 //Pin Motion sensor is associated with

bool motionOccured = false;
bool serviceAvailable = false;

// DHTesp is a library imported from; https://github.com/beegee-tokyo/DHTesp
// A fair bit of the below code is reused from: https://github.com/beegee-tokyo/DHTesp/tree/master/examples/DHT_ESP8266 
DHTesp dht;

HTTPClient https;

void setup()
{
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(MOTION, INPUT);
  pinMode(MOTION, INPUT);

  Serial.begin(115200);
  delay(50);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // 5 is the I/O output as dictated; https://www.c-sharpcorner.com/article/blinking-led-by-esp-12e-nodemcu-v3-module-using-arduinoide/
  dht.setup(TH, DHTesp::DHT11);
}

void checkServiceStatus()
{
  Serial.println("#############################################");
  String endpoint = "https://inft3970.azurewebsites.net/api/Availability";
  https.begin(endpoint,fingerprint);
  int httpCode = https.GET(); //Send the request
  String payload = https.getString(); //Get the response payload
  Serial.println("HTTP Code received: " + String(httpCode)); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  https.end(); //Close connection
  Serial.println("#############################################");
  if(httpCode = 200){
    serviceAvailable = true;
    return;
  }
  serviceAvailable = false;
  return;
}

void debug(){

  double humidity = dht.getHumidity();
  double temperature = dht.getTemperature();
  bool motion = getMotion();
  Serial.println(humidity, 1);
  Serial.println(temperature, 1);

  Serial.println("--------------------------------------------------");
  Serial.println("API status available: " + String(serviceAvailable));
  Serial.println("--------------------------------------------------");

  Serial.println("Motion Detected: " + String(motion));
  Serial.println("--------------------------------------------------");

  Serial.println("Temperature: " + String(temperature,2));
  String temperaturePayload = createJsonPayload("temperature",temperature);
  Serial.println("Temperature payload: " + temperaturePayload);
  Serial.println("--------------------------------------------------");
  
  Serial.println("Humidity: " + String(humidity,2));
  String humidityPayload = createJsonPayload("humidity",humidity);
  Serial.println("Humidity payload: " + humidityPayload);
}

String createJsonPayload(String measure,double value){
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject[measure] = value;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);
  Serial.println(jsonPayload);
  return jsonPayload;
}

void postPayload(String type, String jsonPayload){
  String endpoint = String(host + "/api/" + type + "/Create");
  Serial.println("Reached out to: " + endpoint);

  https.begin(endpoint,fingerprint);
  https.addHeader("Content-Type", "application/json");

  int httpCode = https.POST(jsonPayload); //Send the request
  String payload = https.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  https.end(); //Close connection
}

void changeState(){
  digitalWrite(LED,!(digitalRead(LED)));
}

bool getMotion(){
  int result = digitalRead(MOTION);
  if(result == 0){
    return false;
  }
  return true;
}

void timedFlashLED(){
  for(int i = 1; i <= 3; i++){
    delay(300);
    digitalWrite(LED,!(digitalRead(LED)));
  }
}

void flashLED(int iterations, int msBetweenFlash){
  for(int i = 1; i <= iterations; i++){
    changeState();
    delay(msBetweenFlash/2);
    changeState();
    delay(msBetweenFlash/2);
  }
}

void temperatureAndHumidityExecute(){
  String temperaturePayload = createJsonPayload("temperature",dht.getTemperature());
  postPayload("temperature",temperaturePayload);
  Serial.println("Temperature Posted");

  String humidityPayload = createJsonPayload("humidity",dht.getHumidity());
  postPayload("humidity",humidityPayload);
  Serial.println("Humidity Posted");
}

bool motionExecute(){
  if(motionOccured){
    String motionPayload = createJsonPayload("motion",1);
    postPayload("motion",motionPayload);
    Serial.println("Motion Posted");
    return true;
  }
  motionOccured = getMotion();
  return false;
}

void loop()
{
  int counter = 0;
  int motionCounter = 0;
  bool resetMotionCount = false;
  checkServiceStatus();
  while(serviceAvailable){
    switch (counter) {
      case 60: 
        temperatureAndHumidityExecute(); 
        counter = 0;
        break;
      default:
        if(counter % 6 == 0){
          if(motionCounter > 5){
            resetMotionCount = motionExecute(); 
          }
        }
        if(counter % 3 == 0){
          checkServiceStatus();
        }
        debug();
        delay(5000);        
        break;
    }

    if(resetMotionCount){
      motionCounter = 0;
      resetMotionCount = false;
    }
    
    motionCounter += 1;
    counter += 1;
  }
}

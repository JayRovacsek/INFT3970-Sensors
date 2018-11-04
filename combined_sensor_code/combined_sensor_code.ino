#include <DHTesp.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>

// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino -- need to look at
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecureAxTLS.cpp



// DHTesp is a library imported from; https://github.com/beegee-tokyo/DHTesp
// A fair bit of the below code is reused from: https://github.com/beegee-tokyo/DHTesp/tree/master/examples/DHT_ESP8266 
DHTesp dht;

// WifiClient required for POST requests to hit the API
ESP8266WiFiMulti WiFiMulti;


// Static Json Object Encoder/Decoder
StaticJsonDocument<300> json;

Ticker ticker;
Ticker temperatureHumidityTimer;
Ticker motionTimer;
Ticker serviceStatusTimer;
Ticker debugTimer;

HTTPClient https;

// Consts
const char* ssid     = "SSID";
const char* password = "SUPERSECRETPASSWORD";
const String host = "inft3970.azurewebsites.net";
const char* Id = "1";
const uint8_t fingerprint[20] = {0x3A, 0xB0, 0xB1, 0xC2, 0x7F, 0x74, 0x6F, 0xD9, 0x0C, 0x34, 0xF0, 0xD6, 0xA9, 0x60, 0xCF, 0x73, 0xA4, 0x22, 0x9D, 0xE8};

#define LED 2  //On board LED
#define TEMPHUMID 5
#define MOTION 13 //Pin Motion sensor is associated with

bool DHTTimerSet = false;
bool MotionTimerSet = false;
bool ServiceTimerSet = false;
bool serviceAvailable = false;

void setup()
{
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(MOTION, INPUT);
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);


  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

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
  dht.setup(TEMPHUMID, DHTesp::DHT11);
  
  temperatureHumidityTimer.attach(300,dhtTimerExecute);
  motionTimer.attach(30,motionTimerExecute);
  serviceStatusTimer.attach(15,checkServiceStatus);
  debugTimer.attach(5,debug);
}

void checkServiceStatus()
{
  String endpoint = String("http://" + host + ":/api/Availability");
  https.begin(endpoint, fingerprint);
  int httpCode = https.GET(); //Send the request
  String payload = https.getString(); //Get the response payload
  Serial.println(endpoint); //Print HTTP return code
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  https.end(); //Close connection
  if(httpCode = 200){
    serviceAvailable = true;
    return;
  }
  serviceAvailable = false;
  return;
}

void debug(){
  Serial.println("--------------------------------------------------");
  Serial.println("API status available: " + String(serviceAvailable));
  Serial.println("--------------------------------------------------");

  bool motion = getMotion();
  Serial.println("Motion Detected: " + String(motion));
  Serial.println("--------------------------------------------------");

  double temperature = dht.getTemperature();
  Serial.println("Temperature: " + String(temperature,2));
  String temperaturePayload = createJsonPayload("temperature",temperature);
  Serial.println("Temperature payload: " + temperaturePayload);
  Serial.println("--------------------------------------------------");
  
  double humidity = dht.getHumidity();
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
  String endpoint = String("https://" + host + ":/api/" + type + "/Create");
  Serial.println("Reached out to: " + endpoint);

  //https.begin(endpoint, fingerprint);
  //https.addHeader("Content-Type", "application/json");

  //int httpCode = https.POST(jsonPayload); //Send the request
  //String payload = https.getString(); //Get the response payload
  //Serial.println(httpCode); //Print HTTP return code
  //Serial.println(payload); //Print request response payload
  //https.end(); //Close connection
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

void dhtTimerExecute(){
  if(serviceAvailable){
    String temperaturePayload = createJsonPayload("temperature",dht.getTemperature());
    postPayload("temperature",temperaturePayload);
    Serial.println("temperature timer fired");
  
    String humidityPayload = createJsonPayload("humidity",dht.getHumidity());
    postPayload("humidity",humidityPayload);
    Serial.println("humidity timer fired");
  }
}

void motionTimerExecute(){
  if(serviceAvailable){
    bool motion = getMotion();
    if(motion){
      String motionPayload = createJsonPayload("motion",1);
      postPayload("motion",motionPayload);
      Serial.println("motion timer fired");
      MotionTimerSet = false;
    }
  }
}

void loop()
{
}

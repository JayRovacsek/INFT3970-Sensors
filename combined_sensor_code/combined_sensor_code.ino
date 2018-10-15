#include <DHTesp.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// DHTesp is a library imported from; https://github.com/beegee-tokyo/DHTesp
// A fair bit of the below code is reused from: https://github.com/beegee-tokyo/DHTesp/tree/master/examples/DHT_ESP8266 
DHTesp dht;

// Httpclient required for POST requests to hit the API
HTTPClient http;

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
const String host = "inft3970.azurewebsites.net";
const char* Id = "1";
const char* ntp = "pool.ntp.org";

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
  String endpoint = String("http://" + host + ":80/api/Availability");
  http.begin(endpoint);
  int httpCode = http.GET(); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(endpoint); //Print HTTP return code
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
  if(httpCode = 200){
    serviceAvailable = true;
    return;
  }
  serviceAvailable = false;
  return;
}

void debug(){
  bool motion = getMotion();
  Serial.println("Motion Detected: " + String(motion));
  Serial.println("API status available: " + String(serviceAvailable));

  Serial.println(dht.getTemperature());
  
  String temperaturePayload = createJsonPayload("temperature",dht.getTemperature());
  Serial.println("Temperature payload: " + temperaturePayload);

  Serial.println(dht.getHumidity());

  String humidityPayload = createJsonPayload("humidity",dht.getHumidity());
  Serial.println("Humidity payload: " + humidityPayload);
}

String createJsonPayload(String measure,float value){
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject[measure] = value;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);
  Serial.println(jsonPayload);
  return jsonPayload;
}

void postPayload(String type, String jsonPayload){
  String endpoint = String("http://" + host + ":80/api/" + type + "/Create");
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(jsonPayload); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
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


#include <DHTesp.h>
#include <Ticker.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define LED 2
#define MOTION 13
#define TH 5 

StaticJsonDocument<300> json;

// Consts
const char* ssid     = "SSID";
const char* password = "SUPERSECRETPASSWORD";
const String host = "https://inft3970.azurewebsites.net";
const char* Id = "1";
const String fingerprint = "3A B0 B1 C2 7F 74 6F D9 0C 34 F0 D6 A9 60 CF 73 A4 22 9D E8";

bool motionOccured = false;
bool serviceAvailable = false;

// DHTesp is a library imported from; https://github.com/beegee-tokyo/DHTesp
// A fair bit of the below code is reused from: https://github.com/beegee-tokyo/DHTesp/tree/master/examples/DHT_ESP8266 
DHTesp dht;

HTTPClient https;

void setup()
{
  pinMode(LED, OUTPUT);
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

  dht.setup(TH, DHTesp::DHT11);
}

void checkServiceStatus()
{
  String endpoint = host + "/api/Availability";
  https.begin(endpoint,fingerprint);
  int httpCode = https.GET();
  String payload = https.getString();
  Serial.println("HTTP Code received: " + String(httpCode));
  https.end();
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

  int httpCode = https.POST(jsonPayload);
  String payload = https.getString();
  Serial.println(httpCode);
  Serial.println(payload);
  https.end();
}

bool getMotion(){
  bool result = digitalRead(MOTION);
  Serial.println(result);
  if(result){
      digitalWrite(LED,LOW);
  }
  else{
    digitalWrite(LED,HIGH);
  }
  return result;
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

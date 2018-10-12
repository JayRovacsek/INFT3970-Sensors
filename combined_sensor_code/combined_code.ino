#include <DHTesp.h>
#include <stdlib.h>
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
Ticker temperatureTimer;
Ticker humidityTimer;
Ticker motionTimer;

struct KeyValuePair{
  String key;
  String value;
};

// Consts
const char* ssid     = "SSID";
const char* password = "SUPERSECRETPASSWORD";
const String host = "inft3970.com";
const char* Id = "1";
const char* ntp = "pool.ntp.org";

#define LED 2  //On board LED
#define Motion 12 //Pin Motion sensor is associated with

void setup()
{
  pinMode(2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
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
  
  // 5 is the I/O output as dictated; https://iotbytes.wordpress.com/nodemcu-pinout/ 
  dht.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
}

bool checkServiceStatus()
{
  String endpoint = String("http://" + host + ":80/api/Availability");
  http.begin(endpoint);
  int httpCode = http.GET(); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
  if(httpCode = 200){
    return true;
  }
  return false;
}

String createJsonPayload(KeyValuePair values){
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject[values.key] = values.value;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);
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

void ChangeState(){
  digitalWrite(LED,!(digitalRead(LED)));
}

bool getMotion(){
  int result = digitalRead(Motion);
  if(result == 0){
    return false;
  }
  return true;
}

void flashLED(int iterations, int msBetweenFlash){
  for(int i = 1; i <= iterations; i++){
    ticker.attach_ms(msBetweenFlash,ChangeState);
  }
}

void loop()
{
  digitalWrite(LED, HIGH);
  if (WiFi.status() == WL_CONNECTED){
    bool serviceAvailable = checkServiceStatus();
    while(serviceAvailable)
    {
      flashLED(5,400);
      KeyValuePair temperatureKeyValuePair = {"temperature",String(dht.getTemperature(),2)};
      String temperaturePayload = createJsonPayload(temperatureKeyValuePair);
      temperatureTimer.attach(300,postPayload("temperature",temperaturePayload));
      Serial.println("temperature timer started");

      KeyValuePair humidityKeyValuePair = {"humidity",String(dht.getHumidity(),2)};
      String humidityPayload = createJsonPayload(humidityKeyValuePair);
      humidityTimer.attach(300,postPayload("humidity",humidityPayload));
      Serial.println("humidity timer started");

      bool motion = getMotion();
      if(motion){
        KeyValuePair motionKeyValuePair = {"motion","1"};
        String motionPayload = createJsonPayload(motionKeyValuePair);
        motionTimer.attach(30,postPayload("motion",motionPayload));
        Serial.println("motion timer started");
      }

      serviceAvailable = checkServiceStatus();
    }
  }
}
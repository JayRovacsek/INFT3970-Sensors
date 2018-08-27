#include <DHTesp.h>
#include <stdlib.h>
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

// Consts
const char* ssid     = "SSID";
const char* password = "SUPERSECRETPASSWORD";
const String host = "inft3970.com";
const char* Id = "1";

// Required for string concat
char *strcat(char *dest, const char *src);

void setup()
{
  pinMode(2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  // Set serial baud rate at 115200
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
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

int check_service_status()
{
  String endpoint = String("http://" + host + ":80/api/Availability");
  http.begin(endpoint);
  int httpCode = http.GET(); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
  return httpCode;
}

void post_temperature(double temperature)
{
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject["Temperature"] = temperature;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);

  String endpoint = String("http://" + host + ":80/api/Temperature/Create");
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonPayload); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
}

void post_humidity(double humidity)
{
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject["Humidity"] = humidity;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);

  String endpoint = String("http://" + host + ":80/api/Humidity/Create");
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonPayload); //Send the request
  String payload = http.getString(); //Get the response payload
  Serial.println(httpCode); //Print HTTP return code
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
}

void loop()
{
  digitalWrite(2, HIGH);
  if (WiFi.status() == WL_CONNECTED){
    int availability = check_service_status();
    while(availability == 200)
    {
      digitalWrite(2, LOW);
      delay(500);
      digitalWrite(2, HIGH);
      delay(500);
      digitalWrite(2, LOW);
      double humidity = dht.getHumidity();
      double temperature = dht.getTemperature();
      post_temperature(temperature);
      post_humidity(humidity);
      digitalWrite(2, HIGH);
      // Delay 29 Seconds
      delay(29000);
      availability = check_service_status();
    }
  }
}

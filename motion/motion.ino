#include <Wire.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Httpclient required for POST requests to hit the API
HTTPClient http;

// Static Json Object Encoder/Decoder
StaticJsonDocument<300> json;

// Consts
const char* ssid     = "Ooo Ooo Net";
const char* password = "itiswednesdaymydudes";
const String host = "inft3970.azurewebsites.net";
const char* Id = "9";

// Required for string concat
char *strcat(char *dest, const char *src);

void setup()
{
  pinMode(12,INPUT);
  pinMode(2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  // Set serial baud rate at 115200
  Serial.begin(115200);
  delay(10);

  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
}

int check_service_status()
{
  String endpoint = String("http://" + host + ":80/api/Availability");
  http.begin(endpoint);
  int httpCode = http.GET(); //Send the request
  String payload = http.getString(); //Get the response payload
  //Serial.println(httpCode); //Print HTTP return code
  //Serial.println(payload); //Print request response payload
  http.end(); //Close connection
  return httpCode;
}

void post_motion(bool motion)
{
  JsonObject jsonObject = json.to<JsonObject>();
  jsonObject["Id"] = Id;
  jsonObject["Motion"] = motion;
  
  String jsonPayload;
  serializeJson(jsonObject, jsonPayload);

  String endpoint = String("http://" + host + ":80/api/Motion/Create");
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonPayload); //Send the request
  String payload = http.getString(); //Get the response payload
  //Serial.println(httpCode); //Print HTTP return code
  //Serial.println(payload); //Print request response payload
  http.end(); //Close connection
}

void loop()
{
  bool pir;
  digitalWrite(2, HIGH);
  if (WiFi.status() == WL_CONNECTED){
    digitalWrite(2, LOW);
    delay(500);
    digitalWrite(2, HIGH);
    delay(500);
    pir=digitalRead(12);
    Serial.println("Results :"); 
    Serial.println(pir); 
  }
}

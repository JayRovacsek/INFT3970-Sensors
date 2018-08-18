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

// Consts
const char* ssid     = "Ooo Ooo Net";
const char* password = "itiswednesdaymydudes";
const char* host = "inft3970.com";
const char* id = "1";

// Required for string concat
char *strcat(char *dest, const char *src);

void setup()
{
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
  
  // Print output header for POC
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  // Print board type
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  // 5 is the I/O output as dictated; https://iotbytes.wordpress.com/nodemcu-pinout/ 
  dht.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
}

void post_to_api(double temperature, double humidity)
{ 
  char* temperature_json_payload = generate_temperature_json_payload(temperature);
  char* humidity_json_payload = generate_humidity_json_payload(humidity);
  http.begin("http://192.168.1.11:49987/api/Temperature/Create");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(temperature_json_payload);   //Send the request
  String payload = http.getString();                  //Get the response payload
  
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  
  http.end();  //Close connection

  http.begin("http://192.168.1.11:49987/api/Humidity/Create");
  http.addHeader("Content-Type", "application/json");
  httpCode = http.POST(humidity_json_payload);   //Send the request
  payload = http.getString();                  //Get the response payload
  
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  
  http.end();  //Close connection
}

char* generate_temperature_json_payload(double temperature)
{
  char temp_output[5];
  char json_load[100];
  
  snprintf(temp_output, 5, "%f", temperature);

  strcpy(json_load, "{\"Id\":\"");
  strcat(json_load, id);
  strcat(json_load, "\",\"Temperature\":\"");
  strcat(json_load, temp_output);
  strcat(json_load, "\"}");

  Serial.println("Generated json payload:");
  Serial.println(json_load);
  return json_load;
}

char* generate_humidity_json_payload(double humidity)
{
  char humidity_output[5];
  char json_load[100];
  
  snprintf(humidity_output, 5, "%f", humidity);

  strcpy(json_load, "{\"Id\":\"");
  strcat(json_load, id);
  strcat(json_load, "\",\"Humidity\":\"");
  strcat(json_load, humidity_output);
  strcat(json_load, "\"}");

  Serial.println("Generated json payload:");
  Serial.println(json_load);
  return json_load;
}

void print_serial_details(double humidity, double temperature)
{
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(humidity, 1);
    Serial.print("\t\t");
    Serial.print(temperature, 1);
    Serial.print("\t\t");
    Serial.print(dht.toFahrenheit(temperature), 1);
    Serial.print("\t\t");
    Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
    Serial.print("\t\t");
    Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED){
    // Delay minimum sampling period to avoid errors with the sensor
    //delay(dht.getMinimumSamplingPeriod());
    delay(1000);
  
    double humidity = dht.getHumidity();
    double temperature = dht.getTemperature();
    print_serial_details(temperature,humidity);
    post_to_api(temperature,humidity);
  }
}

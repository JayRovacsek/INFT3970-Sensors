#include "DHTesp.h"

// DHTesp is a library imported from; https://github.com/beegee-tokyo/DHTesp
// A fair bit of the below code is reused from: https://github.com/beegee-tokyo/DHTesp/tree/master/examples/DHT_ESP8266 
DHTesp dht;

void setup()
{
  // Set serial baud rate at 115200
  Serial.begin(115200);
  Serial.println();
  // Print output header for POC
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  // Print board type
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  // 5 is the I/O output as dictated; https://iotbytes.wordpress.com/nodemcu-pinout/ 
  dht.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
}

void loop()
{
  // Delay minimum sampling period to avoid errors with the sensor
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

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

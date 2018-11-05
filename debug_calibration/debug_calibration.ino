#include <DHTesp.h>
#include <Arduino.h>

#define LED 2  //On board LED
#define MOTION 13 //Pin Motion sensor is associated with
#define TH 5 //Pin Motion sensor is associated with

DHTesp dht;

void setup()
{
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(MOTION, INPUT);

  Serial.begin(115200);
  delay(50);

  dht.setup(TH, DHTesp::DHT11);
}

void debug(){

  double humidity = dht.getHumidity();
  double temperature = dht.getTemperature();
  bool motion = getMotion();

  Serial.println("Motion Detected: " + String(motion));
  Serial.println("--------------------------------------------------");

  Serial.println("Temperature: " + String(temperature,2));
  Serial.println("--------------------------------------------------");

  Serial.println("Humidity: " + String(humidity,2));
}

void changeState(){
  digitalWrite(LED,!(digitalRead(LED)));
}

bool getMotion(){
  bool result = digitalRead(MOTION);
  Serial.println("$$$$$$$$$ MOTION READING $$$$$$$$$");
  Serial.println(result);
  Serial.println("$$$$$$$$$ MOTION READING $$$$$$$$$");
  if(result){
      digitalWrite(LED,LOW);
  }
  else{
    digitalWrite(LED,HIGH);
  }
  return result;
}

void flashLED(int iterations, int msBetweenFlash){
  for(int i = 1; i <= iterations; i++){
    changeState();
    delay(msBetweenFlash/2);
    changeState();
    delay(msBetweenFlash/2);
  }
}

void loop()
{
  Serial.println("STARTED MODULE");
  delay(1000);
  while(true){
      debug();
      delay(3000);
  }
}

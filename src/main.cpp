#include <Arduino.h>
#include "DHT20.h"
#include <Wire.h>

// PINS Definitions
#define MOIST_PIN 15
#define LIGHT_PIN 2
#define SDA_PIN 21
#define SCL_PIN 22


// GLOBAL vars
DHT20 DHT(&Wire);
float moistThreshold;
float moistMax;
float moistMin;

float lightThreshold;
float lightMax;
float lightMin;


// SETUPS AND CALIBRATORS
void calibrateLight(){
  float max = 0.0;
  float min = 100000000;
  int period = 5 * 1000;
  Serial.println(" > Please place plant on the sun");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( LIGHT_PIN );
    
    if (lightValue > max){
      max = lightValue;
    }
    delay(10);
  }
  Serial.println(" > Please place in a dark spot");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( MOIST_PIN );
    if (lightValue < min){
      min = lightValue;
    }
    delay(10);
  }
  lightThreshold = (max-min)/2;
  lightMax = max;
  lightMin = min;
  
}

void calibrateMoist(){
  float max = 0.0;
  float min = 100000000;
  int period = 5 * 1000;
  
  Serial.println(" > Please put moisture sensor in dry soil");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float moistureValue = analogRead( MOIST_PIN );
    if (moistureValue > max){
      max = moistureValue;
    }
    delay(10);
  }
  Serial.println(" > Please put moisture sensor in wet soil");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float moistureValue = analogRead( MOIST_PIN );
    if (moistureValue < min){
      min = moistureValue;
    }
    delay(10);
  }
  moistThreshold = (max-min)/2;
}

void setComDHT20(){
  Serial.println();
  Serial.println("Connecting to Sensor!");
  while(1){
    if (!DHT.begin(SDA_PIN, SCL_PIN)) {
      Serial.println("Sensor not found!");
    delay(1000);
    }else{
      Serial.println("------- DHT20 Connected -------");
      Serial.print("DHT20 LIBRARY VERSION: ");
      Serial.println(DHT20_LIB_VERSION);
      Serial.println("Type,\tStatus,\tHumidity (%),\tTemperature (C)");
      break;
    }
  }
  Serial.println();
}

// Getters



/// Program

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setComDHT20();
}



void loop() {
  // put your main code here, to run repeatedly:

  // high value means dry
  // low value means wet
  
  int lightValue = analogRead( LIGHT_PIN );
  Serial.println(lightValue);

  int status = DHT.read();
  float temp = DHT.getTemperature();
  float hum = DHT.getHumidity();

  Serial.println(temp);
  Serial.println(hum);
  Serial.println();
  delay(1000);
}
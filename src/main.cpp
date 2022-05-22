#include <Arduino.h>
#include "DHT20.h"
#include <Wire.h>

#include <WiFi.h>
#include <HttpClient.h>

// PINS Definitions
#define MOIST_PIN 15
#define LIGHT_PIN 2
#define SDA_PIN 21 // Green
#define SCL_PIN 22 // Yellow

#define PORT_NUM 3001 /// Here goes the port number
#define IP_ADDR "54.183.254.192" 
#define IS_LOCAL true

char ssid[] = "MyCampusNet Legacy";    // your network SSID (name) 
char pass[] = ""; // your network password (use for WPA, or use as key for WEP)


// GLOBAL vars
DHT20 DHT(&Wire);
float dryThreshold;
float dryMax;
float dryMin;

float lightThreshold;
float lightMax;
float lightMin;

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;
char charPath[48]; // here is the value /?var=10




// SETUPS AND CALIBRATORS
void setComWiFi(bool isLocal = true){
  if (isLocal){ return;}
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void calibrateLight(){
  float max = 0.0;
  float min = 100000000;
  int period = 5 * 1000;
  Serial.println(" > Please place plant ON the SUN");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( LIGHT_PIN );
    
    if (lightValue > max){
      max = lightValue;
    }
    delay(10);
  }
  Serial.println(" > Please place in a DARK spot");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( LIGHT_PIN );
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
  
  Serial.println(" > Please put moisture sensor in DRY soil");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float rawVal = analogRead( MOIST_PIN );
    if (rawVal > max){
      max = rawVal;
    }
    delay(10);
  }
  Serial.println(" > Please put moisture sensor in WET soil");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float rawVal = analogRead( MOIST_PIN );
    if (rawVal < min){
      min = rawVal;
    }
    delay(10);
  }
  dryThreshold = (max-min)/2;
  dryMax = max;
  dryMin = min;
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
float getMoist(){
//  map(value, fromLow, fromHigh, toLow, toHigh);
  float rawDry = analogRead( MOIST_PIN );
  float valDry = map(rawDry, dryMin, dryMax, 0, 100);
  float valMoist = 100 - valDry;
  return valMoist;
}

float getLight(){
//  map(value, fromLow, fromHigh, toLow, toHigh);
  float rawLight = analogRead( LIGHT_PIN );
  float valLight = map(rawLight, lightMin, lightMax, 0, 100);
  return valLight;
  
}


float getTemp(){
//  map(value, fromLow, fromHigh, toLow, toHigh);
  int status = DHT.read();
  float temp = DHT.getTemperature();
  return temp;
}

float getHum(){
//  map(value, fromLow, fromHigh, toLow, toHigh);
  int status = DHT.read();
  // float temp = DHT.getTemperature();
  float hum = DHT.getHumidity();
  return hum;
}


int sendData(float moist, float light, float temp, float hum, bool isLocal = true){
  if (isLocal){return 200;}
  WiFiClient c;
  HttpClient http(c);
  int err =0;
  String path = "/add?";
  path.concat("moisture=");
  path.concat(String(moist, 2));
  path.concat("&");
  path.concat("light=");
  path.concat(String(light, 2));
  path.concat("&");
  path.concat("temperature=");
  path.concat(String(temp, 2));
  path.concat("&");
  path.concat("humidity=");
  path.concat(String(hum, 2));
  path.toCharArray(charPath, path.length());

  err = http.get(IP_ADDR, PORT_NUM,  charPath);
  if (err == 0){
      err = http.responseStatusCode();
      if (err >= 0){
        Serial.print("Response status: ");
        Serial.println(err);
        err = http.skipResponseHeaders();
        if (err >= 0){
          int bodyLen = http.contentLength();
          Serial.print("HTTP Response Body: ");
          unsigned long timeoutStart = millis();
          char c;
          while ( (http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)){
              if (http.available()){
                  c = http.read();
                  Serial.print(c);
                  bodyLen--;
                  timeoutStart = millis();
              }
              else{
                  delay(kNetworkDelay);
              }
          }
        }
        else{
          Serial.print("Failed to skip response headers: ");
          Serial.println(err);
        }
      }
      else{    
        Serial.print("Getting response failed: ");
        Serial.println(err);
      }
    }
    else{
      Serial.print("Connect failed: ");
      Serial.println(err);
    }
    http.stop();
    return err;
}




///////////////////////////
////     Program       ////
///////////////////////////
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setComDHT20();
  setComWiFi(IS_LOCAL); // set isLocal to false
  calibrateMoist();
  delay(5000);
  calibrateLight();
  
  Serial.printf("%s\t\t%s\t\t%s\t\t%s\t\t%s\n","Light","Moist","Temp", "Hum", "status");
}



void loop() {
  // put your main code here, to run repeatedly:

  // high value means dry
  // low value means wet
  
  float moistValue = getMoist();
  float lightValue = getLight();
  float tempValue  = getTemp();
  float humValue = getHum();
  int status = sendData(moistValue, lightValue, tempValue, humValue, IS_LOCAL); // set isLocal to true
  Serial.printf("%f\t%f\t%f\t%f\t%d\n",lightValue, moistValue, tempValue, humValue, status);
  // Serial.println(temp);
  // Serial.println(hum);
  // Serial.println();
  delay(1000);
}
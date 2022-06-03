#include <Arduino.h>
#include "DHT20.h"
#include <Wire.h>

#include <WiFi.h>
#include <HttpClient.h>

// PINS Definitions
#define LIGHT_PIN   39 // yellow cable
#define MOIST_PIN   32 // blue cable


#define LED_RED  33 
#define LED_YEL  25 
#define LED_GRE  26 

#define WATER_PIN 27
#define USING_RELAY true

#define SDA_PIN 21 // Green cable
#define SCL_PIN 22 // Yellow


#define PORT_NUM 3001 /// Here goes the port number
#define IP_ADDR "54.213.111.92" 
#define IS_LOCAL false

char ssid[] = "MyCampusNet Legacy";    // your network SSID (name) 
char pass[] = ""; // your network password (use for WPA, or use as key for WEP)


// GLOBAL vars
DHT20 DHT(&Wire);
float dryThreshold;
float dryMax;
float dryMin;

float lightThreshold;
int lighttMax;
int lighttMin;

int test0;
int test1;
int test2;

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;
char charPath[52] = "0000"; // here is the value /?var=10




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
  float min = 1000000;
  int period = 5 * 1000;
  Serial.print(" > Please place plant ON the SUN: ");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( LIGHT_PIN );
    
    if (lightValue > max){
      max = lightValue;
    }
    delay(10);
  }
  Serial.println(max);
  Serial.print(" > Please place in a DARK spot: ");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float lightValue = analogRead( LIGHT_PIN );
    if (lightValue < min){
      min = lightValue;
    }
    delay(10);
  }
  Serial.println(min);
  lightThreshold = (max-min)/2;
  lighttMax = max;
  lighttMin = min;
  test0 = min;
  test1=min;
  test2=max;
  
}
void calibrateMoist(){
  float max = 0.0;
  float min = 10000000;
  int period = 5 * 1000;
  
  Serial.print(" > Please put moisture sensor in DRY soil: ");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float rawVal = analogRead( MOIST_PIN );
    if (rawVal > max){
      max = rawVal;
    }
    delay(10);
  }
  Serial.println(max);

  Serial.print(" > Please put moisture sensor in WET soil: ");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    float rawVal = analogRead( MOIST_PIN );
    if (rawVal < min){
      min = rawVal;
    }
    delay(10);
  }
  Serial.println(min);

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
      // Serial.println("Type,\tStatus,\tHumidity (%),\tTemperature (C)");
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
  valDry = constrain(valDry, 0, 100); 
  float valMoist = 100 - valDry;
  // valMoist = constrain(valMoist, 0, 100); 
  return valMoist;
}

float getLight(){
//  map(value, fromLow, fromHigh, toLow, toHigh);
  float rawLight = analogRead( LIGHT_PIN );
  // Serial.printf("Raw Light: %f\n",rawLight);
  // Serial.printf("Min: %d\tMax:%d\n",lighttMin, lighttMax);
  // Serial.printf("TMin: %d\tTMin: %d\tTMax:%d\n",test0,test1, test2);
  float valLight = map(rawLight, lighttMin, lighttMax, 0, 100);
  valLight = constrain(valLight, 0, 100);
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


int charTOint(char c){
  // Serial.printf(" %d %d %d ",c=='0', c=='1',c=='2');
  if (c == '0'){
    return 0;
  }else if (c=='1'){
    return 1;
  }else if(c=='2'){
    return 2;
  }else{
    return 0;
  }
}


void turnLED(int LED_PIN){
  switch (LED_PIN)
  {
  case LED_RED:
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_YEL, LOW);
    digitalWrite(LED_GRE, LOW);
    break;
  case LED_YEL:
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GRE, LOW);
    break;
  case LED_GRE:
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YEL, LOW);
    break;
  }
}

void processStatus(int moistSta, int lightSta, int waterSta){
  if (moistSta == 0 && lightSta == 0){
    turnLED(LED_GRE);
  }
  else if(moistSta > 1  || lightSta  > 1){
    turnLED(LED_RED);
  }
  else /* if (moistSta > 1 || lightSta >1)*/ {
    turnLED(LED_YEL);
  }
  if (waterSta == 1){
    if (USING_RELAY){
    digitalWrite(WATER_PIN, LOW);
    }else{
      digitalWrite(WATER_PIN, HIGH);
    }
  }else{
    if (USING_RELAY){
      digitalWrite(WATER_PIN, HIGH);
    }else{
      digitalWrite(WATER_PIN, LOW);
    }
  }

}

uint8_t switchLED(int LED_PIN, uint8_t LED_status){

  if (LED_status == 0){
    digitalWrite(LED_PIN, HIGH);
    LED_status = 1;
  }else{
    digitalWrite(LED_PIN, LOW);
    LED_status = 0;
  }
  return LED_status; 
}

void testLED(uint8_t Rcurr, uint8_t Ycurr, uint8_t Gcurr){
  int period = 2 * 1000;
  
  Serial.println(" > Please wait, We testing the LEDs");
  uint8_t rstatus = Rcurr;
  uint8_t ystatus = Ycurr;
  uint8_t gstatus = Gcurr;
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    rstatus = switchLED(LED_RED, rstatus);
    ystatus = switchLED(LED_YEL, ystatus);
    gstatus = switchLED(LED_GRE, gstatus);
    delay(200);
  }
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    rstatus = switchLED(LED_RED, rstatus);
    delay(100);
    ystatus = switchLED(LED_YEL, ystatus);
    delay(100);
    gstatus = switchLED(LED_GRE, gstatus);
    delay(100);
  }

  if (rstatus != Rcurr){
    rstatus = switchLED(LED_RED, rstatus);
    ystatus = switchLED(LED_YEL, ystatus);
    gstatus = switchLED(LED_GRE, gstatus);
  }
}

void testWATER(uint8_t Scurr){
  uint8_t status = Scurr;
  int period = 3 * 1000;
  
  Serial.println(" > Please wait, We testing the Water");
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    status = switchLED(WATER_PIN, status);
    delay(1000);
  }
  if (status != Scurr){
    status = switchLED(WATER_PIN, status);
  }
}


///////////////////////////
////     Program       ////
///////////////////////////
// u8_t RED_status = 0;
// u8_t GRE_status = 0;
// u8_t YEL_status = 0;
// u8_t WAT_status = 0;




int sendData(float light,float moist, float temp, float hum, bool isLocal = true){
  if (isLocal){return 200;}
  WiFiClient c;
  HttpClient http(c);
  int err =0;
  String path = "/data?";
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
  // Serial.println(charPath);
  int status;
  err = http.post(IP_ADDR, PORT_NUM,  charPath);
  status = err;
  if (err == 0){
      err = http.responseStatusCode();
      status = err;
    }
    else{
      Serial.print("Connect failed: ");
      Serial.println(err);
    }
    http.stop();
    return status;
}
// char status [4] = "000";

int receiveData(bool isLocal){
  if (isLocal){return 200;}
  WiFiClient c;
  HttpClient http(c);
  int err =0;

  int status;
  err = http.get(IP_ADDR, PORT_NUM,  "/status");
  status = err;
  if (err == 0){
      err = http.responseStatusCode();
      status = err;
      if (err >= 0){
        err = http.skipResponseHeaders();
        if (err >= 0){
          int bodyLen = http.contentLength();
          unsigned long timeoutStart = millis();
          char c;
          String strStatus;
          // String status;
          // int indx = 0;
          // Serial.println();

          while ( (http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)){
              if (http.available()){
                  c = http.read();                  
                  // Serial.print(c);
                  strStatus.concat(c);

                  bodyLen--;
                  timeoutStart = millis();
              }
              else{
                  delay(kNetworkDelay);
              }
          }
        int moistSta = charTOint((char) strStatus[1]);
        int lightSta = charTOint((char) strStatus[2]);
        int waterSta = charTOint((char) strStatus[3]);

        processStatus(moistSta,lightSta, waterSta);
        status = moistSta*100+lightSta*10+waterSta;
        // Serial.printf(" -> %d\t%d\t%d", moistSta,lightSta, waterSta);
        // Serial.println();
        // Serial.println();
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
    return status;
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setComDHT20();
  setComWiFi(IS_LOCAL); // set isLocal to false

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YEL, OUTPUT);
  pinMode(LED_GRE, OUTPUT); 
  pinMode(WATER_PIN, OUTPUT); 
  digitalWrite(WATER_PIN, HIGH);
  delay(1000);

  testLED(0, 0, 0);
  
  testWATER((int) USING_RELAY);
  

  calibrateMoist();
  delay(5000);
  calibrateLight();

  Serial.printf("%s\t\t%s\t\t%s\t\t%s\t\t%s\t%s\n","Light","Moist","Temp", "Hum", "Connection", "Status");
}



void loop() {
  // put your main code here, to run repeatedly:

  // high value means dry
  // low value means wet
  
  float moistValue = getMoist();
  float lightValue = getLight();
  float tempValue  = getTemp();
  float humValue = getHum();
  // set back is local to true
  int conn = sendData(lightValue,moistValue, tempValue, humValue, IS_LOCAL); // set isLocal to true
  int status = receiveData(IS_LOCAL);
  Serial.printf("%f\t%f\t%f\t%f\t%d\t\t%03d\n",lightValue, moistValue, tempValue, humValue, conn, status);
  // Serial.println(temp);
  // Serial.println(hum);
  // Serial.println();
  delay(1000);
}
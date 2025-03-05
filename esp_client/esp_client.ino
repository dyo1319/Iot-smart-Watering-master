#include <DHT.h>
#include <ArduinoJson.h>

// ---- pins ------
#define dhtPin 16
#define pump 15
#define lightSensor 36
#define MoistureSensore 39

// ----- General data -----
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);
JsonDocument doc;
float CurrentTemp;
int light;
int minutes = (1000 * 60);
float temp;
int minT, maxT;
bool isOnPump;
int countOn = 0;

//----- State machine -----
#define TEMP_MODE 61
#define SOIL_MOISTURE_MODE 62
#define SABBATH_MODE 63
#define MANUAL_MODE 64
int CurrentStatus;
unsigned long statusCheckTime;
unsigned long DataPullTime;
unsigned long activationTime;

void setup() {
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  WiFi_SETUP();
  dht.begin();
  isOnPump = true;
  statusCheckTime = millis();
}

void loop() {
 if((millis() - statusCheckTime) > (10 * minutes)){
    CurrentStatus = GetState();
    statusCheckTime= millis();
 }
 
 switch(CurrentStatus){
    case TEMP_MODE:
      CurrentTemp = dht.readTemperature();
      light = map(analogRead(lightSensor),0,4095,0,100);
      if((millis() - DataPullTime) > (2 * minutes)){
        deserializeJson(doc, getJsonData("tempMode"));
        temp = (float) doc["temp"];
        minT = doc["minTime"];
        maxT = doc["maxTime"];
        DataPullTime = millis();
      }

      if(light > 90){
        isOnPump = true;
      }else if(light < 10 && countOn == 2){
        isOnPump = true;
        countOn = 0;
      }
        
      if(isOnPump && temp < CurrentTemp && countOn < 2 && light < 40){
        digitalWrite(pump, LOW);
        if(millis() - activationTime > (maxT * minutes)){
          digitalWrite(pump, HIGH);
          isOnPump = false;
          countOn++;
          activationTime = millis();
        }
      }else if(isOnPump && countOn < 2){
        digitalWrite(pump, LOW);
        if(millis() - activationTime > (minT * minutes)){
          digitalWrite(pump, HIGH);
          isOnPump = false;
          countOn++;
          activationTime = millis();
        }
      }
      
      break;
    case SOIL_MOISTURE_MODE:
      break;
    case SABBATH_MODE:
      break;
    case MANUAL_MODE:
      break;
 }
}

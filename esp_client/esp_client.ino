#include <DHT.h>
#include <ArduinoJson.h>

// ---- pins ------
#define dhtPin 16
#define pump 18
#define lightSensor 34
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
bool isOnPump = false;
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
unsigned long wateringDuration;

void setup() {
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  WiFi_SETUP();
  dht.begin();
  fetchWateringData();

  statusCheckTime = millis();
  DataPullTime = millis();
}

void loop() {
  //Check State Every 10mins
 if((millis() - statusCheckTime) > (10 * minutes)){
    CurrentStatus = GetState();
    statusCheckTime= millis();
 }
 
 switch(61){
    case TEMP_MODE:
      handleTempMode();
      break;
    case SOIL_MOISTURE_MODE:
      break;
    case SABBATH_MODE:
      break;
    case MANUAL_MODE:
      break;
 }
}

void handleTempMode() {
  CurrentTemp = dht.readTemperature();// read temp from sensor live
  light = map(analogRead(lightSensor),0,4095,0,100);//read light from sensor live

  //after 2 mins asgin the values and then Get Data every 2mins
  if ((millis() - DataPullTime) > (2 * minutes)) {
        fetchWateringData();
  }

      Serial.println(temp);
      Serial.println(minT);
      Serial.println(maxT);
      delay(1000);

  //check if should water
  if(CurrentTemp > temp){
    wateringDuration = (maxT * minutes);
  }else {
    wateringDuration = (minT * minutes);
  }
  //turn on pump with the desired time
  if (!isOnPump) {
    TurnOnPump();
    activationTime = millis(); // התחלת זמן ההשקיה
  }

  //turn off tump after desired time.
  if (isOnPump && (millis() - activationTime >= wateringDuration)) {
    TurnOffPump();
  }

}

void TurnOnPump() {
  Serial.println("Pump ON");
  digitalWrite(pump, LOW);
  isOnPump = true;
}

void TurnOffPump() {
  Serial.println("Pump OFF");
  digitalWrite(pump, HIGH);
  isOnPump = false;
}

void fetchWateringData() {
    Serial.println("Fetching watering data...");

    String jsonData = getJsonData("tempMode");  
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error) {
        Serial.print("JSON Parsing Failed: ");
        Serial.println(error.c_str());
        return;  
    }

    if (doc.containsKey("temp") && doc.containsKey("minTime") && doc.containsKey("maxTime")) {
        temp = (float)doc["temp"];
        minT = doc["minTime"];
        maxT = doc["maxTime"];
        DataPullTime = millis(); 

        Serial.println("Watering data updated successfully!");
        Serial.print("Target Temp: "); Serial.println(temp);
        Serial.print("Min Watering Time: "); Serial.println(minT);
        Serial.print("Max Watering Time: "); Serial.println(maxT);
    } else {
        Serial.println("Missing keys in JSON data!");
    }
}


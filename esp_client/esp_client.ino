#include <DHT.h>
#include <ArduinoJson.h>

// ---- pins ------
#define dhtPin 16
#define pump 18
#define lightSensor 34
#define MoistureSensore 39

// ----- General data -----
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);
JsonDocument doc;
float CurrentTemp;
int light;
int minutes = (1000 * 60);
float temp;
int minT, maxT;
bool isOnPump = false;
int countOn = 0;

//Water Schudle Time
unsigned long lastWateringTime = 0;
bool morningWateringDone = false;
bool eveningWateringDone = false;
int currentHour = 0;
#define MORNING_HOUR 7  // 7 AM
#define EVENING_HOUR 20 // 7 PM

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
  digitalWrite(pump, HIGH); // Make sure pump is OFF at startup
  isOnPump = false;
  statusCheckTime = millis();
  DataPullTime = millis(); // Initialize this to trigger initial data fetch
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
  // Read sensor data
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  
  // Fetch data from server periodically
  if ((millis() - DataPullTime) > (2 * minutes)) {
    // Send data to server
    sendData(CurrentTemp, light, moisture);
    // Fetch updated settings and time
    fetchWateringData();
    currentHour = getCurrentHour();
    DataPullTime = millis();
  }
  
  // If pump is already running, check if watering duration is complete
  if (isOnPump) {
    if (millis() - activationTime >= wateringDuration) {
      TurnOffPump();
    }
    return; // Skip the rest of the function while pump is running
  }
  
  // Reset daily flags at midnight
  if (currentHour == 0) {
    morningWateringDone = false;
    eveningWateringDone = false;
  }
  
  // Priority condition #1: Evening watering (highest priority)
  if ((currentHour == EVENING_HOUR || currentHour == 21) && !eveningWateringDone) {
    calculateWateringDuration();
    TurnOnPump();
    eveningWateringDone = true;
    return;
  }
  
  // Priority condition #2: Shade-based watering (medium priority)
  // If light level is low (shady) and morning watering hasn't happened yet
  if (light < 30 && !morningWateringDone && currentHour > 6 && currentHour < 17) {
    calculateWateringDuration();
    TurnOnPump();
    morningWateringDone = true;
    return;
  }
  
  // Fallback condition: Morning scheduled watering if no shade opportunity occurred
  if (currentHour == MORNING_HOUR && !morningWateringDone) {
    calculateWateringDuration();
    TurnOnPump();
    morningWateringDone = true;
    return;
  }
}


void TurnOnPump() {
  Serial.println("Pump ON");
  digitalWrite(pump, LOW);
  isOnPump = true;
  activationTime = millis();
  
  // Send updated status immediately after turning pump on
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  sendData(CurrentTemp, light, moisture);
}

void TurnOffPump() {
  Serial.println("Pump OFF");
  digitalWrite(pump, HIGH);
  isOnPump = false;
  lastWateringTime = millis();
  
  // Send updated status immediately after turning pump off
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  sendData(CurrentTemp, light, moisture);
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

// Helper function to calculate watering duration based on temperature
void calculateWateringDuration() {
  if (CurrentTemp > temp) {
    wateringDuration = (maxT * minutes);
    Serial.print("Temperature above threshold. Watering for ");
    Serial.print(maxT);
    Serial.println(" minutes");
  } else {
    wateringDuration = (minT * minutes);
    Serial.print("Temperature below threshold. Watering for ");
    Serial.print(minT);
    Serial.println(" minutes");
  }
}
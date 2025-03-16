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
unsigned long lastSensorDataTransmission = 0;
#define SENSOR_TRANSMISSION_INTERVAL (3 * 60 * 60 * 1000) // 3 hours in milliseconds
bool pumpShouldBeOn = false;
JsonArray scheduleArray;  // To store the schedule array from JSON

//TempMode Global Variables
unsigned long lastWateringTime = 0;
bool morningWateringDone = false;
bool eveningWateringDone = false;
int currentHour = 0;
#define MORNING_HOUR 7  // 7 AM
#define EVENING_HOUR 20 // 8 PM

//SoilMoisture Global Variables
float desiredMoisture;      // Target moisture level (%)
float toleranceRange;       // Range around target (±%)
unsigned long lastMoistureCheckTime = 0;
#define MOISTURE_CHECK_INTERVAL (1 * minutes)  // Check moisture every minute


//SabbathMode Global Variables
bool scheduleExecuted = false;
int lastScheduleDay = -1;
int lastScheduleHour = -1;


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
  lastSensorDataTransmission = millis(); // Initialize this for sensor transmission
}

void loop() {
  //Check State Every 10mins
  if((millis() - statusCheckTime) > (1 * minutes)){
    CurrentStatus = GetState();
    statusCheckTime = millis();
  }

  // Check if it's time to send sensor data (every 3 hours)
  if (millis() - lastSensorDataTransmission > SENSOR_TRANSMISSION_INTERVAL) {
    readAndSendSensorData();
    lastSensorDataTransmission = millis();
  }
  
  switch(CurrentStatus){
    case TEMP_MODE:
      handleTempMode();
      break;
    case SOIL_MOISTURE_MODE:
      handleSoilMoistureMode();
      break;
    case SABBATH_MODE:
      handleSabbathMode();
      break;
    case MANUAL_MODE:
      handleManualMode();
      break;
    default:
      break;
  }
  delay(100);
}

// function to read and send sensor data
void readAndSendSensorData() {
  // Read all sensor data
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  
  // Debug print raw values
  Serial.print("Raw moisture value: ");
  Serial.println(analogRead(MoistureSensore));
  Serial.print("Mapped moisture value: ");
  Serial.println(moisture);
  
  // Send data to server
  sendData(CurrentTemp, light, moisture);
}

void TurnOnPump() {
  Serial.println("Pump ON");
  digitalWrite(pump, LOW);
  isOnPump = true;
  activationTime = millis();
}

void TurnOffPump() {
  Serial.println("Pump OFF");
  digitalWrite(pump, HIGH);
  isOnPump = false;
  lastWateringTime = millis();
}

void handleTempMode() {
  // Read sensor data
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  
  // Fetch data from server periodically
  if ((millis() - DataPullTime) > (2 * minutes)) {
    // Send data to server
    readAndSendSensorData();
    // Fetch updated settings and time
    fetchWateringData();
    currentHour = getCurrentHour();
    DataPullTime = millis();
  }
  
  // If pump is already running, check if watering duration is complete
  if (isOnPump) {
    if (millis() - activationTime >= wateringDuration) {
      TurnOffPump();
      readAndSendSensorData(); // Send updated data after turning off pump
    }
    return; // Skip the rest of the function while pump is running
  }
  
  // Reset daily flags at midnight
  if (currentHour == 0) {
    morningWateringDone = false;
    eveningWateringDone = false;
  }
  
  // Priority condition #1: Evening watering (highest priority)
  if ((currentHour == EVENING_HOUR) && !eveningWateringDone) {
    calculateWateringDuration();
    TurnOnPump();
    readAndSendSensorData(); // Send updated data after turning on pump
    eveningWateringDone = true;
    return;
  }
  
  // Priority condition #2: Shade-based watering (medium priority)
  if (light < 30 && !morningWateringDone && currentHour > 6 && currentHour < 17) {
    calculateWateringDuration();
    TurnOnPump();
    readAndSendSensorData(); // Send updated data after turning on pump
    morningWateringDone = true;
    return;
  }
  
  // Fallback condition: Morning scheduled watering if no shade opportunity occurred
  if (currentHour == MORNING_HOUR && !morningWateringDone) {
    calculateWateringDuration();
    TurnOnPump();
    readAndSendSensorData(); // Send updated data after turning on pump
    morningWateringDone = true;
    return;
  }
}

// Function to fetch TempMode settings from server
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

// Function to fetch soil moisture settings from server
void fetchSoilMoistureData() {
  Serial.println("Fetching soil moisture settings...");
  
  String jsonData = getJsonData("SOILMOISTURE");
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON Parsing Failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (doc.containsKey("desiredMoisture") && doc.containsKey("toleranceRange")) {
    desiredMoisture = (float)doc["desiredMoisture"];
    toleranceRange = (float)doc["toleranceRange"];
    DataPullTime = millis(); 
    
    Serial.println("Soil moisture settings updated successfully!");
    Serial.print("Target moisture: "); Serial.print(desiredMoisture); Serial.println("%");
    Serial.print("Tolerance range: ±"); Serial.print(toleranceRange); Serial.println("%");
  } else {
    Serial.println("Missing keys in soil moisture settings!");
  }
}

void handleSoilMoistureMode() {
  // Read current sensor data
  CurrentTemp = dht.readTemperature();
  light = map(analogRead(lightSensor), 0, 4095, 0, 100);
  int moisture = map(analogRead(MoistureSensore), 0, 4095, 0, 100);
  
  // Fetch settings from server periodically
  if ((millis() - DataPullTime) > (2 * minutes)) {
    // Send current readings to server
    readAndSendSensorData();
    
    // Fetch updated settings
    fetchSoilMoistureData();
    DataPullTime = millis();
  }
  
  // If pump is already running, check if we should stop
  if (isOnPump) {
    // Calculate the upper moisture threshold
    float upperThreshold = desiredMoisture + toleranceRange;
    
    // Stop watering if we've reached the upper threshold
    if (moisture >= upperThreshold) {
      Serial.println("Moisture level reached upper threshold, stopping pump");
      TurnOffPump();
      readAndSendSensorData(); // Send updated data after turning off pump
    }
    // Also stop if the pump has been running too long (safety feature)
    else if (millis() - activationTime >= (10 * minutes)) {
      Serial.println("Maximum watering time reached, stopping pump");
      TurnOffPump();
      readAndSendSensorData(); // Send updated data after turning off pump
    }
    return; // Skip the rest while pump is running
  }
  
  // Only check if we need to water periodically to avoid rapid on/off cycles
  if (millis() - lastMoistureCheckTime > MOISTURE_CHECK_INTERVAL) {
    // Calculate the lower moisture threshold
    float lowerThreshold = desiredMoisture - toleranceRange;
    
    // Start watering if moisture is below the lower threshold
    if (moisture < lowerThreshold) {
      Serial.print("Moisture level (");
      Serial.print(moisture);
      Serial.print("%) below lower threshold (");
      Serial.print(lowerThreshold);
      Serial.println("%), starting pump");
      
      TurnOnPump();
      readAndSendSensorData(); // Send updated data after turning on pump
    }
    
    lastMoistureCheckTime = millis();
  }
}

void handleSabbathMode() {
  // Fetch schedule from server periodically
  if ((millis() - DataPullTime) > (2 * minutes)) {
    // Send current readings to server
    readAndSendSensorData();
    
    // Fetch updated schedule and current time
    fetchSabbathSchedule();
    currentHour = getCurrentHour();
    int currentDay = getCurrentDay();
    
    // Reset the execution flag if the hour or day has changed
    if (currentHour != lastScheduleHour || currentDay != lastScheduleDay) {
      scheduleExecuted = false;
      lastScheduleHour = currentHour;
      lastScheduleDay = currentDay;
    }
    
    DataPullTime = millis();
  }
  
  // If pump is already running, check if watering duration is complete
  if (isOnPump) {
    if (millis() - activationTime >= wateringDuration) {
      TurnOffPump();
      readAndSendSensorData();
    }
    return;
  }
  
  // Only proceed if we haven't already executed the schedule for this hour
  if (!scheduleExecuted) {
    // Get current day and hour
    int currentDay = getCurrentDay();
    
    // Check JSON schedule to see if we should water now
    for (JsonObject schedule : scheduleArray) {
      if (schedule["dayOfWeek"] == currentDay && schedule["startHour"] == currentHour) {
        wateringDuration = (int)schedule["duration"] * minutes;
        TurnOnPump();
        readAndSendSensorData();
        scheduleExecuted = true;  // Mark this schedule as executed
        return;
      }
    }
  }
}

void fetchSabbathSchedule() {
  Serial.println("Fetching Sabbath schedule...");
  
  String jsonData = getJsonData("SABBATH");
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON Parsing Failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (doc.containsKey("schedule")) {
    // Store the schedule array reference
    scheduleArray = doc["schedule"].as<JsonArray>();
    
    Serial.println("Sabbath schedule updated successfully!");
    Serial.print("Number of schedules: ");
    Serial.println(scheduleArray.size());
    
    // Debug print the schedules
    for (JsonObject schedule : scheduleArray) {
      Serial.print("Day: ");
      Serial.print((int)schedule["dayOfWeek"]);
      Serial.print(", Hour: ");
      Serial.print((int)schedule["startHour"]);
      Serial.print(", Duration: ");
      Serial.println((int)schedule["duration"]);
    }
  } else {
    Serial.println("Missing schedule in JSON data!");
  }
}

void handleManualMode() {
  // Fetch manual control data periodically
  if ((millis() - DataPullTime) > (1 * minutes)) {
    // Send current readings to server
    readAndSendSensorData();
    
    // Fetch manual control data
    fetchManualControlData();
    DataPullTime = millis();
  }
  
  // Handle pump state based on manual control
  if (pumpShouldBeOn && !isOnPump) {
    // Delay for 3 seconds before activating
    delay(3000);
    TurnOnPump();
    readAndSendSensorData();
  } else if (!pumpShouldBeOn && isOnPump) {
    TurnOffPump();
    readAndSendSensorData();
  }
}

void fetchManualControlData() {
  Serial.println("Fetching manual control data...");
  
  String jsonData = getJsonData("MANUAL");
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON Parsing Failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (doc.containsKey("pumpState")) {
    pumpShouldBeOn = doc["pumpState"];
    
    Serial.print("Manual pump state: ");
    Serial.println(pumpShouldBeOn ? "ON" : "OFF");
    
    if (doc.containsKey("lastCommand")) {
      unsigned long lastCommand = doc["lastCommand"];
      Serial.print("Last command timestamp: ");
      Serial.println(lastCommand);
    }
  } else {
    Serial.println("Missing pump state in JSON data!");
  }
}
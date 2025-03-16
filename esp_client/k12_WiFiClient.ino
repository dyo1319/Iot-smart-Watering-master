#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

const char* ssid = "Rageb heno";
const char* password = "225524133";
const char* serverAddress = "http://10.100.102.12:3001";

WiFiClient client;

void WiFi_SETUP(){
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void sendData(float temp, int light, int moisture){
  HTTPClient http;
  
  String url = String(serverAddress) + "/esp?";
  url += "temp=" + String(temp);
  url += "&linght=" + String(light);
  url += "&moisture=" + String(moisture);
  url += "&treeId=13";
  url += "&isRunning=" + String(isOnPump ? 1 : 0);
  
  Serial.println(url);
  
  http.begin(client, url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println("OK: " + response);
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

// Helper function to get current state from server
int GetState() {
  int ret = -1;
  HTTPClient http;
  
  http.begin(client, String(serverAddress) + "/esp/state");
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    JsonDocument stateDoc;
    DeserializationError error = deserializeJson(stateDoc, response);
    
    if (!error) {
      ret = stateDoc["state"];
      Serial.print("State: ");
      Serial.println(ret);
    }
  }
  
  http.end();
  return ret;
}

String getJsonData(String state){  
  String json = "{}";
  HTTPClient http;
  
  http.begin(client, String(serverAddress) + "/esp/dataMode?state=" + state);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    json = http.getString();
  }
  
  http.end();
  return json;
}


// Helper function to get current hour from server
int getCurrentHour() {
  HTTPClient http;
  
  http.begin(client, String(serverAddress) + "/esp/state");
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    JsonDocument stateDoc;
    DeserializationError error = deserializeJson(stateDoc, response);
    
    if (!error && stateDoc.containsKey("date")) {
      int hour = stateDoc["date"];
      http.end();
      return hour;
    }
  }
  
  http.end();
  return -1; // Error case
}
// Helper function to get current day of week from server
int getCurrentDay() {
  HTTPClient http;
  
  http.begin(client, String(serverAddress) + "/esp/state");
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    JsonDocument stateDoc;
    DeserializationError error = deserializeJson(stateDoc, response);
    
    if (!error && stateDoc.containsKey("day")) {
      int day = stateDoc["day"];
      http.end();
      return day;
    }
  }
  
  http.end();
  // Default to Sunday (0) if we can't get the day
  return 0;
}
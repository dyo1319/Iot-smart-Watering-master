#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

const char* ssid = "Raad";
const char* password = "95175386240";
const char* serverAddress = "http://172.20.10.2:3001";

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

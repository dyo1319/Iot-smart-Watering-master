#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

const char* ssid = "Gal";
const char* password = "0544933268";

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

void sendData(float temp, int linght, int moisture){
  HTTPClient http;
  String dataUrl = "temp=" + String(temp);
  dataUrl+= "&linght="+ String(linght);
  dataUrl+= "&moisture="+ String(moisture);
  http.begin(client, "http://192.168.1.83:3001/esp?" + dataUrl );

   int httpCode = http.GET();
   if(httpCode == HTTP_CODE_OK) {
     Serial.print("HTTP response code ");
     Serial.println(httpCode);
     //String Res = http.getString();
     //Serial.println(Res);
     //ret = Res.toInt();
    }
    http.end();
}

int GetState() {
    int ret = -1;
    HTTPClient http;
    http.begin(client, "http://192.168.1.83:3001/esp/state");
    int httpCode = http.GET();
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK) {
      Serial.print("HTTP response code ");
      Serial.println(httpCode);
      String Res = http.getString();
      Serial.println(Res);
      ret = Res.toInt();
    }
    http.end();
        
    return ret;
}

String getJsonData(String state){
  String json = "";
  HTTPClient http;
  http.begin(client, "http://192.168.1.83:3001/esp/dataMode?state="+state);
  int httpCode = http.GET();
  Serial.println(httpCode);
  if (httpCode == HTTP_CODE_OK) {
    Serial.print("HTTP response code ");
    Serial.println(httpCode);
    json = http.getString();
  }
    http.end();
        
    return json;
}

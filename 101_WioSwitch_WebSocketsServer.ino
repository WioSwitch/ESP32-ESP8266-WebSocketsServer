/*
  WioSwitch WebSocketsServer v1.0.3
  Written by Chiming Lee Oct 18, 2018 Arcadia, CA US
*/

//
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h> 
#include <FS.h>
//

//
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";
//

//
IPAddress gateway(192, 168, 1, 1);
IPAddress ip(192, 168, 1, 240);
IPAddress subnet(255, 255, 255, 0);
//

// 
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
//

//
const String deviceId = "WioSwitch";
//

//
const int relayPin = 5;
//

//
const long interval = 1000 * 10;
unsigned long previousMillis = 0;
//

//
void pinStatus(){
  String jsonResponse = "{ \"id\": \""+ deviceId +"\", \"type\": 1, \"value\": " + String(digitalRead(relayPin)) + " }";
  webSocket.broadcastTXT(jsonResponse); 
}
//

//
void toggle() {
  digitalWrite(relayPin, !digitalRead(relayPin));
  pinStatus();
}
//

//
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}
//

//
String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
//

//
bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
//

//
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
                pinStatus();
            }
            break;
        case WStype_TEXT: {
            Serial.printf("[%u] get Text: %s\n", num, payload); 
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(payload);
            if (root.success()) { 
              if (root["type"].as<int>() == 1) {
                toggle();
              }
            } else {
              String jsonResponse = "{ \"type\": 0, \"value\": \"invalid json command\" }";
              webSocket.broadcastTXT(jsonResponse);
            }
            break;
        }
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);
            break;
    } 
}
//

//
void setup(void){
  //
  Serial.begin(115200);
  while (!Serial) continue;
  Serial.println();
  Serial.println();
  //
  
  //
  SPIFFS.begin();
  //

  //
  pinMode(relayPin, OUTPUT);
  //
  
  //
  WiFi.config(ip, gateway, subnet);
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) { 
    WiFi.begin(ssid, password);
  }  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //
  
  //
  server.on("/", HTTP_GET, [](){
    if(!handleFileRead("/index.html")) server.send(404, "text/plain", "FileNotFound");
  });
  //
  
  //
  server.on("/status", HTTP_GET, [](){
    String json = "{ \"relayPin\": \""+ String(digitalRead(relayPin)) +"\" }";
    server.send(200, "application/json", json);
    json = String();
  });
  //

  //
  server.on("/about", HTTP_GET, [](){
    String json = "{ \"WioSwitch\": \"to switch digital pin On/Off\" }";
    server.send(200, "application/json", json);
    json = String();
  });
  //

  //
  server.on("/on", HTTP_GET, [](){
    digitalWrite(relayPin, HIGH);
    pinStatus();
    String json = "{ \"WioSwitch\": \"On\" }";
    server.send(200, "application/json", json);
    json = String();
  });
  //

  //
  server.on("/off", HTTP_GET, [](){
    digitalWrite(relayPin, LOW);
    pinStatus();
    String json = "{ \"WioSwitch\": \"Off\" }";
    server.send(200, "application/json", json);
    json = String();
  });
  //

  //
  server.on("/onoff", HTTP_GET, [](){
    digitalWrite(relayPin, HIGH);
    delay(1000);
    digitalWrite(relayPin, LOW);
    pinStatus();
    String json = "{ \"WioSwitch\": \"On/Off\" }";
    server.send(200, "application/json", json);
    json = String();
  });
  //

  //
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  //
  
  //
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  //

  //
  server.begin();
  Serial.println("HTTP server started");
  //

  //
  webSocket.begin();
  Serial.println("WebSocket server started");
  webSocket.onEvent(webSocketEvent);
  Serial.println();  
  //
}
//

//
void loop() {
  server.handleClient();
  webSocket.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    //
    previousMillis = currentMillis;
  }
}
//

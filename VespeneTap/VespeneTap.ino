#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FS.h>

#include <ESP8266WebServer.h>

#include <ArduinoOTA.h>

#include <WiFiManager.h>

#define STATUS_LED 0
#define LISTEN_PORT 80
#define RESTFUL_HOSTNAME "vespenehome"

ESP8266WebServer server(LISTEN_PORT);

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);

  digitalWrite(STATUS_LED, LOW);

  //Connect to wifi
  WiFiManager manager;
  manager.autoConnect();
  
  Serial.printf("WiFi connected!\r\nIP: %s\r\n", WiFi.localIP().toString().c_str());

  blinkTimes(1);

  delay(300);

  //initialize MDNS responder

  delay(300);

  //Initialize ArduinoOTA
  ArduinoOTA.setHostname(RESTFUL_HOSTNAME);
  ArduinoOTA.begin();

  //Start the SPI Flash Files System
  SPIFFS.begin();

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  blinkTimes(3);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}

String getContentType(String filename){
  if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){  // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.html";           // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){  // If the file exists, either as a compressed archive, or normal
    if(SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);
  return false;                                          // If the file doesn't exist, return false
}

void blinkTimes(int numTimes){
  for(int i = 0; i<numTimes; i++){
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(STATUS_LED, LOW);    
    delay(100);
  }
}

void errorBlink(){
  while(true){
    digitalWrite(STATUS_LED, LOW);
    delay(1000);
    digitalWrite(STATUS_LED, HIGH);
    delay(1000);
  }
}

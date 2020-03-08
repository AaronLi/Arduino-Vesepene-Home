#define DEBUGPRINT

#include <HTTPClient.h>
#include <WiFiServer.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>
#include <WiFi.h>
#include <aREST.h>
#include <ArduinoJson.h>

#define STATUS_LED 13
#define LISTEN_PORT 80
#define RESTFUL_HOSTNAME "vespenegeyser32"
#define ENDPOINT_ID "0"
#define ENDPOINT_NAME "VESPENE-GEYSER"
#define SERVICE_NAME "vespene"
#define JSON_CAPACITY 1024
#define SENSOR_POLL_INTERVAL_MS 30000

WiFiServer server(LISTEN_PORT);
aREST rest = aREST();

//JSON decoding stuff
DynamicJsonDocument doc(JSON_CAPACITY);

//Client info reading things
typedef struct VespeneSensor{
  char *sensorName;
  int temperature;
  int humidity;
  char * location;
} VespeneSensor;

VespeneSensor *sensorData;
int numSensors = 0;

char *sensorResponseString;

HTTPClient http;

// Sensor update timing
unsigned long lastUpdateTime = millis();

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  AsyncWebServer asyncServer(80);
  DNSServer dns;
  AsyncWiFiManager manager(&asyncServer, &dns);
  manager.autoConnect();
  Serial.printf("WiFi connected!\nIP: %s\n", WiFi.localIP().toString().c_str());

  blinkTimes(1);


  delay(300);

  rest.variable("house_status", &sensorResponseString);
  rest.set_id(ENDPOINT_ID);
  rest.set_name(ENDPOINT_NAME);

  Serial.println("RESTful API parameters set");

  blinkTimes(3);

  delay(300);

  ArduinoOTA.setHostname(RESTFUL_HOSTNAME);
  ArduinoOTA.begin();

  readAndStoreSensorInfo();
  
  server.begin();
  Serial.println("Server started");

  blinkTimes(4);


}

void loop() {
  ArduinoOTA.handle();

  if(millis() - lastUpdateTime > SENSOR_POLL_INTERVAL_MS){
      readAndStoreSensorInfo();
      lastUpdateTime = millis();
      #ifdef DEBUGPRINT
      Serial.printf("Free memory: %d\r\n", ESP.getFreeHeap());
      #else
      Serial.println(ESP.getFreeHeap());
      #endif
  }

  WiFiClient client = server.available();
  if(!client){
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
  blinkTimes(1);
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

void readAndStoreSensorInfo(){
  #ifdef DEBUGPRINT
  Serial.print("Fulfilling new sensor query");
  #endif
  for(int i = 0; i<numSensors; i++){
    free(sensorData[i].sensorName); // free dynamically allocated strings from last call
    free(sensorData[i].location);
  }
  
  int sensorResponses = MDNS.queryService(SERVICE_NAME, "tcp");
  numSensors = 0;
  int infoStorageSize = sizeof(VespeneSensor) * sensorResponses; // prepare to store all responses
  
  if(sensorData == NULL){
    sensorData = (VespeneSensor *)malloc(infoStorageSize); // if sensorData is NULL, allocate the memory. Else, reallocate with the pointer
  }else{
    realloc(sensorData, infoStorageSize);
  }
  
  WiFiClient client;

  int totalSizeBytes = 1; // null terminator

  for(int i = 0; i<sensorResponses; i++){
    delay(0);
    if(http.begin(client, "http://"+MDNS.IP(i).toString()+"/")){
      int responseCode = http.GET();
      if(responseCode > 0){
        if (responseCode == HTTP_CODE_OK || responseCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          deserializeJson(doc, payload);

          JsonObject variables = doc["variables"];
          int variables_temperature = variables["temperature"]; // 28
          int variables_humidity = variables["humidity"]; // 41
          const char* variables_location = variables["location"]; // "office"
          const char* nodeName = doc["name"]; // "VESPENE-002"

          int sensorNameLength = strlen(nodeName);
          int sensorLocationLength = strlen(variables_location);
          sensorData[numSensors].sensorName = (char *)malloc(sensorNameLength+1);
          strcpy(sensorData[numSensors].sensorName, nodeName);

          sensorData[numSensors].temperature = variables_temperature;
          sensorData[numSensors].humidity = variables_humidity;
          sensorData[numSensors].location = (char *)malloc(sensorLocationLength+1);
          strcpy(sensorData[numSensors].location, variables_location);

          totalSizeBytes += sensorNameLength + sensorLocationLength + sizeof(char) * 6 + sizeof(char) * 4; // allows for triple digit temperature and hunmidity, and separators
          ++numSensors;
          #ifdef DEBUGPRINT
          Serial.print(".");
          #endif
        }
      }
      else{
        #ifdef DEBUGPRINT
        Serial.printf("\r\nInvalid response from %s\n", MDNS.IP(i).toString().c_str());
        #endif
      }
      }else{
        #ifdef DEBUGPRINT
      Serial.printf("\r\nCould not connect to %s\n", MDNS.IP(i).toString().c_str());
      #endif
    }
  }
  #ifdef DEBUGPRINT
  Serial.println("\r\nWriting request to String");
  Serial.printf("New string approximate length: %d\r\nNumber of responses: %d\r\nNumber of sensors: %d\r\n", totalSizeBytes, sensorResponses, numSensors);
  #endif
  if(sensorResponseString == NULL){
    sensorResponseString = (char *)malloc(totalSizeBytes + 1);
  }else{
    realloc(sensorResponseString, totalSizeBytes + 1); // allow for null pointer in worst case
  }
  Serial.println("Memory allocated");
  sensorResponseString[0] = '\0';

  char buf[10]; // 6 digits, a separator, and a null, and a sensor separator
  for(int i = 0; i < numSensors; i++){
    delay(0);
    strcat(sensorResponseString, sensorData[i].sensorName);
    strcat(sensorResponseString, ",");
    strcat(sensorResponseString, sensorData[i].location);

    sprintf(buf, ",%d,%d;", sensorData[i].temperature, sensorData[i].humidity);

    strcat(sensorResponseString, buf);
    
  }
  #ifdef DEBUGPRINT
  Serial.println(sensorResponseString);
  Serial.println("Request fulfilled");
  #endif
}

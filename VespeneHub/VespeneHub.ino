//#define DEBUGPRINT

#include <ArduinoJson.h>

#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiServer.h>

#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <aREST.h>

#include <ArduinoOTA.h>

#define STATUS_LED 0
#define LISTEN_PORT 80
#define RESTFUL_HOSTNAME "vespenegeyser"
#define ENDPOINT_ID "0"
#define ENDPOINT_NAME "VESPENE-GEYSER"
#define SERVICE_NAME "vespene"
#define JSON_CAPACITY 1024
#define SENSOR_POLL_INTERVAL_MS 60000

//RESTful server things
aREST rest = aREST();

WiFiServer server(LISTEN_PORT);

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
int totalSizeBytes = 1;

HTTPClient http;

//JSON decoding stuff

DynamicJsonDocument doc(JSON_CAPACITY);

// Sensor update timing
unsigned long lastUpdateTime = millis();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  WiFiManager manager;
  manager.autoConnect();

  Serial.printf("WiFi connected!\nIP: %s\n", WiFi.localIP().toString().c_str());

  blinkTimes(1);

  delay(300);

   if(!MDNS.begin(RESTFUL_HOSTNAME)){
    Serial.println("mDNS Service could not be started");
    errorBlink();
  }
  else{
    blinkTimes(2);
    Serial.println("mDNS service started");
  }

  delay(300);
  rest.variable("house_status", &sensorResponseString);
  rest.set_id(ENDPOINT_ID);
  rest.set_name(ENDPOINT_NAME);

  Serial.println("RESTful API parameters set");

  blinkTimes(3);

  delay(300);

  //ArduinoOTA.begin();

  readAndStoreSensorInfo();

  server.begin();
  Serial.println("Server started");

  blinkTimes(4);


}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - lastUpdateTime > SENSOR_POLL_INTERVAL_MS){
      readAndStoreSensorInfo();
      lastUpdateTime = millis();
      #ifdef DEBUGPRINT
      Serial.printf("Free memory: %d\r\n", ESP.getFreeHeap());
      #else
      Serial.println(ESP.getFreeHeap());
      #endif
  }
  
  MDNS.update();
  ArduinoOTA.handle();
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
  
  numSensors = MDNS.queryService(SERVICE_NAME, "tcp");

  int infoStorageSize = sizeof(VespeneSensor) * numSensors;
  
  if(sensorData == NULL){
    sensorData = (VespeneSensor *)malloc(infoStorageSize); // if sensorData is NULL, allocate the memory. Else, reallocate with the pointer
  }else{
    realloc(sensorData, infoStorageSize);
  }
  
  WiFiClient client;

  totalSizeBytes = 1; // null terminator

  for(int i = 0; i<numSensors; i++){
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
          sensorData[i].sensorName = (char *)malloc(sensorNameLength);
          strcpy(sensorData[i].sensorName, nodeName);

          sensorData[i].temperature = variables_temperature;
          sensorData[i].humidity = variables_humidity;
          sensorData[i].location = (char *)malloc(sensorLocationLength);
          strcpy(sensorData[i].location, variables_location);

          totalSizeBytes += sensorNameLength + sensorLocationLength + sizeof(char) * 6 + sizeof(char) * 4; // allows for triple digit temperature and hunmidity, and separators
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
  Serial.printf("New string approximate length: %d\r\nNumber of sensors: %d\r\n", totalSizeBytes, numSensors);
  #endif
  if(sensorResponseString == NULL){
    sensorResponseString = (char *)malloc(totalSizeBytes + 1);
  }else{
    realloc(sensorResponseString, totalSizeBytes + 1); // allow for null pointer in worst case
  }
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

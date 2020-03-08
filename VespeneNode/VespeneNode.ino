#define PRINT_FREE_MEMORY

#include <ArduinoOTA.h>

#include <ESP8266mDNS.h>

#include <ESP8266WiFi.h>
#include <WiFiServer.h>

#include <aREST.h>

#include <Adafruit_Si7021.h>

#include <WiFiManager.h>

#define STATUS_LED 0 // pin that the status led is located on
#define LISTEN_PORT 80 // The port that the rest api will listen on
#define ENDPOINT_ID "4" // change to suit whatever is needed
#define ENDPOINT_NAME "VESPENE-004" // change to suit whatever is needed
#define RESTFUL_HOSTNAME "vespeneenv4" // change to suit whatever is needed
#define SENSOR_LOCATION "Room" // change to suit whatever is needed
#define SERVICE_NAME "vespene" // leave as default


//sensor things
int temperatureReading = -255;
int humidityReading = -255;

Adafruit_Si7021 sensor = Adafruit_Si7021();

//RESTful server things
aREST rest = aREST();

WiFiServer server(LISTEN_PORT);

#ifdef PRINT_FREE_MEMORY

uint32_t last_memory_check = 0;

#endif

void setup() {
  Serial.begin(115200);
  
  pinMode(STATUS_LED, OUTPUT);

  digitalWrite(STATUS_LED, LOW);

  //connect to wifi
  WiFiManager manager;
  manager.autoConnect();

  Serial.printf("WiFi connected!\r\nIP: %s\r\n", WiFi.localIP().toString().c_str());

  blinkTimes(1);

  delay(300);

  //initialize sensor
  if(!sensor.begin()){
    Serial.println("Sensor failed to initialize");
    errorBlink();
  }
  else{
    blinkTimes(2);
    Serial.println("Sensor initialized successfully");
  }

  delay(300);

  //setup RESTful API
  rest.variable("temperature", &temperatureReading);
  rest.variable("humidity", &humidityReading);
  rest.variable("location", &SENSOR_LOCATION);
  rest.set_id(ENDPOINT_ID);
  rest.set_name(ENDPOINT_NAME);
  Serial.println("RESTful API parameters set");

  blinkTimes(3);

  delay(300);

  //setup mDNS service

  if(!MDNS.begin(RESTFUL_HOSTNAME)){
    Serial.println("mDNS Service could not be started");
    errorBlink();
  }
  else{
    MDNS.addService(SERVICE_NAME, "tcp", 80);
    blinkTimes(4);
    Serial.println("mDNS service started");
  }

  delay(300);

  ArduinoOTA.begin();
  
  //start server

  server.begin();
  Serial.println("Server started");
}

void loop() {
  #ifdef PRINT_FREE_MEMORY
  
  if(millis() - last_memory_check > 5000){
    last_memory_check = millis();
    Serial.printf("%d bytes free\r\n", ESP.getFreeHeap());
  }
  
  #endif
  MDNS.update();
  ArduinoOTA.handle();
  WiFiClient client = server.available();
  if(!client){
    return;
  }
  while(!client.available()){
    delay(1);
  }
  temperatureReading = sensor.readTemperature();
  humidityReading = sensor.readHumidity();
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

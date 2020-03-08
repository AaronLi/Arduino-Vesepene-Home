# Arduino-Vesepene-Home
Wifi connected sensor nodes in Arduino with MDNS

## Files
---
* VespeneHub
  * The central hub where data flows in and out of, the standard VespeneHub is designed to run on an ESP8266 based Arduino will autodiscover nodes on the network

* VespeneHubESP32
  * A version of VespeneHub for use with the ESP32

* VespeneNode
  * A sensor Node based on the ESP8266 using the Si7021 Sensor. Will be autodiscovered by VespeneHubs on a properly configured WiFi network. Can also be directly accessed by IP or MDNS handle

* VespeneTap
  * For use on an ESP8266. Serves a webpage that can poll the VespeneHub and display its data.
  * The `data` directory must be uploaded to SPIFFS

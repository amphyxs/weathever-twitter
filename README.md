# Weathever
Meteostation Arduino sketch for ESP8266 that takes measurements from these sensors:

* DHT11 or DHT22 - Tempearature and humidity
* BME280         - Air pressure and altitude
* Photoresitior  - Light
* Rain Sensor    - Precipitation

Also, it uses 4051 analog multiplexor, cause ESP8266 has only one analog pin.
Data from the sensors is posted on Twitter using ThingSpeak API.

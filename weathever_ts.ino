// Weathever. Version 1.1
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>

// WiFi Network & ThingSpeak API (Twitter)
const String apiKey = "API KEY";
const char* server = "api.thingspeak.com"; 
const char* ssid = "WIFI SSID";  
const char* password = "WIFI PASSWORD";
WiFiClient client;

// Deep Sleep
const int STD_DELAY = 15000; // Delay for sensors measurements reading in microseconds
const int SLEEP_S = 900;
const int mS_TO_S = 1000000;

// NTP Client to get time
const long utcOffsetInSeconds = 3 * 60 * 60;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
const String months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Sensors
// * DHT22 on PIN D8
#define DHTTYPE DHT22
uint8_t dht_pin = D6;
DHT dht(dht_pin, DHTTYPE); 

// * Muliplexer on PINS D4(A), D3(B), D2(C) for analog sensors:
// ** Light Sensor(X0)
// ** Rain Sensor(X1) 
uint8_t mux_a = D5;
uint8_t mux_b = D4;
uint8_t mux_c = D3;
uint8_t rain_pin = A0;
uint8_t light_pin = A0;

// Main variables
float Hum = 0, Temp = 0, Light = 0, RainAnalog = 0, Press = 0, Alt = 0;
String Weather = "";

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Device is in wake up mode.");
  pinMode(dht_pin, INPUT);
  pinMode(mux_a, OUTPUT);
  pinMode(mux_b, OUTPUT);     
  pinMode(mux_c, OUTPUT); 
  dht.begin();      
  WiFi.disconnect();
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Successfully connected.");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  getMeasurements();
  uploadOnServer();
  Serial.println("Device is in sleep mode for " + (String)SLEEP_S + " seconds");
  ESP.deepSleep(SLEEP_S * mS_TO_S); 
}

void loop() { }

void change_Mux(int c, int b, int a) {
  digitalWrite(mux_a, a);
  digitalWrite(mux_b, b);
  digitalWrite(mux_c, c);
}

void getMeasurements() {
  Temp = dht.readTemperature();
  Hum = dht.readHumidity();
  change_Mux(LOW, LOW, LOW); // X0
  Light = analogRead(light_pin);
  change_Mux(LOW, LOW, HIGH); // X1
  RainAnalog = analogRead(rain_pin);
  RainAnalog = constrain(RainAnalog, 150, 440); 
  RainAnalog = map(RainAnalog, 150, 440, 1023, 0);
  Light = 1024 - Light; 
  if(RainAnalog >= 20 && Temp < 0) {
    Weather = "Snowy"; 
  }
  else if(RainAnalog >= 20 && Temp >= 0) {
    Weather = "Rainy"; 
  }
  else if(RainAnalog >= 20 && Light < 200) {
    Weather = "Rainy Night";
  }
  else if(RainAnalog < 20 && Light >= 200 && Light <= 400) {
    Weather = "Cloudy";
  }
  else if(RainAnalog < 20 && Light >= 400) {
    Weather = "Sunny";
  }
  else if(RainAnalog <= 20 && Light < 200) {
    Weather = "Night";
  }
  else if(Hum >= 98 && Light < 700 && Light > 500) {
    Weather = "Foggy";
  }
}

void uploadOnServer() {
  if(client.connect(server,80)) {
     String date = getDate();
     String s = "Current weather conditions on " + date + ":\n" + "Temperature: " + String((int)Temp) + " Celsius degrees\n" + "Humidity: " + String((int)Hum) + " percents\n" + "Weahter conditions: " + Weather; 
     String tsData = "api_key="+ apiKey +"&status=" + s;
     client.print("POST /apps/thingtweet/1/statuses/update HTTP/1.1\n");
     client.print("Host: api.thingspeak.com\n");
     client.print("Connection: close\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(tsData.length());
     client.print("\n\n");
     client.print(tsData);
     Serial.println("Successfully uploaded data on server.");
  }
  client.stop();
  delay(STD_DELAY);
}

String getDate() {
  timeClient.update();  
  String format_date = String(timeClient.getFormattedDate());
  int splitT = format_date.indexOf("T");
  String month_idx_s = format_date.substring(5, 7);
  if(month_idx_s[0] == '0') month_idx_s = month_idx_s[1];
  int month_idx = month_idx_s.toInt();
  String month = months[month_idx-1];
  String day = format_date.substring(8, 10);
  if(day[0] == '0') day = day[1];
  String stime = format_date.substring(splitT+1, format_date.length()-4);
  return month + ", " + day + " " + stime;
}
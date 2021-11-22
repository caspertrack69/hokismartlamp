#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include "FS.h"
#include <DNSServer.h>
#include <ArduinoJson.h>

#include "index_html.h"
#include "sw_js.h"
#include "style_css.h"
#include "manifest_json.h"

#define NUM_PIX 16
const char* ssid     = "Hoki_SmartLamp";
//const char* password = "Hoki_SmartLamp";
unsigned char red,green,blue,bright;
String wifi_ssid,wifi_password,wifi_status;
bool power = true;

char output[128];
char readPower[128];
char setWifi[128];

//StaticJsonBuffer<200> json_data;
DynamicJsonDocument json_data(200);
DynamicJsonDocument json_power(200);
DynamicJsonDocument json_wifi(200);
 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIX, D4, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);             // Mode AP/Hotspot
  WiFi.softAP(ssid);
  wifi_status = "disconnected";
  Serial.println(wifi_status);
  Serial.print("set");
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  
  red=255;
  green=255;
  blue=255;
  bright=100;
  
  server.on("/", [](){    
    server.send(200, "text/html", index_html);
  });
  server.on("/sw.js", [](){
    server.send_P(200, "application/javascript", sw_js);
  });
  server.on("/manifest.json", [](){
    server.send(200, "application/json", manifest_json);
  });  
  server.on("/app.js", [](){
    server.send(200, "application/javascript", app_js);
  });  
  server.on("/data", HTTP_GET, [](){
    setJson(red, green, blue, bright);
    
    server.send(200, "application/json", output);
  });
  server.on("/power", HTTP_GET, [](){
    json_power["power"] = power;
    serializeJson(json_power, readPower);
    
    server.send(200, "application/json", readPower);
  });
  server.on("/wifi", HTTP_GET, [](){
    json_wifi["ssid"] = wifi_ssid;
    json_wifi["password"] = wifi_password;
    json_wifi["status"] = wifi_status;
    serializeJson(json_wifi, setWifi);
    
    server.send(200, "application/json", setWifi);
  });
  server.on("/data/set", HTTP_POST, [](){
    String RMsg = server.arg("red");
    String GMsg = server.arg("green"); 
    String BMsg = server.arg("blue");
    String brightMsg=server.arg("bright");
    
    if( !server.hasArg("red") || RMsg == NULL) {
      server.send(400, "text/plain", "400: Invalid Request");
      return;
    } else {
      red = RMsg.toInt();
      green = GMsg.toInt(); 
      blue = BMsg.toInt();
      bright = brightMsg.toInt();
      setJson(red, green, blue, bright);
      toFirstPage();
    }
  });
  server.on("/power/set", HTTP_POST, [](){
    String powerMsg=server.arg("power");
    
    if( !server.hasArg("power") || powerMsg == NULL) {
      server.send(400, "text/plain", "400: Invalid Request");
      return;
    } else {
      power = (powerMsg == "false") ? false : true;
      toFirstPage();
    }
  });
  server.on("/wifi/set", HTTP_POST, [](){
    String ssidMsg=server.arg("ssid");
    String passwordMsg=server.arg("password");
    
    if( !server.hasArg("ssid") || ssidMsg == NULL) {
      server.send(400, "text/plain", "400: Invalid Request");
      return;
    } else {
      wifi_ssid = ssidMsg;
      wifi_password = passwordMsg;
      toFirstPage();

      WiFi.begin(wifi_ssid, wifi_password);
      int second = 0;
      int stats;
      while(WiFi.status() != WL_CONNECTED) {
        stats = WiFi.status();
        Serial.println(stats);
        wifi_status = "connecting";
        Serial.println(wifi_status);
        delay(1000);
        second++;
        if (stats == 1 || second >= 10) {
          toFirstPage();
          break;
        }
      }
      Serial.println(stats);
      if (stats == 1 || second >= 10) {
        wifi_status = "disconnected";
      } else {
        wifi_status = "connected";
      }
      Serial.println(wifi_status);
    }
  });
  server.on("/favicon.ico", HTTP_GET, []() {
    handleFileRead("/favicon.ico", "image/png");
  });
  server.on("/img/icons-192.png", HTTP_GET, []() {
    handleFileRead("/icons-192.png", "image/png");
  });
  server.on("/img/icons-512.png", HTTP_GET, []() {
    handleFileRead("/icons-512.png", "image/png");
  });
  server.begin(); 
  pixels.begin();
  pixels.setBrightness(bright);
}

void loop() {
  server.handleClient(); 

  for(int i=0;i<NUM_PIX;i++){
    pixels.setPixelColor(i, pixels.Color(red,green,blue));
    pixels.show();
  }
  
  if(!power) {
    pixels.setBrightness(0);
//    Serial.println("off");
  } else {
    pixels.setBrightness(bright);
//    Serial.println("on");
  }
  delay(500);
}

void setJson(int r, int g, int b, int a) {
  json_data["red"] = r;
  json_data["green"] = g;
  json_data["blue"] = b;
  json_data["bright"] = a;
  
  serializeJson(json_data, output);
}

bool handleFileRead(String path, String type) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, type);
    server.send(200);
    file.close();
    return true;
  }
  return false;
}

void toFirstPage() {
  String redirect = "<html><script>window.location.href = \"/\";</script></html>";
  server.send(200, "text/html", redirect);
}

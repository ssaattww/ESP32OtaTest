#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h> // NVS
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <string>
#include <esp32fota.h>

#include "html.hpp"

esp32FOTA FOTA;
SemverClass semver(0,0,0);

Preferences preferences;
AsyncWebServer server(80);

char ap_ssid[] = "M5StackCore2";
char ap_pass[] = "password";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

std::string ssidStr = "";
std::string pwdStr = "";
bool connected = false;

bool tryConnect(){
  // if(strcmp(ssid,ssidStr.c_str())==0) Serial.println("ssid compared");
  // else Serial.println("ssid not compared");
  // if(strcmp(pwd,pwdStr.c_str())==0) Serial.println("pwd compared");
  // else Serial.println("pwd not compared");

  delay(100);
  
  // WiFi.begin(ssid, pwd);
  WiFi.begin(ssidStr.c_str(), pwdStr.c_str());
  
  int retryCount = 0;
  Serial.printf("try connect to %s\n",ssidStr.c_str());
  
  // while (WiFi.status() != WL_CONNECTED && retryCount<40) {
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      retryCount++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.printf("wifi connected\n");
    String tmps1 = String(ssidStr.c_str());
    String tmps2 = String(pwdStr.c_str());
    preferences.putString("ssid", tmps1);
    preferences.putString("pwd", tmps2);
    connected = true;
    return true;
  } 
  else{
    Serial.printf("wifi connect failed\n");
    ssidStr = "";
    pwdStr = "";
    return false;
  };
}

void my_progress_callback( size_t progress, size_t size )
{
  if( progress == size || progress == 0 ) Serial.println();
  Serial.printf("%d/%d        \r",progress,size);
}

void setupWiFi(){
  bool con =false;
  preferences.begin("my-app",false);
  ssidStr = preferences.getString("ssid","").c_str();
  pwdStr  = preferences.getString("pwd","").c_str();
  const char *ssid = ssidStr.c_str();
  const char *pwd  = pwdStr.c_str();

  WiFi.mode(WIFI_AP_STA); //AP mode and STA mode
  WiFi.softAP(ap_ssid, ap_pass);
  
  if(ssidStr.length() != 0){
    con = tryConnect();
  }

  if(!con){
    M5.Lcd.qrcode("http://" + WiFi.softAPIP().toString(),10,10,110,6);
    M5.Lcd.setCursor(0,150);
    M5.Lcd.printf("please connect \nssid:%s\npw:%s\n",ap_ssid,ap_pass);
    Serial.println("show qr code for input ssid and pw");
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
      Serial.println("main page");
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", loginIndex.c_str());
      response->addHeader("charset","UTF-8");
      request->send(response);
      // request->send_P(200, "text/html", loginIndex.c_str());
    });
    
    server.on("/post", HTTP_POST, [&](AsyncWebServerRequest *request){
      String message1 = "";
      String message2 = "";
      String param1 = "";
      String param2 = "";

      if(request->hasParam("ssid",true,false)){
        param1 = "ssid";
        message1 = request->getParam("ssid",true,false)->value();
        ssidStr = message1.c_str();
        Serial.printf("ssid:%s\n",message1.c_str());
      }
      if(request->hasParam("pwd",true,false)){
        param2 = ",pwd";
        message2 = request->getParam("pwd",true,false)->value();
        pwdStr = message2.c_str();

        Serial.printf("pwd:%s\n",message2.c_str());
      }
      request->send(200, "text/html", "HTTP post request sent to your ESP on input field (" 
                                + param1 + param2 +") with value: " + message1 + "," + message2 +
                                "<br><a href=\"/\">Return to Home Page</a>");
    });
    server.onNotFound(notFound);
    server.begin();
    Serial.println("server begin");
  }
  else{
    server.end();
  }
}

void setupFota(){
  auto cfg = FOTA.getConfig();
  cfg.name         = "esp32-fota-http";
  cfg.manifest_url = "https://ssaattww.github.io/ESP32OtaTest/test.json?aaa";
  cfg.sem          = semver;
  cfg.check_sig    = false;
  cfg.unsafe       = true;
  FOTA.setConfig(cfg);
  FOTA.setProgressCb(my_progress_callback);
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  setupWiFi();
  setupFota();
}

void loop() {
  static bool updated = false;

  if(!connected){
    // connectedではないときにはwebサーバーが立ち上がっている想定
    // wifiがつながっていない時
    if(ssidStr.length()!=0){
      tryConnect();
    }
    if(connected){
      server.end();
    }
  }
  else{
    // wifiとつながっている時
    if(!updated){
      bool updatedNeeded = FOTA.execHTTPcheck();
      if(!updatedNeeded){
        updated = true;
      }
      else{
        FOTA.execOTA();
        updated = true;
      }
      Serial.printf("version %d,%d,%d\n", semver.ver()->major,semver.ver()->minor,semver.ver()->patch);
    }
    else{
      delay(10);
    }
  }

  delay(10);
}
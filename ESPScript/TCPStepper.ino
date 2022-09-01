#include <EEPROM.h>
#include "ArduinoJson.h"
#include "site.h"
#include <ElegantOTA.h>
#include "dinamicStruct.h"
#include <vector>
#include "conf.h"

#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer serverHTTP(80);
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer serverHTTP(80);
#else
        #error "Please check your mode setting,it must be esp8266 or esp32."
#endif
extern FastAccelStepperEngine engine;
extern FastAccelStepper *stepper[MAX_STEPPER];


#define aIP 20
#define aGATEWAY 35
#define aSUBNET 51
#define aSSID 66
#define aPASW 96

#define MaxSpeed 1000


WiFiServer server(8888);

uint32_t getChipID(){
  uint32_t chipId = 0;
  
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  return chipId;
}

const char* ssid = String(getChipID(), HEX).c_str();  // для SSID точки доступа

const char* password = "12345678";  // для пароля к точке доступа

struct  confList{
  String SSID, PASW;
  IPAddress ip;
  IPAddress gateway;
  IPAddress subnet;
} tempList;


uint32_t Timer; // таймер подключения к wi-fi

StaticJsonDocument<384> doc;   //Пул памяти

StaticJsonDocument<768> data;


struct EvClient {
  WiFiClient client;
  uint8_t event;
};

void ClientTCP(EvClient &ProcessingClient);
void ClientDestinationTCP(WiFiClient& client, uint8_t *event);

std::vector<EvClient> event_client;

std::map <String, uint8_t> command = {
  {"/actions/start", 1}, {"/actions/stop", 2},
  {"/actions/state", 3}, {"/service/settings/network", 4}, 
  {"/service/settings/network/value", 5},
  {"/service/reload", 6}, {"/events", 7}, {"/service/info", 8}
};

std::map <String, uint8_t> destination  = {
  {"/actions/state", 1}, {"/service/info", 2},
  {"/service/settings/network/value", 3}
};

// arduino  default____________________________________________________________
void setup() {
  Serial.begin(115200);
  EEPROM.begin(1024);
  Serial.println();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("SSID " + String(ssid));
  Serial.println(WiFi.macAddress());
  Serial.println("local host:");
  Serial.print("http://");
  Serial.println(WiFi.softAPIP());

  // /* для очиски EEPROM (когда ничего не помогает :) )) */
  // for(uint8_t i = 0; i < 100; i++){
  //   EEPROM.write(i, 0);
  // }
  // EEPROM.commit();
  // обьявляем шаговики
  for(int i = 0; i < 8; i++){
     FastAccelStepper *s = NULL;
        s = engine.stepperConnectToPin(stepPin[i]);
        if(s){
          s->setEnablePin(sleepPin[i / 2], false);
          s->setAutoEnable(true);
    }
    stepper[i] = s;
  }
  
  tempList = GetEeprom();
  newWifi(&tempList);
  serverHTTP.on("/", handleRoot);
  serverHTTP.on("/wifiSet", setupWiFi);
  serverHTTP.begin();
  ElegantOTA.begin(&serverHTTP);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP and TCP server started");

  engine.init();
}

void loop() {
  whileList(); // для проверки заданий на двигателях
  serverHTTP.handleClient();
  WiFiClient client = server.available();
  if (client){
    if(client.connected())
    {
      EvClient NewClient;
      NewClient.client = client;
      NewClient.event = 0;
      Serial.println("Client Connected");
      event_client.push_back(NewClient);
    }

    Serial.println();
  } 
  for(uint8_t i = 0; i < event_client.size(); i++){
    if(event_client.at(i).client.connected()){
      if(event_client.at(i).event == 0){ // проверяем что хочет клинет
        ClientTCP(event_client.at(i));
      }else{ // проверяем какой distination ему отправить
        if(millis() - Timer > 1000){
          uint8_t event = event_client.at(i).event;
          switch(event){  
            case 4:{
              event = 1;
              break;
            }
            case 5:{
              event = 2;
              break;
            }
            case 6:{
              event = 3;
              break;
            }
            case 7:{
              event_client.at(i).event = 4;
              event = 1;
              break;
            }
            default:{
              event_client.at(i).event--;
            }
          }
          event_client.at(i).event++;
          ClientDestinationTCP(event_client.at(i).client, &event);
        }
      }
    }else{ // клиент отключился
      Serial.println("Client disconnected"); 
      event_client.at(i).client.stop();
      event_client.erase(event_client.begin() + i);
    }
  }
  if(millis() - Timer > 1000){
    Timer = millis();
  }
}

// функция для отправки событий
void ClientDestinationTCP(WiFiClient& client, uint8_t *event){
  data.clear();
  String event_out = "{\"path\":";
  String su;
  switch ((*event)){
    case 1:{
      event_out += "\"/actions/state\",\"values\":[";
      su += stateJSON();
      break;
    }
    case 2:{
      event_out += "\"/service/info\",";
      su += infoJSON();
      break;
    }
    case 3:{
      event_out += "\"/service/settings/network/value\",";
      su += valueJSON();
      break;
    }
  }
  su.remove(0, 1);

  event_out += su;
  if(event_out[event_out.length() - 1] != '}')
    event_out += "}";
  Serial.println("SENT destination:");
  Serial.println(event_out);
  client.print(event_out);
  data.clear();
}

void ClientTCP(EvClient &ProcessingClient){
  String JSONMessage;
  while(ProcessingClient.client.available()>0){
    whileList();
      JSONMessage = ProcessingClient.client.readStringUntil('\n');
      Serial.println(JSONMessage);
      DeserializationError error = deserializeJson(doc, JSONMessage);
      Serial.println("GET:");
      serializeJson(doc, Serial);
    if (error) {
        Serial.print(F("deserializeJson() failed with code: "));
        Serial.println(error.c_str());
    }

    String outS;
    switch (command[doc["path"]]){
      case 1:{
        // "/actions/start"
        uint8_t moto = doc["motor"];
        if(NoMotor(&moto)){
          uint32_t MSpeed = doc["velocity"];
          int32_t MStep = doc["steps"];
          int32_t MAcceleration = doc["acceleration"];
          remove(&moto);
          add(&moto, &MStep, &MSpeed, &MAcceleration);
          data["message"] = "OK";
        }
        break;
      }
      case 2:{
        // "/actions/stop"
        uint8_t moto = doc["motor"];
        if(NoMotor(&moto)){
          if(find(&moto)){
            remove(&moto);
            data["message"] = "OK";
          }else{
            data["error"] = "motor not started";
          }
        }
        break;
      }
      case 3:{
        // "/actions/state"
        stateJSON();
        break;
      }
      case 4:{
        // "/service/settings/network",
        struct confList temp;
        temp.SSID = doc["ssid"].as<String>();
        Serial.println(doc["ssid"].as<String>());
        temp.PASW = doc["password"].as<String>();
        temp.ip.fromString(doc["ip"].as<String>());
        temp.gateway.fromString(doc["gateway"].as<String>());
        temp.subnet.fromString(doc["mask"].as<String>());
        ProcessingClient.client.print("{\"message\":\"Connecting\"}");
        MyDelay();
        if(newWifi(&temp)){
          Serial.println("Save conf");
          tempList = temp;
          SaveEeprom(&tempList);
        }else
        Serial.println("Error");
          data["error"] = "Disconnet";
          connectingWiFi();
        break;
      }
      case 5:{
        // "/service/settings/network/value"
        valueJSON();
        break;
      }
      case 6:{
        // "/service/reload"
        Serial.println("SENT:");
        data["message"] = "OK";
        ProcessingClient.client.print("{\"message\":\"OK\"}");
        serializeJson(data, Serial);
        Serial.println("restart");
        ProcessingClient.client.stop();
        Serial.flush();
        // SPIFFS.end();
        ESP.restart();
        break;
      }
      case 7:{
        // "events"
        ProcessingClient.event = destination[doc["destination"]];
        Serial.println();
        Serial.println("event = ");
        Serial.println(ProcessingClient.event);
        if(!ProcessingClient.event){
          ProcessingClient.event = 4;
        }
        Serial.println();
        Serial.println("event = ");
        Serial.println(ProcessingClient.event);
        break;
      }
      case 8:{
        infoJSON();
        break;
      }
      default:
        data["error"] = "no command";
        break;
    }
    Serial.println();

    serializeJson(data, outS);
    if(!outS.isEmpty() && ProcessingClient.event == 0){
      Serial.println("SENT:");
      serializeJson(data, Serial);
      ProcessingClient.client.print(outS);
    }
    data.clear();
  }
}
// ____________________________________________________________________________

// Wifi set____________________________________________________________________
boolean newWifi(struct confList *temp){
  WiFi.disconnect();
  WiFi.config(temp->ip, temp->gateway, temp->subnet);
  WiFi.begin(temp->SSID.c_str(), temp->PASW.c_str());
  Serial.println();
  Serial.print("Connecting ");
  Serial.println(temp->SSID);
  Timer = millis();
  while (WiFi.status() != 3 && millis() - Timer < 5000) {
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status() != 3){
    Serial.println();
    Serial.println("Disconnet");
    return false;
  }
  Serial.println();
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  tempList.ip = WiFi.localIP();
  tempList.gateway = WiFi.gatewayIP();
  tempList.subnet = WiFi.subnetMask();

  return true;
}

void connectingWiFi() {
  if(!newWifi(&tempList)){
    Serial.println();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("http://");
    Serial.println(WiFi.softAPIP());
  }
  Serial.println();
  Serial.print("Gateway:");
  Serial.println(WiFi.gatewayIP());
  Serial.println();
}
// _____________________________________________________________________


// HTTP server_________________________________________________________________
void setupWiFi() {
  confList tempWeb = {};
  tempWeb.SSID = serverHTTP.arg("name");
  tempWeb.PASW = serverHTTP.arg("password");
  tempWeb.ip.fromString(serverHTTP.arg("ip"));
  tempWeb.gateway.fromString(serverHTTP.arg("gateway"));
  tempWeb.subnet.fromString(serverHTTP.arg("subnet"));
  if(newWifi(&tempWeb)){
    serverHTTP.send(200, "text/html", "Сonnected");
    Serial.println("Connect");
    // сохраняем данные
    tempList = tempWeb;
    SaveEeprom(&tempList);
    Serial.flush();
    ESP.restart();
  }else{
    serverHTTP.send(404, "text/html", "Error connection");
    connectingWiFi();
  }
}

void handleRoot(){
   String s = html_1;
   serverHTTP.send(200, "text/html", s);
}
//_____________________________________________________________________________


// EEPROM______________________________________________________________________
// сохраняем данные
void SaveEeprom(struct confList *temp){
  set_String(aIP, temp->ip.toString());
  set_String(aGATEWAY, temp->gateway.toString());
  set_String(aSUBNET, temp->subnet.toString());
  set_String(aSSID, temp->SSID);
  set_String(aPASW, temp->PASW);
}
// читаем
confList GetEeprom(){
  struct confList temp;
  temp.ip.fromString(get_String(aIP));
  temp.gateway.fromString(get_String(aGATEWAY));
  temp.subnet.fromString(get_String(aSUBNET));
  Serial.println(get_String(aIP));
  Serial.println(get_String(aGATEWAY));
  Serial.println(get_String(aSUBNET));
  temp.SSID = get_String(aSSID);
  temp.PASW = get_String(aPASW);
  Serial.println(temp.SSID);
  Serial.println(temp.PASW);
  return temp;
}


// a записываем длину строки, b - начальный бит, str - строка, которую нужно сохранить
void set_String(int a, String str){
  #if defined(ESP32)   
    EEPROM.writeString(a, str);
  #elif defined(ESP8266)
    EEPROM.write(a, str.length());// EEPROM бит a, записываем длину строки str
    // Сохраняем все данные str в EEPROM по одному
    a++;
    for (uint8_t i = 0; i < str.length(); i++){
      EEPROM.write(a + i, str[i]);
    }
  #endif
  EEPROM.commit();
}

// бит - длина строки, b - начальный бит
String get_String(int a){ 
  String data = "";
  #if defined(ESP32)
    data = EEPROM.readString(a);
  #elif defined(ESP8266)
    uint8_t b = EEPROM.read(a);
    // Извлекаем значение каждого бита по одному из EEPROM и связываем
    for (uint8_t i = 1; i <= b; i++){
      data += char(EEPROM.read(a + i));
    }
  #endif
  return data;
}
//_____________________________________________________________________________


// Сommand processing__________________________________________________________

void MyDelay(){
  Timer = millis();
  while (millis() - Timer < 1000){
    whileList();
  }
}

bool NoMotor(uint8_t* m){
  if((*m) >= 0 && (*m) <= 8){ 
    return true;
  }else{
    data["error"] = "Engine " + String(*m) + " not found";
  }
  return false;
}
 

String stateJSON(){
  JsonArray number = data.createNestedArray("values");
  for(uint8_t i = 0; i < 8;  i++){
    if(Node *pkey = find(&i)){
      number[i]["input_steps"] = pkey->MStep;
      number[i]["out_steps"] = pkey->s->getPositionAfterCommandsCompleted();
      number[i]["velocity"] = pkey->s->getSpeedInMilliHz() / 1000;
      number[i]["acceleration"] = pkey->s->getAcceleration();
    }else{
      number[i]["input_steps"] = 0;
      number[i]["out_steps"] = 0;
      number[i]["velocity"] = 0;
      number[i]["acceleration"] = 0;
    }
  }
  String su;
  serializeJson(number, su);
  return su;
}


String infoJSON(){
  // /service/info
  data["network_sig"] = WiFi.RSSI();
  data["mac"] = WiFi.macAddress();
  data["ip"] = tempList.ip;
  String su;
  serializeJson(data, su);
  // s += su;
  return su;
}

String valueJSON(){
  data["ssid"] = tempList.SSID;
  data["ip"] = tempList.ip;
  data["gateway"] = tempList.gateway;
  data["subnet"] = tempList.subnet;
  String su;
  serializeJson(data, su);
  // s += su;
  return su;
}
//_____________________________________________________________________________
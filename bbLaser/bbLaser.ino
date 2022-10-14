#include <WiFi.h>
#include <vector>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <AsyncTCP.h> //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int kppsTime = 1000000 / (20 * 1000);
volatile unsigned long timeOld;

volatile unsigned long timeStart;
// ================= Streaming -_,- =========================//
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  //单帧
  if (info->final && info->index == 0 && info->len == len) {
    handleStream(data, len, 0, info->len);
  }
  //多帧
  else {
    if (info->index == 0) {
      if (info->num == 0)
        //Serial.println("MSG Start");
        Serial.println("Frame Start");
        //handleStream(data, len, 0, info->len);
    }
    //Serial.print(info->index);
    //Serial.print(" ");
    //Serial.println(len);
    if ((info->index + len) == info->len) {
      //Serial.println("Frame End");
      if (info->final) {
        //Serial.println("MSG End");
        //Serial.println(frameLen);
        handleStream(data, len, info->index, info->len);
      }
    }
    else{
      handleStream(data, len, info->index, info->len);
    }
  }

  /*
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    /*
        StaticJsonDocument<100> pdoc;
        DeserializationError err = deserializeJson(pdoc, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }
        //Serial.println(count);
        //Serial.println(pdoc["x"].as<float>());
    }
    else{

    }*/
}

bool isStreaming = false;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    //client connected
    ESP_LOGI("ws[%s][%u] connect\n", server->url(), client->id());
    //client->printf("I am bbLaser :)", client->id());
    //client->ping();
    isStreaming = true;
  } else if (type == WS_EVT_DISCONNECT) {
    //client disconnecteds
    ESP_LOGI("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    isStreaming = false;
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);

  }
}



void setup() {

  
  
  Serial.begin(115200);
  setupSD();


  WiFi.begin("Hollyshit_A", "00197633");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->redirect("https://www.bbrealm.com/bblaser/?ip=" + WiFi.localIP().toString());
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();


  setupRenderer();
  
  attachInterrupt(22,goPrev,FALLING);
  attachInterrupt(21,goNext,FALLING);
  attachInterrupt(15,changeAutoNext,CHANGE);

  Serial.println(kppsTime);
}

void loop() {
  // put your main code here, to run repeatedly:

  if (micros() - timeOld >= kppsTime) {
    timeOld = micros();
    draw_task();
  }
}
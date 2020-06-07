#define ESP32

#include <Arduino.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define FL_PWM_PIN 2
#define FL_IN1_PIN 4
#define FL_IN2_PIN 16
#define RL_PWM_PIN 17
#define RL_IN1_PIN 21
#define RL_IN2_PIN 22
#define FR_PWM_PIN 14
#define FR_IN1_PIN 27
#define FR_IN2_PIN 26
#define RR_PWM_PIN 25
#define RR_IN1_PIN 33
#define RR_IN2_PIN 32

#define FR_CHANNEL 1
#define FL_CHANNEL 2
#define RR_CHANNEL 3
#define RL_CHANNEL 4

#define CAM_IP_LEN 40

const char *ssid = "ERIC4WD";
const char *password = "1q2w3e4r";
int CONNECTED = 0;
int outputPower = 0;
int turnPower = 0;
int forwardStatus = 1;
int feq = 5000;
int dutyCycle = 210;
int continueOutput = false;
int stoped = true;
//640 - 850
int minPower = 640;
char camip[CAM_IP_LEN];

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");           // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)


void processTextCmd(String cmd,AsyncWebSocketClient *client)
{
  Serial.println("processTextCmd");
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, cmd);
  JsonObject obj = doc.as<JsonObject>();
  int txnId = obj[String("txnid")].as<int>();
  switch(txnId){
    case 1:
      carControl(obj);
      client->text("Command executed");
      break;
    default:
      break;
  }
}

void carControl(const JsonObject obj){
  
  int distance = obj[String("distance")];
  String direction = obj[String("direction")].as<String>();
  if (direction == "up")
  {
    if (distance > 0)
    {
      outputPower = map(distance, 0, 50, 0, 1024);
      if(outputPower < minPower)
        outputPower = minPower;
      continueOutput = true;
      forward();
      enPower();
    }
    else
    {
      continueOutput = false;
      stopCar();
    }
  }
  else if (direction == "down")
  {
    if (distance > 0)
    {
      outputPower = map(distance, 0, 50, 0, 1024);
      if(outputPower < minPower)
        outputPower = minPower;
      continueOutput = true;
      goBack();
      enPower();
    }
    else
    {
      continueOutput = false;
      stopCar();
    }
  }
  else if (direction == "right")
  {
    if (distance > 0)
    {
      //if is not forwarding or going back,  then use this distance
      if (continueOutput != 1)
      {
        outputPower = map(distance, 0, 50, 0, 1024);
        if(outputPower < minPower)
          outputPower = minPower;
      }
      goRight();
      right();
    }
    else
    {
      if (forwardStatus == 1)
      {
        forward();
      }
      else
      {
        goBack();
      }
      if (continueOutput)
      {
        enPower();
      }
      else
      {
        stopCar();
      }
    }
  }
  else if (direction == "left")
  {
    if (distance > 0)
    {
      //if is not forwarding or going back,  then use this distance
      if (continueOutput != 1)
      {
        outputPower = map(distance, 0, 50, 0, 1024);
        if(outputPower < minPower)
          outputPower = minPower;
      }
      goLeft();
      left();
    }
    else
    {
      if (forwardStatus == 1)
      {
        forward();
      }
      else
      {
        goBack();
      }
      if (continueOutput)
      {
        enPower();
      }
      else
      {
        stopCar();
      }
    }
  }else if(direction == "spinRight"){
    if (distance > 0)
    {
      outputPower = map(distance, 0, 50, 0, 1024);
      if(outputPower < minPower)
        outputPower = minPower;
      forward(); //先回到前進狀態
      spinRight();
      enPower();
    }
    else
    {
      stopCar();
    }
  }else if(direction == "spinLeft"){
    if (distance > 0)
    {
      outputPower = map(distance, 0, 50, 0, 1024);
      if(outputPower < minPower)
        outputPower = minPower;
      forward(); //先回到前進狀態
      spinLeft();
      enPower();
    }
    else
    {
      stopCar();
    }
  }
}

void onRequest(AsyncWebServerRequest *request)
{
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  //Handle body
}


void stopCar()
{
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(4, 0);
  stoped = true;
}

void enPower()
{
  int p1 = map(outputPower, 0, 1023, 0, 255);
  Serial.print("output power level = ");
  Serial.println(p1);
  ledcWrite(1, p1);
  ledcWrite(2, p1);
  ledcWrite(3, p1);
  ledcWrite(4, p1);
  stoped = false;
}

void forward()
{
  forwardStatus = 1;
  digitalWrite(FR_IN1_PIN, HIGH);
  digitalWrite(FR_IN2_PIN, LOW);
  digitalWrite(FL_IN1_PIN, HIGH);
  digitalWrite(FL_IN2_PIN, LOW);
  digitalWrite(RR_IN1_PIN, HIGH);
  digitalWrite(RR_IN2_PIN, LOW);
  digitalWrite(RL_IN1_PIN, HIGH);
  digitalWrite(RL_IN2_PIN, LOW);
}

void goBack()
{
  forwardStatus = 0;
  digitalWrite(FR_IN1_PIN, LOW);
  digitalWrite(FR_IN2_PIN, HIGH);
  digitalWrite(FL_IN1_PIN, LOW);
  digitalWrite(FL_IN2_PIN, HIGH);
  digitalWrite(RR_IN1_PIN, LOW);
  digitalWrite(RR_IN2_PIN, HIGH);
  digitalWrite(RL_IN1_PIN, LOW);
  digitalWrite(RL_IN2_PIN, HIGH);
}

void goRight()
{
  //如果是前進狀態，停右前輪逆轉右後輪
  //如果是倒退狀態，停右後輪逆轉右前輪
  if (forwardStatus == 1)
  {
    digitalWrite(FR_IN1_PIN, HIGH);
    digitalWrite(FR_IN2_PIN, LOW);
    digitalWrite(FL_IN1_PIN, HIGH);
    digitalWrite(FL_IN2_PIN, LOW);
    digitalWrite(RR_IN1_PIN, LOW);
    digitalWrite(RR_IN2_PIN, HIGH);
    digitalWrite(RL_IN1_PIN, HIGH);
    digitalWrite(RL_IN2_PIN, LOW);
  }
  else
  {
    digitalWrite(FR_IN1_PIN, HIGH);
    digitalWrite(FR_IN2_PIN, LOW);
    digitalWrite(FL_IN1_PIN, LOW);
    digitalWrite(FL_IN2_PIN, HIGH);
    digitalWrite(RR_IN1_PIN, LOW);
    digitalWrite(RR_IN2_PIN, HIGH);
    digitalWrite(RL_IN1_PIN, LOW);
    digitalWrite(RL_IN2_PIN, HIGH);
  }
}

void goLeft()
{
  //如果是前進狀態，停左前輪逆轉左後輪
  //如果是倒退狀態，停左後輪逆轉左前輪
  if (forwardStatus == 1)
  {
    digitalWrite(FR_IN1_PIN, HIGH);
    digitalWrite(FR_IN2_PIN, LOW);
    digitalWrite(FL_IN1_PIN, HIGH);
    digitalWrite(FL_IN2_PIN, LOW);
    digitalWrite(RR_IN1_PIN, HIGH);
    digitalWrite(RR_IN2_PIN, LOW);
    digitalWrite(RL_IN1_PIN, LOW);
    digitalWrite(RL_IN2_PIN, HIGH);
  }
  else
  {
    digitalWrite(FR_IN1_PIN, LOW);
    digitalWrite(FR_IN2_PIN, HIGH);
    digitalWrite(FL_IN1_PIN, HIGH);
    digitalWrite(FL_IN2_PIN, LOW);
    digitalWrite(RR_IN1_PIN, LOW);
    digitalWrite(RR_IN2_PIN, HIGH);
    digitalWrite(RL_IN1_PIN, LOW);
    digitalWrite(RL_IN2_PIN, HIGH);
  }
}

void right()
{
  int p1 = map(outputPower, 0, 1023, 0, 255);
  int p2 = map(turnPower, 0, 1023, 0, 255);
  //如果是前進狀態，停右前輪
  //如果是倒退狀態，停右後輪
  if (forwardStatus == 1)
  {
    ledcWrite(FR_CHANNEL, 0);
    ledcWrite(RR_CHANNEL, p2);
    ledcWrite(FL_CHANNEL, p1);
    ledcWrite(RL_CHANNEL, p1);
  }
  else
  {
    ledcWrite(FR_CHANNEL, p2);
    ledcWrite(RR_CHANNEL, 0);
    ledcWrite(FL_CHANNEL, p1);
    ledcWrite(RL_CHANNEL, p1);
  }
}

void left()
{
  int p1 = map(outputPower, 0, 1023, 0, 255);
  int p2 = map(turnPower, 0, 1023, 0, 255);
  //如果是前進狀態，停左前輪
  //如果是倒退狀態，停左後輪
  if (forwardStatus == 1)
  {
    ledcWrite(FR_CHANNEL, p1);
    ledcWrite(RR_CHANNEL, p1);
    ledcWrite(FL_CHANNEL, 0);
    ledcWrite(RL_CHANNEL, p2);
  }
  else
  {
    ledcWrite(FR_CHANNEL, p1);
    ledcWrite(RR_CHANNEL, p1);
    ledcWrite(FL_CHANNEL, p2);
    ledcWrite(RL_CHANNEL, 0);
  }
}

void spinRight()
{
  digitalWrite(FR_IN1_PIN, LOW);
  digitalWrite(FR_IN2_PIN, HIGH);
  digitalWrite(RR_IN1_PIN, LOW);
  digitalWrite(RR_IN2_PIN, HIGH);
}

void spinLeft()
{
  digitalWrite(FL_IN1_PIN, LOW);
  digitalWrite(FL_IN2_PIN, HIGH);
  digitalWrite(RL_IN1_PIN, LOW);
  digitalWrite(RL_IN2_PIN, HIGH);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    //client->ping();
    CONNECTED += 1;
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    CONNECTED -= 1;
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());
      //process input
      String input(msg.c_str());
      
      if (info->opcode == WS_TEXT){
        processTextCmd(input,client);
      }else
        client->binary("I got your binary message");
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());
      String input(msg.c_str());
      if ((info->index + len) == info->len)
      {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT){
            processTextCmd(input,client);
          }else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void setup()
{

  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi..");
  // }

  // Serial.println(WiFi.localIP());

  MDNS.addService("http", "tcp", 80);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String(ESP.getFreeHeap()));
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/setcamip", HTTP_GET, [](AsyncWebServerRequest *request) {
    memset(camip,0,CAM_IP_LEN);
    if(request->hasArg("camip")){
      String myIp = request->arg("camip");
      memcpy(camip,myIp.c_str(),strlen(myIp.c_str()));
    }
    Serial.printf("set camip to %s",camip);
    Serial.println("");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String(camip));
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/getcamip", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String(camip));
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  
  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      Serial.printf("GET");
    else if (request->method() == HTTP_POST)
      Serial.printf("POST");
    else if (request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if (request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if (request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength())
    {
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
      AsyncWebHeader *h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for (i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isFile())
      {
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      }
      else if (p->isPost())
      {
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
      else
      {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char *)data);
    if (index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

  ledcAttachPin(FR_PWM_PIN, FR_CHANNEL); // assign RGB led pins to channels
  ledcAttachPin(FL_PWM_PIN, FL_CHANNEL);
  ledcAttachPin(RR_PWM_PIN, RR_CHANNEL);
  ledcAttachPin(RL_PWM_PIN, RL_CHANNEL);
  ledcSetup(FR_CHANNEL, feq, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(FL_CHANNEL, feq, 8);
  ledcSetup(RR_CHANNEL, feq, 8);
  ledcSetup(RL_CHANNEL, feq, 8);
  pinMode(FR_IN1_PIN, OUTPUT);
  pinMode(FR_IN2_PIN, OUTPUT);
  pinMode(FL_IN1_PIN, OUTPUT);
  pinMode(FL_IN2_PIN, OUTPUT);
  pinMode(RR_IN1_PIN, OUTPUT);
  pinMode(RR_IN2_PIN, OUTPUT);
  pinMode(RL_IN1_PIN, OUTPUT);
  pinMode(RL_IN2_PIN, OUTPUT);

  server.begin();
}

void loop()
{
  ws.cleanupClients();
  if (CONNECTED <= 0)
  {
    continueOutput = false;
    stopCar();
  }
}

/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP32 chip.

  Note: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial


#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32_SSL.h>

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



// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "tT_R_ZV88d-Np3eu7cZpWdFKKEPoWCLs";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "要連線的WIFI SSID";
char pass[] = "WIFI的密碼";
int outputPower = 0;
int turnPower = 0;
int forwardStatus = 1;
int feq = 5000;
int dutyCycle = 210;
int continueOutput = false;
int stoped = true;
//640 - 850

void stopCar(){
  ledcWrite(1,0);
  ledcWrite(2,0);
  ledcWrite(3,0);
  ledcWrite(4,0); 
  stoped = true;
}

void enPower(){
  int p1 = map(outputPower,0,1023,0,255);
  Serial.print("output power level = ");
  Serial.println(p1);
  ledcWrite(1,p1);
  ledcWrite(2,p1);
  ledcWrite(3,p1);
  ledcWrite(4,p1);
  // if ((p1 > dutyCycle) && stoped){
  //   Serial.print("Gather than base duty Cycle : ");
  //   Serial.println(dutyCycle);
  //   int max = p1;
  //   p1 = dutyCycle;
  //     Serial.print("ducy Cycle start from : ");
  //     Serial.println(p1);
  //   while(p1 < max){
  //     ledcWrite(1,p1);
  //     ledcWrite(2,p1);
  //     ledcWrite(3,p1);
  //     ledcWrite(4,p1);
  //     delay(100);
  //     p1+=1;
  //     Serial.print("Next ducy cycle : ");
  //     Serial.println(p1);
  //   }
  // }else{
  //   ledcWrite(1,p1);
  //   ledcWrite(2,p1);
  //   ledcWrite(3,p1);
  //   ledcWrite(4,p1);
  // }
  stoped = false; 
}

void forward(){
  forwardStatus = 1;
  digitalWrite(FR_IN1_PIN,HIGH);
  digitalWrite(FR_IN2_PIN,LOW);
  digitalWrite(FL_IN1_PIN,HIGH);
  digitalWrite(FL_IN2_PIN,LOW);
  digitalWrite(RR_IN1_PIN,HIGH);
  digitalWrite(RR_IN2_PIN,LOW);
  digitalWrite(RL_IN1_PIN,HIGH);
  digitalWrite(RL_IN2_PIN,LOW);
}

void goBack(){
  forwardStatus = 0;
  digitalWrite(FR_IN1_PIN,LOW);
  digitalWrite(FR_IN2_PIN,HIGH);
  digitalWrite(FL_IN1_PIN,LOW);
  digitalWrite(FL_IN2_PIN,HIGH);
  digitalWrite(RR_IN1_PIN,LOW);
  digitalWrite(RR_IN2_PIN,HIGH);
  digitalWrite(RL_IN1_PIN,LOW);
  digitalWrite(RL_IN2_PIN,HIGH);
}

void goRight(){
  //如果是前進狀態，停右前輪逆轉右後輪
  //如果是倒退狀態，停右後輪逆轉右前輪
  if(forwardStatus==1){
    digitalWrite(FR_IN1_PIN,HIGH);
    digitalWrite(FR_IN2_PIN,LOW);
    digitalWrite(FL_IN1_PIN,HIGH);
    digitalWrite(FL_IN2_PIN,LOW);
    digitalWrite(RR_IN1_PIN,LOW);
    digitalWrite(RR_IN2_PIN,HIGH);
    digitalWrite(RL_IN1_PIN,HIGH);
    digitalWrite(RL_IN2_PIN,LOW);
  }else{
    digitalWrite(FR_IN1_PIN,HIGH);
    digitalWrite(FR_IN2_PIN,LOW);
    digitalWrite(FL_IN1_PIN,LOW);
    digitalWrite(FL_IN2_PIN,HIGH);
    digitalWrite(RR_IN1_PIN,LOW);
    digitalWrite(RR_IN2_PIN,HIGH);
    digitalWrite(RL_IN1_PIN,LOW);
    digitalWrite(RL_IN2_PIN,HIGH);
  }
}

void goLeft(){
  //如果是前進狀態，停左前輪逆轉左後輪
  //如果是倒退狀態，停左後輪逆轉左前輪
  if(forwardStatus==1){
    digitalWrite(FR_IN1_PIN,HIGH);
    digitalWrite(FR_IN2_PIN,LOW);
    digitalWrite(FL_IN1_PIN,HIGH);
    digitalWrite(FL_IN2_PIN,LOW);
    digitalWrite(RR_IN1_PIN,HIGH);
    digitalWrite(RR_IN2_PIN,LOW);
    digitalWrite(RL_IN1_PIN,LOW);
    digitalWrite(RL_IN2_PIN,HIGH);
  }else{
    digitalWrite(FR_IN1_PIN,LOW);
    digitalWrite(FR_IN2_PIN,HIGH);
    digitalWrite(FL_IN1_PIN,HIGH);
    digitalWrite(FL_IN2_PIN,LOW);
    digitalWrite(RR_IN1_PIN,LOW);
    digitalWrite(RR_IN2_PIN,HIGH);
    digitalWrite(RL_IN1_PIN,LOW);
    digitalWrite(RL_IN2_PIN,HIGH);
  }
}

void right(){
  int p1 = map(outputPower,0,1023,0,255);
  int p2 = map(turnPower,0,1023,0,255);
  if(forwardStatus==1){
    ledcWrite(FR_CHANNEL,0);
    ledcWrite(RR_CHANNEL,p2);
    ledcWrite(FL_CHANNEL,p1);
    ledcWrite(RL_CHANNEL,p1);
  }else{
    ledcWrite(FR_CHANNEL,p2);
    ledcWrite(RR_CHANNEL,0);
    ledcWrite(FL_CHANNEL,p1);
    ledcWrite(RL_CHANNEL,p1);
  }
  //如果是前進狀態，停右前輪
  //如果是倒退狀態，停右後輪
  // if(forwardStatus==1){
  //   ledcWrite(FR_CHANNEL,0);
  //   ledcWrite(FL_CHANNEL,p2);
  // }else{
  //   ledcWrite(FR_CHANNEL,p1);
  //   ledcWrite(FL_CHANNEL,p1);
  // }
  // if(forwardStatus==1){
  //   ledcWrite(RR_CHANNEL,p1);
  //   ledcWrite(RL_CHANNEL,p1);  
  // }else{
  //   ledcWrite(RR_CHANNEL,0);
  //   ledcWrite(RL_CHANNEL,p2);  
  // }
}

void left(){
  int p1 = map(outputPower,0,1023,0,255);
  int p2 = map(turnPower,0,1023,0,255);
  if(forwardStatus==1){
    ledcWrite(FR_CHANNEL,p1);
    ledcWrite(RR_CHANNEL,p1);
    ledcWrite(FL_CHANNEL,0);
    ledcWrite(RL_CHANNEL,p2);
  }else{
    ledcWrite(FR_CHANNEL,p1);
    ledcWrite(RR_CHANNEL,p1);
    ledcWrite(FL_CHANNEL,p2);
    ledcWrite(RL_CHANNEL,0);
  }

  //如果是前進狀態，停左前輪
  //如果是倒退狀態，停左後輪
  // if(forwardStatus==1){
  //   ledcWrite(FL_CHANNEL,0);
  //   ledcWrite(RL_CHANNEL,p2);
  // }else{
  //   ledcWrite(FL_CHANNEL,p1);
  //   ledcWrite(RL_CHANNEL,p1);
  // }
  // if(forwardStatus==1){
  //   ledcWrite(RR_CHANNEL,p1);
  //   ledcWrite(FR_CHANNEL,p1);
  // }else{
  //   ledcWrite(FL_CHANNEL,p2);
  //   ledcWrite(RL_CHANNEL,0);
  // }
}

void spinRight(){
  digitalWrite(FR_IN1_PIN,LOW);
  digitalWrite(FR_IN2_PIN,HIGH);
  digitalWrite(RR_IN1_PIN,LOW);
  digitalWrite(RR_IN2_PIN,HIGH);
}

void spinLeft(){
  digitalWrite(FL_IN1_PIN,LOW);
  digitalWrite(FL_IN2_PIN,HIGH);
  digitalWrite(RL_IN1_PIN,LOW);
  digitalWrite(RL_IN2_PIN,HIGH);
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  Blynk.begin(auth, ssid, pass);
  ledcAttachPin(FR_PWM_PIN, FR_CHANNEL); // assign RGB led pins to channels
  ledcAttachPin(FL_PWM_PIN, FL_CHANNEL);
  ledcAttachPin(RR_PWM_PIN, RR_CHANNEL);
  ledcAttachPin(RL_PWM_PIN, RL_CHANNEL);
  ledcSetup(FR_CHANNEL, feq, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(FL_CHANNEL, feq, 8);
  ledcSetup(RR_CHANNEL, feq, 8);
  ledcSetup(RL_CHANNEL, feq, 8);
  pinMode(FR_IN1_PIN,OUTPUT);
  pinMode(FR_IN2_PIN,OUTPUT);
  pinMode(FL_IN1_PIN,OUTPUT);
  pinMode(FL_IN2_PIN,OUTPUT);
  pinMode(RR_IN1_PIN,OUTPUT);
  pinMode(RR_IN2_PIN,OUTPUT);
  pinMode(RL_IN1_PIN,OUTPUT);
  pinMode(RL_IN2_PIN,OUTPUT);
}

void loop()
{
  if ((WiFi.status() != WL_CONNECTED) || !Blynk.connected()){
    continueOutput = false;
    stopCar();
  }
  Blynk.run();
  // if(continueOutput){
  //   enPower();
  // }
}

//forward
BLYNK_WRITE(V1) {
  int x = param.asInt();
  Serial.print("forward = ");
  Serial.println(x);
  if(x == 1){
    continueOutput = true;
    forward();
    enPower();
  }else{
    continueOutput = false;
    stopCar();
  }
}

//goback
BLYNK_WRITE(V2) {
  int x = param.asInt();
  Serial.print("goback = ");
  Serial.println(x);
  if(x == 1){
    continueOutput = true;
    goBack();
    enPower();
  }else{
    continueOutput = false;
    stopCar();
  }
}

//set output power level
BLYNK_WRITE(V3) {
  outputPower = param.asInt();
  Serial.print("set power = ");
  Serial.println(outputPower);
  if(continueOutput){
    enPower();
  }
}

//設定轉彎力道
BLYNK_WRITE(V4) {
  turnPower = param.asInt();
  Serial.print("set turn power = ");
  Serial.println(turnPower);
}

BLYNK_WRITE(V5) {
  int x = param.asInt();
  Serial.println("=== turn right ===");
  if(x == 1){
    goRight();
    right();
  }else{
    if(forwardStatus==1){
      forward();
    }
    else
    {
      goBack();
    }  
    if(continueOutput){
      enPower();
    }
    else
    {
      stopCar();
    }
  }
}

BLYNK_WRITE(V6) {
  int x = param.asInt();
  Serial.println("=== turn left ===");
  if(x == 1){
    goLeft();
    left();
  }else{
    if(forwardStatus==1){
      forward();
    }
    else
    {
      goBack();
    }
    if(continueOutput){
      enPower();
    }
    else
    {
      stopCar();
    }
  }
}

BLYNK_WRITE(V7) {
  int x = param.asInt();
  Serial.println("==== spin right ====");
  if(x == 1){
    forward(); //先回到前進狀態
    spinRight();
    enPower();
  }else{
    stopCar();
  }
}

BLYNK_WRITE(V8) {
  int x = param.asInt();
  Serial.println("==== spin left ====");
  if(x == 1){
    forward(); //先回到前進狀態
    spinLeft();
    enPower();
  }else{
    stopCar();
  }
}

//board: http://arduino.esp8266.com/stable/package_esp8266com_index.json

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
// Wifi
const char* ssid = "yourssid";
const char* password = "yourpassword";
// Facebook
// https://github.com/gbrault/esp8266-Arduino/blob/master/doc/esp8266wifi/client-secure-examples.md
const char* host = "graph.facebook.com";
const String url = "/v3.1/YOUR_FACEBOOK_PAGE_ID?fields=fan_count&access_token=YOUR_TOKEN";
const int httpsPort = 443;
const char* fingerprint = "YOUR_FINGERPRINT";
String payload;

int uSwitch = D5;
int dSwitch = D6;
int hSwitch = D7;
int kSwitch = 10;

int uStep = D0;
int dStep = D1;
int hStep = D2;
int kStep = D3;
int stepEnable = D4;

struct _cifra {
  int currentStep;
  int stepPin;
  int switchPin;
  int homeVal;
  byte state;
};

typedef struct _cifra cifra;

byte singlePrint(int target, cifra **number)
{
  byte doStep = 0;
  if ((*number)->state == 0)
    if (!digitalRead((*number)->switchPin))
    {
      doStep = 1;
    }
    else
      (*number)->state = 1;
  else if ((*number)->state == 1)
  {
    if (digitalRead((*number)->switchPin))
    {
      doStep = 1;
    }
    else
    {
      (*number)->state = 2;
      (*number)->currentStep = (*number)->homeVal;
    }
  }
  else if ((*number)->state == 2)
  {
    if ((*number)->currentStep != target)
    {
      doStep = 1;
      (*number)->currentStep = ((*number)->currentStep + 1) % 700;
    }
    else
      (*number)->state = 3;
  }
  return doStep;
}


void printVal(int target, cifra *u, cifra *d, cifra *h, cifra *k)
{
  k->state = 0;
  h->state = 0;
  d->state = 0;
  u->state = 0;

  int kTarget = (target / 1000) * 70;
  int hTarget = (target / 100 - (target / 1000) * 10) * 70;
  int dTarget = (target / 10 - (target / 100) * 10) * 70;
  int uTarget = (target / 1 - (target / 10) * 10) * 70;

  while (k->state != 3 ||
         h->state != 3 ||
         d->state != 3 ||
         u->state != 3)
  {

    byte doStepK, doStepH, doStepD, doStepU;
    doStepK = singlePrint(kTarget, &k);
    doStepH = (k->state == 3) ? singlePrint(hTarget, &h) : 1;
    doStepD = (h->state == 3) ? singlePrint(dTarget, &d) : 1;
    doStepU = (d->state == 3) ? singlePrint(uTarget, &u) : 1;

    digitalWrite(k->stepPin, doStepK);
    digitalWrite(h->stepPin, doStepH);
    digitalWrite(d->stepPin, doStepD);
    digitalWrite(u->stepPin, doStepU);
    delay(1);
    digitalWrite(k->stepPin, 0);
    digitalWrite(h->stepPin, 0);
    digitalWrite(d->stepPin, 0);
    digitalWrite(u->stepPin, 0);
    delay(1);

  }
}

int oldFancount = 0;
cifra u, d, h, k;

void setup() {
  Serial.begin(115200);

  wdt_disable();
  pinMode(uStep, OUTPUT);
  pinMode(dStep, OUTPUT);
  pinMode(hStep, OUTPUT);
  pinMode(kStep, OUTPUT);
  pinMode(stepEnable, OUTPUT);

  pinMode(uSwitch, INPUT);
  pinMode(dSwitch, INPUT);
  pinMode(hSwitch, INPUT);
  pinMode(kSwitch, INPUT);

  u.currentStep = 0;
  u.stepPin = uStep;
  u.switchPin = uSwitch;
  u.homeVal = 500;
  u.state = 0;

  d.currentStep = 0;
  d.stepPin = dStep;
  d.switchPin = dSwitch;
  d.homeVal = 510;
  d.state = 0;

  h.currentStep = 0;
  h.stepPin = hStep;
  h.switchPin = hSwitch;
  h.homeVal = 510;
  h.state = 0;

  k.currentStep = 0;
  k.stepPin = kStep;
  k.switchPin = kSwitch;
  k.homeVal = 510;
  k.state = 0;

  Serial.begin(115200);
  // Wifi
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

}
void loop() {
  String wifistatus = "WIFI OK";
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(host, httpsPort)) {
    String wifistatus = "WIFI FAIL";
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  // JSON
  String line = client.readStringUntil('\n');
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(line);
  int fancount = root[String("fan_count")];
  Serial.print("Fans: ");
  Serial.println(fancount);


  if (fancount > oldFancount) {
    digitalWrite(stepEnable, LOW);

    printVal(fancount, &u, &d, &h, &k);
    
    delay(2000);
    digitalWrite(stepEnable, HIGH);

    oldFancount = fancount;
  }

  delay(30000);
}

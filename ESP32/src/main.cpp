#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include <xboxlogo.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <map>

#define LED_BUILTIN 2
#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

std::map<String, unsigned long> timers;
unsigned long serialMessageStartTime;
int serialMessageCount = 0;  //874 seems to be it (with OLED display this is down to 402), (770 if not writing to serial and writing to OLED every second)
unsigned long MQTTMessageStartTime;
int MQTTMessageCount = 0;  

void displayMessage(String message);
void sendMQTTmessage(String message);
void updateMessageCount();
void checkMQTTconnection();
void dealWithMessage(String message, String value, const char *MQTTTopic, const char *MQTTPayload);
void dealWithMessage(String message, String value, const char *MQTTTopic, const char *MQTTPayload, int interval);

void setup()
{
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  //turn off bluetooth
  btStop();

#if defined(USE_SERIAL)
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif

  Serial2.begin(115200);

  // Init I2C bus & OLED
  Wire.begin();

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true) == false)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    ESP.restart();
  }

  display.clearDisplay(); //for Clearing the display
  //https://javl.github.io/image2cpp/
  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();

  //WIFI start up
  displayMessage(std::move("Connecting to WiFi")); // + (String)ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    displayMessage(std::move("."));
  }

  displayMessage(std::move("Connected! IP address: "));
  displayMessage(WiFi.localIP().toString());

  //set this to be a large enough value to allow an MQTT message containing a 22Kb JPEG to be sent
  MQTTClient.setBufferSize(30000);

  displayMessage(std::move("Connecting to MQTT server"));
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage(std::move("connect mqtt..."));
  checkMQTTconnection();

  messageStartTime = millis();
}

void loop()
{
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto message = Serial2.readStringUntil('\n');

    sendMQTTmessage(message);

    messageCount++;

    digitalWrite(LED_BUILTIN, LOW);
  }

  yield();
}

void updateMessageCount()
{
  //send reciever state every one second
  if (millis() - messageStartTime >= 1000)
  {
    if (messageCount > maxMessageCount)
    {
      maxMessageCount = messageCount;
    }

    messageStartTime = millis();

    //displayMessage(std::move("Max Msg Recieved: " + (String)maxMessageCount));

    messageCount = 0;
  }
}

void displayMessage(String message)
{
#if defined(USE_SERIAL)
  Serial.println(message);
#endif

  display.clearDisplay(); //for Clearing the display
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print((String)message);

  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();                                   //https://javl.github.io/image2cpp/
}

void sendMQTTmessage(String message)
{
  checkMQTTconnection();

  //deal with the message and make ready for sending
  message.trim();
  message.toLowerCase();

  //auto mqttMessage = message.c_str();
  auto tempValue = message.substring(message.indexOf(':') + 1);
  auto mqttValue = tempValue.c_str();

  if (message.startsWith(F("battery:")))
  {
    MQTTClient.publish("XBOX360/Battery", mqttValue);
  }

  dealWithMessage(message, "click: up", "XBOX360/D-Pad/Up", "Click");
  dealWithMessage(message, "click: down", "XBOX360/D-Pad/Down", "Click");
  dealWithMessage(message, "click: left", "XBOX360/D-Pad/Left", "Click");
  dealWithMessage(message, "click: right", "XBOX360/D-Pad/Right", "Click");

  dealWithMessage(message, "click: l1", "XBOX360/Bumper/Left", "Click");
  dealWithMessage(message, "click: r1", "XBOX360/Bumper/Right", "Click");

  dealWithMessage(message, "click: l3", "XBOX360/Stick/Left", "Click");
  dealWithMessage(message, "click: r3", "XBOX360/Stick/Right", "Click");

  dealWithMessage(message, "click: a", "XBOX360/Button/A", "Click");
  dealWithMessage(message, "click: b", "XBOX360/Button/B", "Click");
  dealWithMessage(message, "click: x", "XBOX360/Button/X", "Click");
  dealWithMessage(message, "click: y", "XBOX360/Button/Y", "Click");

  dealWithMessage(message, "click: xbox", "XBOX360/Button/XBOX", "Click");
  dealWithMessage(message, "click: back", "XBOX360/Button/Back", "Click");
  dealWithMessage(message, "click: start", "XBOX360/Button/Start", "Click");
  dealWithMessage(message, "click: sync", "XBOX360/Button/SYNC", "Click");

  dealWithMessage(message, "press: up", "XBOX360/D-Pad/Up", "Press", 200);
  dealWithMessage(message, "press: down", "XBOX360/D-Pad/Down", "Press", 200);
  dealWithMessage(message, "press: left", "XBOX360/D-Pad/Left", "Press", 200);
  dealWithMessage(message, "press: right", "XBOX360/D-Pad/Right", "Press", 200);

  dealWithMessage(message, "press: l1", "XBOX360/Bumper/Left", "Press", 200);
  dealWithMessage(message, "press: r1", "XBOX360/Bumper/Right", "Press", 200);

  dealWithMessage(message, "press: a", "XBOX360/Button/A", "Press", 200);
  dealWithMessage(message, "press: b", "XBOX360/Button/B", "Press", 200);
  dealWithMessage(message, "press: x", "XBOX360/Button/X", "Press", 200);
  dealWithMessage(message, "press: y", "XBOX360/Button/Y", "Press", 200);

  dealWithMessage(message, "press: xbox", "XBOX360/Button/XBOX", "Press", 200);
  dealWithMessage(message, "press: back", "XBOX360/Button/Back", "Press", 200);
  dealWithMessage(message, "press: start", "XBOX360/Button/Start", "Press", 200);
  dealWithMessage(message, "press: sync", "XBOX360/Button/SYNC", "Press", 200);

  //Trigger pulls
  if (message.startsWith(F("l2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - timers["l2"] > 200)
    {
      timers["l2"] = millis();

      long trigger = atol(mqttValue);

      auto mappedValue = String(map(trigger, 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/Trigger/Left", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Trigger/Left/Raw", mqttValue);
    }
  }

  if (message.startsWith(F("r2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - timers["r2"] > 200)
    {
      timers["r2"] = millis();

      long trigger = atol(mqttValue);

      auto mappedValue = String(map(trigger, 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/Trigger/Right", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Trigger/Right/Raw", mqttValue);
    }
  }

  //TRIGGER FAIL SAFES - if time between triggers is greater than 500 milliseconds then send zero
  if (millis() - timers["l2"] > 500)
  {
    timers["l2"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Trigger/Left", "0");
  }
  if (millis() - timers["r2"] > 500)
  {
    timers["r2"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Trigger/Right", "0");
  }

  //Left Hat X&Y
  if (message.startsWith(F("lefthatx:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["lefthatx"] > 200)
    {
      timers["lefthatx"] = millis();

      long x = atol(mqttValue);

      auto mappedValue = String(map(x, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Left/X", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Left/X/Raw", mqttValue);
    }
  }
  if (message.startsWith(F("lefthaty:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["lefthaty"] > 300)
    {
      timers["lefthaty"] = millis();

      long y = atol(mqttValue);

      auto mappedValue = String(map(y, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Left/Y", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Left/Y/Raw", mqttValue);
    }
  }
  //HAT FAIL SAFES
  if (millis() - timers["lefthatx"] > 500)
  {
    timers["lefthatx"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Stick/Left/X", "0");
  }
  if (millis() - timers["lefthaty"] > 500)
  {
    timers["lefthaty"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Stick/Left/Y", "0");
  }

  //Right Hat X&Y
  if (message.startsWith(F("righthatx:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["righthatx"] > 200)
    {
      timers["righthatx"] = millis() ;

      long x = atol(mqttValue);

      auto mappedValue = String(map(x, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Right/X", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Right/X/Raw", mqttValue);
    }
  }
  if (message.startsWith(F("righthaty:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["righthaty"] > 200)
    {
      timers["righthaty"] = millis();

      long y = atol(mqttValue);

      auto mappedValue = String(map(y, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Right/Y", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Right/Y/Raw", mqttValue);
    }
  }

  //HAT FAIL SAFES
  if (millis() - timers["righthatx"] > 500)
  {
    timers["righthatx"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Stick/Right/X", "0");
  }
  if (millis() - timers["righthaty"] > 500)
  {
    timers["righthaty"] = millis()+ random(1,100);
    MQTTClient.publish("XBOX360/Stick/Right/Y", "0");
  }
}

void checkMQTTconnection()
{
  if (!MQTTClient.connected())
  {
    if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_KEY))
    {
      displayMessage(std::move("Connected to MQTT server"));
    }
  }
}

void dealWithMessage(String message, String value, const char *MQTTTopic, const char *MQTTPayload)
{
  if (message == value)
  {
    MQTTClient.publish(MQTTTopic, MQTTPayload);
  }
}

void dealWithMessage(String message, String value, const char *MQTTTopic, const char *MQTTPayload, int interval)
{
  if (message == value)
  { //if time between triggers is greater than interval (milliseconds) then send
    if (millis() - timers[value] > interval)
    {
      timers[value] = millis();

      MQTTClient.publish(MQTTTopic, MQTTPayload);
    }
  }
}
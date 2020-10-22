#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include <xboxlogo.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <map>
#include <ArduinoJSON.h>

#define LED_BUILTIN 2
#define USE_SERIAL
//#define SEND_FAIL_SAFES

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;
QueueHandle_t serialQueue;
StaticJsonDocument<850> joystick;

std::map<String, unsigned long> timers;
unsigned long messageStartTime;
int serialMessageCount = 0; 
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
  Serial.flush();
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

  serialQueue = xQueueCreate(100, sizeof(String));

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

  Serial.flush();

  messageStartTime = millis();
}

void loop()
{
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto message = Serial2.readStringUntil('\n');

    serialMessageCount++;

    xQueueSend(serialQueue, &message, 0);

    sendMQTTmessage(message);

    digitalWrite(LED_BUILTIN, LOW);
  }

  yield();
}

void updateMessageCount()
{
  //send reciever state every one second
  if (millis() - messageStartTime >= 1000)
  {
    String msg = "Serial RX: ";
    msg += (String)serialMessageCount;
    msg += "\n";
    msg += "MQTT RX: ";
    msg += (String)MQTTMessageCount;

    displayMessage(std::move(msg));

    messageStartTime = millis();
    serialMessageCount = 0;
    MQTTMessageCount = 0;
  }
}

void displayMessage(String message)
{
  // #if defined(USE_SERIAL)
  //   Serial.println(message);
  // #endif

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
  message.toUpperCase();

  //auto mqttMessage = message.c_str();
  auto tempValue = message.substring(message.indexOf(':')+1);
  tempValue.trim();
  auto mqttValue = tempValue.c_str();

  if (message.startsWith(F("BATTERY:")))
  {
    MQTTClient.publish("XBOX360/Battery", mqttValue);
  }

  dealWithMessage(message, "U:C", "XBOX360/D-Pad/Up", "Click");
  dealWithMessage(message, "D:C", "XBOX360/D-Pad/Down", "Click");
  dealWithMessage(message, "L:C", "XBOX360/D-Pad/Left", "Click");
  dealWithMessage(message, "R:C", "XBOX360/D-Pad/Right", "Click");

  // dealWithMessage(message, "U:P", "XBOX360/D-Pad/Up", "Press", 200);
  // dealWithMessage(message, "D:P", "XBOX360/D-Pad/Down", "Press", 200);
  // dealWithMessage(message, "L:P", "XBOX360/D-Pad/Left", "Press", 200);
  // dealWithMessage(message, "R:P", "XBOX360/D-Pad/Right", "Press", 200);

  dealWithMessage(message, "L1:C", "XBOX360/Bumper/Left", "Click");
  dealWithMessage(message, "R1:C", "XBOX360/Bumper/Right", "Click");

  // dealWithMessage(message, "L1:P", "XBOX360/Bumper/Left", "Press", 200);
  // dealWithMessage(message, "R1:P", "XBOX360/Bumper/Right", "Press", 200);

  dealWithMessage(message, "L3:C", "XBOX360/Stick/Left", "Click");
  dealWithMessage(message, "R3:C", "XBOX360/Stick/Right", "Click");

  dealWithMessage(message, "A:C", "XBOX360/Button/A", "Click");
  dealWithMessage(message, "B:C", "XBOX360/Button/B", "Click");
  dealWithMessage(message, "X:C", "XBOX360/Button/X", "Click");
  dealWithMessage(message, "Y:C", "XBOX360/Button/Y", "Click");

  // dealWithMessage(message, "A:P", "XBOX360/Button/A", "Press", 200);
  // dealWithMessage(message, "B:P", "XBOX360/Button/B", "Press", 200);
  // dealWithMessage(message, "X:P", "XBOX360/Button/X", "Press", 200);
  // dealWithMessage(message, "Y:P", "XBOX360/Button/Y", "Press", 200);

  dealWithMessage(message, "XBOX:C", "XBOX360/Button/XBOX", "Click");
  dealWithMessage(message, "BACK:C", "XBOX360/Button/Back", "Click");
  dealWithMessage(message, "START:C", "XBOX360/Button/Start", "Click");
  dealWithMessage(message, "SYNC:C", "XBOX360/Button/SYNC", "Click");

  //Trigger pulls
  if (message.startsWith(F("L2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - timers["L2"] > 200)
    {
      timers["L2"] = millis();

      long trigger = atol(mqttValue);

      // Serial.print("L2<<<");
      // Serial.println(trigger);

      auto mappedValue = String(map(trigger, 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/Trigger/Left", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Trigger/Left/Raw", mqttValue);
    }
  }

  if (message.startsWith(F("R2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - timers["R2"] > 200)
    {
      timers["R2"] = millis();

      long trigger = atol(mqttValue);

      // Serial.print("R2<<<");
      // Serial.println(trigger);

      auto mappedValue = String(map(trigger, 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/Trigger/Right", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Trigger/Right/Raw", mqttValue);
    }
  }

  //TRIGGER FAIL SAFES - if time between triggers is greater than 500 milliseconds then send zero
  #if defined(SEND_FAIL_SAFES)
  if (millis() - timers["L2"] > 500)
  {
    timers["L2"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Trigger/Left", "0");
  }
  if (millis() - timers["R2"] > 500)
  {
    timers["R2"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Trigger/Right", "0");
  }
  #endif

  //Left Hat X&Y
  if (message.startsWith(F("LHX:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["LHX"] > 200)
    {
      timers["LHX"] = millis();

      long x = atol(mqttValue);

      // Serial.print("LHX<<<");
      // Serial.println(x);

      auto mappedValue = String(map(x, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Left/X", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Left/X/Raw", mqttValue);
    }
  }
  if (message.startsWith(F("LHY:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["LHY"] > 300)
    {
      timers["LHY"] = millis();

      long y = atol(mqttValue);

      // Serial.print("LHY<<<");
      // Serial.println(y);

      auto mappedValue = String(map(y, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Left/Y", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Left/Y/Raw", mqttValue);
    }
  }
  //HAT FAIL SAFES
  #if defined(SEND_FAIL_SAFES)
  if (millis() - timers["LHX"] > 500)
  {
    timers["LHX"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Stick/Left/X", "0");
  }
  if (millis() - timers["LHY"] > 500)
  {
    timers["LHY"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Stick/Left/Y", "0");
  }
  #endif

  //Right Hat X&Y
  if (message.startsWith(F("RHX:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["RHX"] > 200)
    {
      timers["RHX"] = millis();

      long x = atol(mqttValue);

      // Serial.print("RHX<<<");
      // Serial.println(x);

      auto mappedValue = String(map(x, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Right/X", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Right/X/Raw", mqttValue);
    }
  }
  if (message.startsWith(F("RHY:")))
  {
    //if time between triggers is greater than 200 milliseconds then send
    if (millis() - timers["RHY"] > 200)
    {
      timers["RHY"] = millis();

      long y = atol(mqttValue);

      // Serial.print("RHY<<<");
      // Serial.println(y);

      auto mappedValue = String(map(y, -32768, 32767, -100, 100));

      MQTTClient.publish("XBOX360/Stick/Right/Y", mappedValue.c_str());
      //MQTTClient.publish("XBOX360/Stick/Right/Y/Raw", mqttValue);
    }
  }

  //HAT FAIL SAFES
  #if defined(SEND_FAIL_SAFES)
  if (millis() - timers["RHX"] > 500)
  {
    timers["RHX"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Stick/Right/X", "0");
  }
  if (millis() - timers["RHY"] > 500)
  {
    timers["RHY"] = millis() + random(1, 100);
    MQTTClient.publish("XBOX360/Stick/Right/Y", "0");
  }
  #endif
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
    // Serial.print(MQTTTopic);
    // Serial.println(MQTTPayload);

    MQTTClient.publish(MQTTTopic, MQTTPayload);

    MQTTMessageCount++;
  }
}

void dealWithMessage(String message, String value, const char *MQTTTopic, const char *MQTTPayload, int interval)
{
  if (message == value)
  { //if time between triggers is greater than interval (milliseconds) then send
    if (millis() - timers[value] > interval)
    {
      timers[value] = millis();

      Serial.print(message);
      Serial.print(":");
      Serial.println(value);

      MQTTClient.publish(MQTTTopic, MQTTPayload);

      MQTTMessageCount++;
    }
  }
}
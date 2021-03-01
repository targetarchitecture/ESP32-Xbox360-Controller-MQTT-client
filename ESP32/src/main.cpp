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
#include <topics.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <list>

#define LED_BUILTIN 2
#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

//create the JSON object once
const size_t JSON_capacity = JSON_OBJECT_SIZE(21);
DynamicJsonDocument JSON(JSON_capacity);

unsigned long displayMessageStartTime = millis();
uint64_t serialMessageCount = 0;
uint64_t MQTTMessageCount = 0;

//declare functions
void displayMessage(std::string message);
void updateMessageCount();
void checkMQTTconnection();
void dealWithReceivedMessage(const std::string message);
std::vector<std::string> processQueueMessage(const std::string msg);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  //turn off bluetooth
  btStop();

#if defined(USE_SERIAL)
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.flush();
#endif

  //baud speed of Arduino Uno sketch
  Serial2.begin(38400);

  // Init I2C bus & OLED
  Wire.begin();

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true) == false)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    ESP.restart();
  }

  //WIFI start up
  displayMessage("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    displayMessage(".");
  }

  displayMessage("Connected! IP address: ");
  displayMessage(WiFi.localIP().toString().c_str());

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //set this to be a large enough value to allow a large MQTT message
  MQTTClient.setBufferSize(5000);

  displayMessage("Connecting to MQTT server");
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage("connect mqtt...");
  checkMQTTconnection();
}

void loop()
{
  //sort out the OLED display message
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    String UART = Serial2.readStringUntil('\n');

    std::string message = UART.c_str();

    serialMessageCount++;

    //Too verbose
    //MQTTClient.publish(MQTT_INFO_TOPIC, message.c_str());

    dealWithReceivedMessage(message.c_str());

    checkMQTTconnection();

    digitalWrite(LED_BUILTIN, LOW);
  }

  //give some breathing space (arduino only has 5ms delay)
  delay(5);
}

void updateMessageCount()
{
  //send reciever state every one second
  if (millis() - displayMessageStartTime >= 1000)
  {
    std::stringstream msg;

    msg << "Serial RX: " << serialMessageCount << "\n"
        << "MQTT RX: " << MQTTMessageCount;

    displayMessage(msg.str().c_str());

    displayMessageStartTime = millis();
    serialMessageCount = 0;
    MQTTMessageCount = 0;
  }
}

void dealWithReceivedMessage(const std::string message)
{
  //deal with the message and make ready for sending
  String msg = message.c_str();

  msg.trim();
  msg.toUpperCase();

  //check for bad string with more than one command
  auto L1 = msg.length();
  auto msg2 = msg;
  msg2.replace(":", "");
  auto L2 = msg2.length();

  if ((L1 - L2) > 1)
  {
    String err = "Multiple Commands:" + msg;

    MQTTClient.publish(MQTT_ERROR_TOPIC, err.c_str());

    return;
  }

  if (msg.startsWith("BAT:"))
  {
    auto battery = msg.substring(msg.indexOf(":") + 1);

    MQTTClient.publish(MQTT_BATTERY_TOPIC, battery.c_str());

    return;
  }

  if (msg.startsWith("XCC:") || msg.startsWith("XRC:"))
  {
    MQTTClient.publish(MQTT_INFO_TOPIC, msg.c_str());

    return;
  }

  if (msg.startsWith("BTN:L2"))
  {
    auto triggerMsg = msg.substring(msg.indexOf(",") + 1);

    long triggerValue = atol(triggerMsg.c_str());
    long triggerValue_mapped = map(triggerValue, 0, 255, 0, 100);

    //clear down JSON array
    JSON.clear();

    JSON["left"] = triggerValue;
    JSON["left_mapped"] = triggerValue_mapped;

    JsonObject buttonObj = JSON.as<JsonObject>();

    std::string mqttMessage;

    serializeJson(buttonObj, mqttMessage);

    MQTTClient.publish(MQTT_TRIGGER_TOPIC, mqttMessage.c_str());

    return;
  }

  if (msg.startsWith("BTN:R2"))
  {
    auto triggerMsg = msg.substring(msg.indexOf(",") + 1);

    long triggerValue = atol(triggerMsg.c_str());
    long triggerValue_mapped = map(triggerValue, 0, 255, 0, 100);

    //clear down JSON array
    JSON.clear();

    JSON["right"] = triggerValue;
    JSON["right_mapped"] = triggerValue_mapped;

    JsonObject buttonObj = JSON.as<JsonObject>();

    std::string mqttMessage;

    serializeJson(buttonObj, mqttMessage);

    MQTTClient.publish(MQTT_TRIGGER_TOPIC, mqttMessage.c_str());

    return;
  }

  if (msg.startsWith("BTN:"))
  {
    String buttonCSV = msg.substring(msg.indexOf(":") + 1);

    std::vector<std::string> buttonList;

    buttonList = processQueueMessage(buttonCSV.c_str());

    //clear down button array
    JSON.clear();

    for (std::string i : buttonList)
    {
      JSON[i] = true;
    }

    JsonObject buttonObj = JSON.as<JsonObject>();

    if (buttonObj.size() > 0)
    {
      std::string mqttMessage;

      serializeJson(buttonObj, mqttMessage);

      MQTTClient.publish(MQTT_BUTTON_TOPIC, mqttMessage.c_str());
    }

    return;
  }

  if (msg.startsWith("LH:"))
  {
    String hatCSV = msg.substring(msg.indexOf(":") + 1);

    //Serial.println(hatCSV.c_str());

    std::list<std::string> hatValues;
    std::istringstream f(hatCSV.c_str());
    std::string part;

    while (std::getline(f, part, ','))
    {
      hatValues.push_back(part);
    }

    hatValues.pop_front();
    auto Xstr = hatValues.front();
    hatValues.pop_front();
    hatValues.pop_front();
    auto Ystr = hatValues.front();

    auto X = atoi(Xstr.c_str());
    auto Y = atoi(Ystr.c_str());

    auto mappedXValue = map(X, -32768, 32767, -100, 100);
    auto mappedYValue = map(Y, -32768, 32767, -100, 100);

    //clear down JSON array
    JSON.clear();

    JSON["x"] = X;
    JSON["y"] = Y;

    JSON["x_mapped"] = mappedXValue;
    JSON["y_mapped"] = mappedYValue;

    JsonObject buttonObj = JSON.as<JsonObject>();

    std::string mqttMessage;

    serializeJson(buttonObj, mqttMessage);

    MQTTClient.publish(MQTT_LEFT_TOPIC, mqttMessage.c_str());

    return;
  }

  if (msg.startsWith("RH:"))
  {
    String hatCSV = msg.substring(msg.indexOf(":") + 1);

    //Serial.println(hatCSV.c_str());

    std::list<std::string> hatValues;
    std::istringstream f(hatCSV.c_str());
    std::string part;

    while (std::getline(f, part, ','))
    {
      hatValues.push_back(part);
    }

    hatValues.pop_front();
    auto Xstr = hatValues.front();
    hatValues.pop_front();
    hatValues.pop_front();
    auto Ystr = hatValues.front();

    auto X = atoi(Xstr.c_str());
    auto Y = atoi(Ystr.c_str());

    auto mappedXValue = map(X, -32768, 32767, -100, 100);
    auto mappedYValue = map(Y, -32768, 32767, -100, 100);

    //clear down JSON array
    JSON.clear();

    JSON["x"] = X;
    JSON["y"] = Y;

    JSON["x_mapped"] = mappedXValue;
    JSON["y_mapped"] = mappedYValue;

    JsonObject buttonObj = JSON.as<JsonObject>();

    std::string mqttMessage;

    serializeJson(buttonObj, mqttMessage);

    MQTTClient.publish(MQTT_RIGHT_TOPIC, mqttMessage.c_str());

    return;
  }
}

void checkMQTTconnection()
{
  if (!MQTTClient.connected())
  {
    if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_KEY))
    {
      displayMessage("Connected to MQTT server");
    }
  }
}

void displayMessage(const std::string message)
{
  display.clearDisplay(); //for Clearing the display
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(message.c_str());

  //https://javl.github.io/image2cpp/
  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();
}

std::vector<std::string> processQueueMessage(const std::string msg)
{
  std::vector<std::string> parts;
  std::istringstream f(msg);
  std::string part;

  while (std::getline(f, part, ','))
  {
    parts.push_back(part);
  }

  return parts;
}

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

#define LED_BUILTIN 2
#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

//create the joystick object once
const size_t left_stick_capacity = JSON_OBJECT_SIZE(4);
DynamicJsonDocument left_joystick(left_stick_capacity);

const size_t right_stick_capacity = JSON_OBJECT_SIZE(4);
DynamicJsonDocument right_joystick(right_stick_capacity);

const size_t button_capacity = JSON_OBJECT_SIZE(21);
DynamicJsonDocument buttons(button_capacity);

unsigned long displayMessageStartTime = millis();
int serialMessageCount = 0;
int MQTTMessageCount = 0;
unsigned long lastMQTTMessageSentTime = millis();
unsigned long lastMessageReceivedTime = millis();

//declare functions
void displayMessage(String message);
void updateMessageCount();
void checkMQTTconnection();
void sendMQTTmessage();
void dealWithReceivedMessage(const String message);
void dealWithButtonClick(const char *JSONKey, bool JSONValue);
void setJoystick(const std::string message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue);
std::vector<std::string> processQueueMessage(const String msg);

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

    String message = Serial2.readStringUntil('\n');

    Serial.println(message.c_str());

    serialMessageCount++;

    lastMessageReceivedTime = millis();

    //MQTTClient.publish(MQTT_INFO_TOPIC, message.c_str());

    dealWithReceivedMessage(message);

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

void setJoystick(std::string message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue)
{
  // auto tempValue = message.substring(message.indexOf(':') + 1);
  // tempValue.trim();
  // auto mqttValue = tempValue.c_str();

  // long analogValue = atol(mqttValue);

  // auto mappedValue = map(analogValue, lowestValue, highestValue, 0, 100);

  // joystick[JSONKey] = analogValue;
  // joystick[JSONKeyMapped] = mappedValue;

  // joystickActionTimes[JSONKey] = millis() + FAIL_SAFE_TIME_MS;
  // joystickActionTimes[JSONKeyMapped] = millis() + FAIL_SAFE_TIME_MS;
}


void dealWithReceivedMessage(const String message)
{
  //deal with the message and make ready for sending
  String msg = message;

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

  //MQTTClient.publish("XBOX360/msg", msg.c_str());

  MQTTClient.publish("XBOX360/test", String(L1 - L2).c_str());

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

  if (msg.startsWith("BTN:L2")) // || msg.startsWith("BTN:R2"))
  {
    auto triggerValue = msg.substring(msg.indexOf(",") + 1);

    //clear down button array
    buttons.clear();

    buttons["left"] = triggerValue;

    JsonObject buttonObj = buttons.as<JsonObject>();

    std::string json;

    serializeJson(buttonObj, json);

    MQTTClient.publish(MQTT_TRIGGER_TOPIC, json.c_str());

    return;
  }

  if (msg.startsWith("BTN:R2")) // || msg.startsWith("BTN:R2"))
  {
    auto triggerValue = msg.substring(msg.indexOf(",") + 1);

    //clear down button array
    buttons.clear();

    buttons["right"] = triggerValue;

    JsonObject buttonObj = buttons.as<JsonObject>();

    std::string json;

    serializeJson(buttonObj, json);

    MQTTClient.publish(MQTT_TRIGGER_TOPIC, json.c_str());

    return;
  }

  if (msg.startsWith("BTN:"))
  {
    auto buttonCSV = msg.substring(msg.indexOf(":") + 1);

    std::vector<std::string> buttonList;

    buttonList = processQueueMessage(buttonCSV);

    //clear down button array
    buttons.clear();

    for (std::string i : buttonList)
    {
      buttons[i] = true;
    }

    JsonObject buttonObj = buttons.as<JsonObject>();

    if (buttonObj.size() > 0)
    {
      std::string json;

      serializeJson(buttonObj, json);

      MQTTClient.publish(MQTT_BUTTON_TOPIC, json.c_str());
    }

    return;
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

void displayMessage(const String message)
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

std::vector<std::string> processQueueMessage(const String msg)
{
  std::vector<std::string> parts;
  std::istringstream f(msg.c_str());
  std::string part;

  while (std::getline(f, part, ','))
  {
    parts.push_back(part);

    Serial.println(part.c_str());
  }

  return parts;
}

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

#define LED_BUILTIN 2
#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

// StaticJsonDocument<850> joystick;
// StaticJsonDocument<850> failSafeValues;
// std::map<String, unsigned long> joystickActionTimes;

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
//void displayMessage(String message);
void displayMessage(const std::string message);
void updateMessageCount();
void checkMQTTconnection();
void sendMQTTmessage();
void dealWithReceivedMessage(const std::string message);
//void checkFailSafe();
void dealWithButtonClick(const char *JSONKey, bool JSONValue);
void setJoystick(const std::string message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue);
//void setupFailSafeValues();
// std::string &rtrim(std::string &str, const std::string &chars = "\t\n\v\f\r ");
// std::string &ltrim(std::string &str, const std::string &chars = "\t\n\v\f\r ");
// std::string &trim(std::string &str, const std::string &chars = "\t\n\v\f\r ");

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

    serialMessageCount++;

    lastMessageReceivedTime = millis();

     MQTTClient.publish(MQTT_INFO_TOPIC, message.c_str());

    //dealWithReceivedMessage(message);

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

    displayMessage(msg.str());

    displayMessageStartTime = millis();
    serialMessageCount = 0;
    MQTTMessageCount = 0;
  }
}

void dealWithButtonClick(const char *JSONKey, bool JSONValue)
{
  // joystick[JSONKey] = JSONValue;

  // joystickActionTimes[JSONKey] = millis() + FAIL_SAFE_TIME_MS;
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

void setValue(std::string message, std::string value, const char *JSONKey)
{
  // if (message.startsWith(value))
  // {
  //   auto tempValue = message.substring(message.indexOf(':') + 1);
  //   tempValue.trim();

  //   joystick[JSONKey] = tempValue;
  // }
}

void dealWithReceivedMessage(String message)
{
  // //deal with the message and make ready for sending
  // message = trim(message);

  // std::transform(message.begin(), message.end(), message.begin(), ::toupper);

  // // message..trim();
  // // message.toUpperCase();

  // if (message == "BATTERY:")
  // {
  //   setValue(message, "BATTERY:", "battery");

  //   MQTTClient.publish(MQTT_BATTERY_TOPIC, json.c_str());
  // }
  
  // else if (message == "A:C")
  // {
  //   dealWithButtonClick("button_a", true);
  // }
  // else if (message == "B:C")
  // {
  //   dealWithButtonClick("button_b", true);
  // }
  // else if (message == "X:C")
  // {
  //   dealWithButtonClick("button_x", true);
  // }
  // else if (message == "A:C")
  // {
  //   dealWithButtonClick("button_y", true);
  // }
  // else if (message == "U:C")
  // {
  //   dealWithButtonClick("pad_up", true);
  // }
  // else if (message == "D:C")
  // {
  //   dealWithButtonClick("pad_down", true);
  // }
  // else if (message == "L:C")
  // {
  //   dealWithButtonClick("pad_left", true);
  // }
  // else if (message == "R:C")
  // {
  //   dealWithButtonClick("pad_right", true);
  // }
  // else if (message == "START:C")
  // {
  //   dealWithButtonClick("start", true);
  // }
  // else if (message == "BACK:C")
  // {
  //   dealWithButtonClick("back", true);
  // }
  // else if (message == "XBOX:C")
  // {
  //   dealWithButtonClick("xbox", true);
  // }
  // else if (message == "SYNC:C")
  // {
  //   dealWithButtonClick("sync", true);
  // }
  // else if (message == "L1:C")
  // {
  //   dealWithButtonClick("shoulder_left", true);
  // }
  // else if (message == "R2:C")
  // {
  //   dealWithButtonClick("shoulder_right", true);
  // }
  // else if (message == "L3:C")
  // {
  //   dealWithButtonClick("left", true);
  // }
  // else if (message == "R3:C")
  // {
  //   dealWithButtonClick("right", true);
  // }
  // else if (message.startsWith("L2:"))
  // {
  //   setJoystick(message, "trigger_left", "trigger_left_mapped", 0, 255);
  // }
  // else if (message.startsWith("R2:"))
  // {
  //   setJoystick(message, "trigger_right", "trigger_right_mapped", 0, 255);
  // }
  // else if (message.startsWith("LHX:"))
  // {
  //   setJoystick(message, "left_x", "left_x_mapped", -32768, 32767);
  // }
  // else if (message.startsWith("LHY:"))
  // {
  //   setJoystick(message, "left_y", "left_y_mapped", -32768, 32767);
  // }
  // else if (message.startsWith("RHX:"))
  // {
  //   setJoystick(message, "right_x", "right_x_mapped", -32768, 32767);
  // }
  // else if (message.startsWith("RHY:"))
  // {
  //   setJoystick(message, "right_y", "right_y_mapped", -32768, 32767);
  // }

  // //only send MQTT every X milliseconds
  // if (millis() - lastMQTTMessageSentTime >= 50)
  // {
  //   lastMQTTMessageSentTime = millis();

  //   MQTTMessageCount++;

  //   sendMQTTmessage();
  // }
}

//actually send the MQTT message
void sendMQTTmessage(std::string topic )
{
  // std::string json;
  // serializeJson(joystick, json);

  // checkMQTTconnection();

  // MQTTClient.publish("XBOX360", json.c_str());

  // //Serial.println(json.c_str());

  // json.clear();
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

// std::string &ltrim(std::string &str, const std::string &chars = "\t\n\v\f\r ")
// {
//   str.erase(0, str.find_first_not_of(chars));
//   return str;
// }

// std::string &rtrim(std::string &str, const std::string &chars = "\t\n\v\f\r ")
// {
//   str.erase(str.find_last_not_of(chars) + 1);
//   return str;
// }

// std::string &trim(std::string &str, const std::string &chars = "\t\n\v\f\r ")
// {
//   return ltrim(rtrim(str, chars), chars);
// }

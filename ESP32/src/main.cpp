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
#define SEND_FAIL_SAFES

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;
StaticJsonDocument<850> joystick;
StaticJsonDocument<850> failSafeValues;
std::map<String, unsigned long> failSafeTimes;

unsigned long messageStartTime;
int serialMessageCount = 0;
int MQTTMessageCount = 0;
unsigned long messageLastTime;

//declare functions
void displayMessage(String message);
void updateMessageCount();
void checkMQTTconnection();
void sendMQTTTask(String message);
void setJoystick(String message, String value, const char *JSONKey, bool JSONValue);
void setJoystick(String message, String value, const char *JSONKey, const char *JSONValue);
void setJoystick(String message, String value, const char *JSONKey, long JSONValue);
void setupFailSafeValues();
void checkFailSafe();

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

  //WIFI start up
  displayMessage(std::move("Connecting to WiFi"));
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
  MQTTClient.setBufferSize(5000);

  displayMessage(std::move("Connecting to MQTT server"));
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage(std::move("connect mqtt..."));
  checkMQTTconnection();

  Serial.flush();

  //set some default values
  joystick["make"] = "XBOX360";

  //set fail safe values
  setupFailSafeValues();

  messageStartTime = millis();
  messageLastTime = millis();
}

void loop()
{
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto message = Serial2.readStringUntil('\n');

    serialMessageCount++;

    sendMQTTTask(std::move(message));

    digitalWrite(LED_BUILTIN, LOW);
  }

  //check fail safe
  checkFailSafe();

  yield();
}

void setupFailSafeValues()
{
  failSafeValues["start"] = false;
  failSafeValues["pad_up"] = false;
  failSafeValues["pad_down"] = false;
  failSafeValues["pad_left"] = false;
  failSafeValues["pad_right"] = false;
  failSafeValues["shoulder_left"] = false;
  failSafeValues["shoulder_right"] = false;
  failSafeValues["trigger_left"] = false;
  failSafeValues["trigger_right"] = false;
  failSafeValues["trigger_left_analog"] = 0;
  failSafeValues["trigger_right_analog"] = 0;
  failSafeValues["left_x"] = 0;
  failSafeValues["left_y"] = 0;
  failSafeValues["left_x_mapped"] = 0;
  failSafeValues["left_y_mapped"] = 0;
  failSafeValues["left"] = false;
  failSafeValues["right_x"] = 0;
  failSafeValues["right_y"] = 0;
  failSafeValues["right_x_mapped"] = 0;
  failSafeValues["right_y_mapped"] = 0;
  failSafeValues["right"] = false;
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

void setJoystick(String message, String value, const char *JSONKey, const char *JSONValue)
{
  if (message == value)
  {
    joystick[JSONKey] = JSONValue;

    failSafeTimes.insert(std::pair<String, unsigned long>(JSONKey, millis() + 500));
  }
}

void setJoystick(String message, String value, const char *JSONKey, bool JSONValue)
{
  if (message == value)
  {
    joystick[JSONKey] = JSONValue;

    failSafeTimes.insert(std::pair<String, unsigned long>(JSONKey, millis() + 500));
  }
}

void setJoystick(String message, String value, const char *JSONKey, long JSONValue)
{
  if (message == value)
  {
    joystick[JSONKey] = JSONValue;

    failSafeTimes.insert(std::pair<String, unsigned long>(JSONKey, millis() + 500));
  }
}

void sendMQTTTask(String message)
{
  //deal with the message and make ready for sending
  message.trim();
  message.toUpperCase();

  auto tempValue = message.substring(message.indexOf(':') + 1);
  tempValue.trim();
  auto mqttValue = tempValue.c_str();

  setJoystick(message, "U:C", "pad_up", true);
  setJoystick(message, "U:C", "pad_down", true);
  setJoystick(message, "U:C", "pad_left", true);
  setJoystick(message, "U:C", "pad_right", true);
  setJoystick(message, "U:C", "start", true);
  setJoystick(message, "U:C", "shoulder_left", true);
  setJoystick(message, "U:C", "shoulder_right", true);
  setJoystick(message, "U:C", "trigger_left", true);
  setJoystick(message, "U:C", "trigger_right", true);
  setJoystick(message, "U:C", "trigger_left_analog", true);
  setJoystick(message, "U:C", "trigger_right_analog", true);
  setJoystick(message, "U:C", "left_x", true);
  setJoystick(message, "U:C", "left_y", true);
  setJoystick(message, "U:C", "left_x_mapped", true);
  setJoystick(message, "U:C", "left_y_mapped", true);
  setJoystick(message, "U:C", "left", true);
  setJoystick(message, "U:C", "right_x", true);
  setJoystick(message, "U:C", "right_y", true);
  setJoystick(message, "U:C", "right_x_mapped", true);
  setJoystick(message, "U:C", "right_y_mapped", true);
  setJoystick(message, "U:C", "right", true);

  //only send MQTT every X milliseconds
  if (millis() - messageLastTime >= 100)
  {
    messageLastTime = millis();

    String json;
    serializeJson(joystick, json);

    checkMQTTconnection();

    MQTTClient.publish("XBOX360", json.c_str());

    json.clear();

    MQTTMessageCount++;
  }
}

void checkFailSafe()
{
  std::map<String, unsigned long>::iterator failsafe;

  for (failsafe = failSafeTimes.begin(); failsafe != failSafeTimes.end(); ++failsafe)
  {
    unsigned long failSafeTime = failsafe->second;
    String JSONKey = failsafe->first;

     Serial.print(JSONKey);
     Serial.print(":");
     Serial.print(failSafeTime);
     Serial.print(":");
     Serial.println(millis());

    if (failSafeTime > millis())
    {   
      joystick[JSONKey] = failSafeValues[JSONKey];

      failSafeTimes.erase(failSafeTimes.begin(), failSafeTimes.find(JSONKey));
    }
  }

  //check to see if a change was made
  // checkMQTTconnection();

  // MQTTClient.publish("XBOX360", json.c_str());
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

void displayMessage(String message)
{
  display.clearDisplay(); //for Clearing the display
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(message);

  //https://javl.github.io/image2cpp/
  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();
}

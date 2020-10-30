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
#define FAIL_SAFE_TIME_MS 500

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;
StaticJsonDocument<850> joystick;
// typedef std::pair<String, long> failsafeValue;
// std::map<unsigned long, failsafeValue> failSafeTimes;

typedef std::pair<unsigned long, long> failsafeValue;
std::map<String, failsafeValue> failSafeTimes;

unsigned long messageStartTime = millis();
int serialMessageCount = 0;
int MQTTMessageCount = 0;
unsigned long lastMessageSentTime = millis();

//declare functions
void displayMessage(String message);
void updateMessageCount();
void checkMQTTconnection();
void sendMQTTTask(String message);
void checkFailSafe();
void dealWithButtonClick(const char *JSONKey, bool JSONValue);
void setJoystick(String message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue);

void setup()
{
  // pinMode(23, OUTPUT);
  // digitalWrite(23, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  //turn off bluetooth
  btStop();

#if defined(USE_SERIAL)
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.flush();
#endif

  //baud speed of Arduino Uno sketch
  Serial2.begin(14400);

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

  //set this to be a large enough value to allow a large MQTT message
  MQTTClient.setBufferSize(5000);

  displayMessage(std::move("Connecting to MQTT server"));
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage(std::move("connect mqtt..."));
  checkMQTTconnection();

  //set some default values
  joystick["make"] = "XBOX360";
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
    msg += "FSAFE: ";
    msg += (String)failSafeTimes.size();

    displayMessage(std::move(msg));

    messageStartTime = millis();
    serialMessageCount = 0;
    MQTTMessageCount = 0;
  }
}

void dealWithButtonClick(const char *JSONKey, bool JSONValue)
{
  joystick[JSONKey] = JSONValue;

  failSafeTimes[JSONKey] = std::make_pair(millis() + FAIL_SAFE_TIME_MS, false);

  //Serial.println(failSafeTimes.size());
}

void setJoystick(String message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue)
{
  auto tempValue = message.substring(message.indexOf(':') + 1);
  tempValue.trim();
  auto mqttValue = tempValue.c_str();

  long analogValue = atol(mqttValue);

  auto mappedValue = String(map(analogValue, lowestValue, highestValue, 0, 100));

  joystick[JSONKey] = analogValue;
  joystick[JSONKeyMapped] = mappedValue;

  failSafeTimes[JSONKey] = std::make_pair(millis() + FAIL_SAFE_TIME_MS, 0);
  failSafeTimes[JSONKeyMapped] = std::make_pair(millis() + FAIL_SAFE_TIME_MS, 0);

  //Serial.println(failSafeTimes.size());
}

void setValue(String message, String value, const char *JSONKey)
{
  if (message.startsWith(value))
  {
    auto tempValue = message.substring(message.indexOf(':') + 1);
    tempValue.trim();

    joystick[JSONKey] = tempValue;
  }
}

void sendMQTTTask(String message)
{
  //deal with the message and make ready for sending
  message.trim();
  message.toUpperCase();

  if (message == "BATTERY:")
  {
    setValue(message, "BATTERY:", "battery");
  }
  else if (message == "A:C")
  {
    dealWithButtonClick("button_a", true);
  }
  else if (message == "B:C")
  {
    dealWithButtonClick("button_b", true);
  }
  else if (message == "X:C")
  {
    dealWithButtonClick("button_x", true);
  }
  else if (message == "A:C")
  {
    dealWithButtonClick("button_y", true);
  }
  else if (message == "U:C")
  {
    dealWithButtonClick("pad_up", true);
  }
  else if (message == "D:C")
  {
    dealWithButtonClick("pad_down", true);
  }
  else if (message == "L:C")
  {
    dealWithButtonClick("pad_left", true);
  }
  else if (message == "R:C")
  {
    dealWithButtonClick("pad_right", true);
  }
  else if (message == "START:C")
  {
    dealWithButtonClick("start", true);
  }
  else if (message == "BACK:C")
  {
    dealWithButtonClick("back", true);
  }
  else if (message == "XBOX:C")
  {
    dealWithButtonClick("xbox", true);
  }
  else if (message == "SYNC:C")
  {
    dealWithButtonClick("sync", true);
  }
  else if (message == "L1:C")
  {
    dealWithButtonClick("shoulder_left", true);
  }
  else if (message == "R2:C")
  {
    dealWithButtonClick("shoulder_right", true);
  }
  else if (message == "L3:C")
  {
    dealWithButtonClick("left", true);
  }
  else if (message == "R3:C")
  {
    dealWithButtonClick("right", true);
  }
  else if (message.startsWith("L2:"))
  {
    setJoystick(message, "trigger_left", "trigger_left_mapped", 0, 255);
  }
  else if (message.startsWith("R2:"))
  {
    setJoystick(message, "trigger_right", "trigger_right_mapped", 0, 255);
  }
  else if (message.startsWith("LHX:"))
  {
    setJoystick(message, "left_x", "left_x_mapped", -32768, 32767);
  }
  else if (message.startsWith("LHY:"))
  {
    setJoystick(message, "left_y", "left_y_mapped", -32768, 32767);
  }
  else if (message.startsWith("RHX:"))
  {
    setJoystick(message, "right_x", "right_x_mapped", -32768, 32767);
  }
  else if (message.startsWith("RHY:"))
  {
    setJoystick(message, "right_y", "right_y_mapped", -32768, 32767);
  }

  //only send MQTT every X milliseconds
  if (millis() - lastMessageSentTime >= 100)
  {
    lastMessageSentTime = millis();

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
  std::map<String, failsafeValue>::iterator i = failSafeTimes.begin();

  while (i != failSafeTimes.end())
  {
    failsafeValue values = i->second;
    unsigned long failSafeTime = values.first;

    Serial.print(i->first);
    Serial.print(":");
    Serial.print(failSafeTime);
    Serial.print(":");
    Serial.println(millis());

    if (millis() >= failSafeTime)
    {
      joystick[i->first] = values.second;

      i = failSafeTimes.erase(i);
    }
    else
    {
      ++i;
    }
  }

  //Serial.println(failSafeTimes.size());
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

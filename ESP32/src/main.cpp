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
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define LED_BUILTIN 2
#define USE_SERIAL
#define FAIL_SAFE_TIME_MS 500

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;
StaticJsonDocument<850> joystick;
StaticJsonDocument<850> failSafeValues;
std::map<String, unsigned long> joystickActionTimes;

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
void dealWithReceivedMessage(String message);
void checkFailSafe();
void dealWithButtonClick(const char *JSONKey, bool JSONValue);
void setJoystick(String message, const char *JSONKey, const char *JSONKeyMapped, long lowestValue, long highestValue);
void setupFailSafeValues();

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

  //set up failsafe values & set joystick defaults
  setupFailSafeValues();

  //set some default values
  joystick["make"] = "XBOX360";

  //sorting out OTA
  MDNS.begin("ESP32-XBOX360");
  ArduinoOTA.setHostname("ESP32-XBOX360");
  ArduinoOTA.setPassword("0a3ccdeb-ee30-4485-aaa5-c54303ca5f74");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  //Over the air updates
  ArduinoOTA.handle();

  //sort out the OLED display message
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto message = Serial2.readStringUntil('\n');

    serialMessageCount++;

    lastMessageReceivedTime = millis();

    dealWithReceivedMessage(std::move(message));

    digitalWrite(LED_BUILTIN, LOW);
  }

  //check fail safe
  checkFailSafe();

  //send fail safe if no message has been recieved for 1 second
  if (millis() - lastMessageReceivedTime >= 1000)
  {
    lastMessageReceivedTime = millis();

    sendMQTTmessage();

    //try to tidy up memory
    joystick.garbageCollect();
  }

  //give RTOS some breathing space
  yield();
}

void updateMessageCount()
{
  //send reciever state every one second
  if (millis() - displayMessageStartTime >= 1000)
  {
    String msg = "Serial RX: ";
    msg += (String)serialMessageCount;
    msg += "\n";
    msg += "MQTT RX: ";
    msg += (String)MQTTMessageCount;
    msg += " FSAFE: ";
    msg += (String)joystickActionTimes.size();

    displayMessage(std::move(msg));

    displayMessageStartTime = millis();
    serialMessageCount = 0;
    MQTTMessageCount = 0;
  }
}

void dealWithButtonClick(const char *JSONKey, bool JSONValue)
{
  joystick[JSONKey] = JSONValue;

  joystickActionTimes[JSONKey] = millis() + FAIL_SAFE_TIME_MS;
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

  joystickActionTimes[JSONKey] = millis() + FAIL_SAFE_TIME_MS;
  joystickActionTimes[JSONKeyMapped] = millis() + FAIL_SAFE_TIME_MS;
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

void dealWithReceivedMessage(String message)
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
  if (millis() - lastMQTTMessageSentTime >= 50)
  {
    lastMQTTMessageSentTime = millis();

    MQTTMessageCount++;

    sendMQTTmessage();
  }
}

//actually send the MQTT message
void sendMQTTmessage()
{
  String json;
  serializeJson(joystick, json);

  checkMQTTconnection();

  MQTTClient.publish("XBOX360", json.c_str());

  //Serial.println(json.c_str());

  json.clear();
}

void checkFailSafe()
{
  std::map<String, unsigned long>::iterator i = joystickActionTimes.begin();

  while (i != joystickActionTimes.end())
  {
    unsigned long failSafeTime = i->second;

    // Serial.print(i->first);
    // Serial.print(":");
    // Serial.print(failSafeTime);
    // Serial.print(":");
    // Serial.println(millis());

    if (millis() >= failSafeTime)
    {
      joystick[i->first] = failSafeValues[i->first];

      i = joystickActionTimes.erase(i);
    }
    else
    {
      ++i;
    }
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

void setupFailSafeValues()
{
  failSafeValues["start"] = false;
  failSafeValues["xbox"] = false;
  failSafeValues["sync"] = false;
  failSafeValues["back"] = false;

  failSafeValues["pad_up"] = false;
  failSafeValues["pad_down"] = false;
  failSafeValues["pad_left"] = false;
  failSafeValues["pad_right"] = false;

  failSafeValues["shoulder_left"] = false;
  failSafeValues["shoulder_right"] = false;

  failSafeValues["trigger_left"] = 0;
  failSafeValues["trigger_left_mapped"] = 0;

  failSafeValues["trigger_right"] = 0;
  failSafeValues["trigger_right_mapped"] = 0;

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

  failSafeValues["button_a"] = false;
  failSafeValues["button_b"] = false;
  failSafeValues["button_x"] = false;
  failSafeValues["button_y"] = false;

  //create a copy for the joystick
  joystick = failSafeValues;
}
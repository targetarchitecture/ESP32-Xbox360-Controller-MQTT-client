#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include <xboxlogo.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <map>
#include <topics.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <list>
#include <cmath>

#define LED_BUILTIN 2
#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

unsigned long displayMessageStartTime = millis();
uint64_t serialMessageCount = 0;
uint64_t MQTTMessageCount = 0;

constexpr double pi = 3.14159265358979323846;

// declare functions
void displayMessage(std::string message);
void updateMessageCount();
void checkMQTTconnection();
void dealWithReceivedMessage(const std::string message);
double getAngleFromXY(float XAxisValue, float YAxisValue);
std::string convertXYtoDirection(float X, float Y);
float mapf(float value, float istart, float istop, float ostart, float ostop);
void convertPadtoDirection(std::string msg, const char *topic);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  // turn off bluetooth
  btStop();

#if defined(USE_SERIAL)
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.flush();
#endif

  // baud speed of Arduino Uno sketch
  Serial2.begin(115200);

  // Init I2C bus & OLED
  Wire.begin();

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true) == false)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
    ESP.restart();
  }

  // WIFI start up
  displayMessage("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    displayMessage(".");
  }

  displayMessage("Connected! IP address: ");
  displayMessage(WiFi.localIP().toString().c_str());

  Serial.println("");
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // set this to be a large enough value to allow a large MQTT message
  MQTTClient.setBufferSize(5000);

  displayMessage("Connecting to MQTT server");
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage("connect mqtt...");
  checkMQTTconnection();
}

void loop()
{
  // sort out the OLED display message
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    String UART = Serial2.readStringUntil('\n');

    std::string message = UART.c_str();

    Serial.println(message.c_str());

    serialMessageCount++;

    dealWithReceivedMessage(message.c_str());

    checkMQTTconnection();

    digitalWrite(LED_BUILTIN, LOW);
  }

  // give some breathing space (arduino only has 5ms delay)
  delay(5);
}

void updateMessageCount()
{
  // send reciever state every one second
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
  // deal with the message and make ready for sending
  String msg = message.c_str();

  msg.trim();
  msg.toUpperCase();

  // check for bad string with more than one command
  auto L1 = msg.length();
  auto msg2 = msg;
  msg2.replace(":", "");
  auto L2 = msg2.length();

  if ((L1 - L2) > 1)
  {
    String err = "Multiple Commands:" + msg;

    MQTTClient.publish(MQTT_ERROR_TOPIC.c_str(), err.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("XRC:"))
  {
    MQTTClient.publish(MQTT_INFO_TOPIC.c_str(), msg.c_str());

    MQTTMessageCount++;

    return;
  }

  std::string controller = "";

  if (msg.startsWith("0#"))
  {
    controller = "1";
  }
  if (msg.startsWith("1#"))
  {
    controller = "2";
  }
  if (msg.startsWith("2#"))
  {
    controller = "3";
  }
  if (msg.startsWith("4#"))
  {
    controller = "4";
  }

  // remove the controller index off the front
  msg = msg.substring(2);

  if (msg.startsWith("XCC:"))
  {
    String topic = MQTT_CONTROLLER_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), msg.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("BAT:"))
  {
    String topic = MQTT_BATTERY_TOPIC;
    topic.replace("{{controller}}", controller.c_str());

    auto battery = msg.substring(msg.indexOf(":") + 1);

    MQTTClient.publish(topic.c_str(), battery.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("BTN:L2"))
  {
    auto triggerMsg = msg.substring(msg.indexOf(",") + 1);

    String topic = MQTT_BUTTON_TOPIC + "/trigger_left_raw";
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), triggerMsg.c_str());

    auto triggerValue_mapped = String(map(atol(triggerMsg.c_str()), 0, 255, 0, 100));

    String topicRaw = MQTT_BUTTON_TOPIC + "/trigger_left";
    topicRaw.replace("{{controller}}", controller.c_str());
    topicRaw.toLowerCase();

    MQTTClient.publish(topicRaw.c_str(), triggerValue_mapped.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("BTN:R2"))
  {
    auto triggerMsg = msg.substring(msg.indexOf(",") + 1);

    String topic = MQTT_BUTTON_TOPIC + "/trigger_right_raw";
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), triggerMsg.c_str());

    auto triggerValue_mapped = String(map(atol(triggerMsg.c_str()), 0, 255, 0, 100));

    String topicRaw = MQTT_BUTTON_TOPIC + "/trigger_right";
    topicRaw.replace("{{controller}}", controller.c_str());
    topicRaw.toLowerCase();

    MQTTClient.publish(topicRaw.c_str(), triggerValue_mapped.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("BTN:"))
  {
    String buttonCSV = msg.substring(msg.indexOf(":") + 1);

    std::istringstream buttons(buttonCSV.c_str());
    std::string button;

    while (std::getline(buttons, button, ','))
    {
      std::string mqttMessage = "press";

      String topic = MQTT_BUTTON_TOPIC + String("/") + button.c_str();
      topic.replace("{{controller}}", controller.c_str());
      topic.toLowerCase();

      MQTTClient.publish(topic.c_str(), mqttMessage.c_str());
    }

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("LH:"))
  {
    String hatCSV = msg.substring(msg.indexOf(":") + 1);

    // Serial.println(hatCSV.c_str());

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

    auto mappedXValue = String(map(X, -32768, 32767, -100, 100));
    auto mappedYValue = String(map(Y, -32768, 32767, -100, 100));

    String topic = MQTT_LEFT_X_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), mappedXValue.c_str());

    topic = MQTT_LEFT_XRAW_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), Xstr.c_str());

    topic = MQTT_LEFT_Y_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), mappedYValue.c_str());

    topic = MQTT_LEFT_YRAW_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), Ystr.c_str());

    MQTTMessageCount++;

    return;
  }

  if (msg.startsWith("RH:"))
  {
    String hatCSV = msg.substring(msg.indexOf(":") + 1);

    // Serial.println(hatCSV.c_str());

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

    auto mappedXValue = String(map(X, -32768, 32767, -100, 100));
    auto mappedYValue = String(map(Y, -32768, 32767, -100, 100));

    String topic = MQTT_RIGHT_XRAW_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), Xstr.c_str());

    topic = MQTT_RIGHT_X_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), mappedXValue.c_str());

    topic = MQTT_RIGHT_YRAW_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), Ystr.c_str());

    topic = MQTT_RIGHT_Y_TOPIC;
    topic.replace("{{controller}}", controller.c_str());
    topic.toLowerCase();

    MQTTClient.publish(topic.c_str(), mappedYValue.c_str());

    MQTTMessageCount++;

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
  display.clearDisplay(); // for Clearing the display
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(message.c_str());

  // https://javl.github.io/image2cpp/
  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();
}

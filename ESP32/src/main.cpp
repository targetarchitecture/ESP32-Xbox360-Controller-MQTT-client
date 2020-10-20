#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include <xboxlogo.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LED_BUILTIN 2
//#define USE_SERIAL

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
PubSubClient MQTTClient;

//std::map<std::string, unsigned long> counters;
unsigned long leftTriggerTime = millis();
unsigned long rightTriggerTime = millis();
unsigned long messageStartTime;
unsigned long messageCount = 0;
unsigned long maxMessageCount = 0; //874 seems to be it (with OLED display this is down to 402), (770 if not writing to serial and writing to OLED every second)

void displayMessage(String message);
void sendMQTTmessage(String message);
void updateMessageCount();
void checkMQTTconnection();

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
  displayMessage("Connecting to WiFi"); // + (String)ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    displayMessage(".");
  }

  displayMessage("Connected! IP address: ");
  displayMessage(WiFi.localIP().toString());

  //set this to be a large enough value to allow an MQTT message containing a 22Kb JPEG to be sent
  MQTTClient.setBufferSize(30000);

  displayMessage("Connecting to MQTT server");
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  displayMessage("connect mqtt...");
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

    displayMessage("Max Msg Recieved: " + (String)maxMessageCount);

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

  auto mqttMessage = message.c_str();
  auto tempValue = message.substring(message.indexOf(':') + 1);
  auto mqttValue = tempValue.c_str();

  if (message == F("click: up"))
  {
    MQTTClient.publish("XBOX360/1/D-Pad/Up", "Click");
  }
  if (message == F("click: down"))
  {
    MQTTClient.publish("XBOX360/1/D-Pad/Down", "Click");
  }
  if (message == F("click: left"))
  {
    MQTTClient.publish("XBOX360/1/D-Pad/Left", "Click");
  }
  if (message == F("click: right"))
  {
    MQTTClient.publish("XBOX360/1/D-Pad/Up", "Click");
  }

  // if (message == F("press: r1"))
  // {
  //   MQTTClient.publish("XBOX360/1/Bumper/Right", "Press");
  // }

  // if (message == F("press: l1"))
  // {
  //   MQTTClient.publish("XBOX360/1/Bumper/Left", "Press");
  // }

  if (message == F("click: l1"))
  {
    MQTTClient.publish("XBOX360/1/Bumper/Left", "Click");
  }

  if (message == F("click: r1"))
  {
    MQTTClient.publish("XBOX360/1/Bumper/Right", "Click");
  }

  if (message == F("click: a"))
  {
    MQTTClient.publish("XBOX360/1/Button/A", "Click");
  }
  if (message == F("click: b"))
  {
    MQTTClient.publish("XBOX360/1/Button/B", "Click");
  }
  if (message == F("click: x"))
  {
    MQTTClient.publish("XBOX360/1/Button/X", "Click");
  }
  if (message == F("click: y"))
  {
    MQTTClient.publish("XBOX360/1/Button/Y", "Click");
  }

  if (message == F("click: xbox"))
  {
    MQTTClient.publish("XBOX360/1/Button/XBOX", "Click");
  }
  if (message == F("click: back"))
  {
    MQTTClient.publish("XBOX360/1/Button/Back", "Click");
  }
  if (message == F("click: start"))
  {
    MQTTClient.publish("XBOX360/1/Button/Start", "Click");
  }
  if (message == F("click: sync"))
  {
    MQTTClient.publish("XBOX360/1/Button/SYNC", "Click");
  }

  if (message.startsWith(F("battery:")))
  {
    MQTTClient.publish("XBOX360/1/Battery", mqttValue);
  }

  if (message.startsWith(F("l2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - leftTriggerTime > 200)
    {
      leftTriggerTime = millis();

      long trigger = atol(mqttValue);

      auto mappedValue = String(map(trigger, 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/1/Trigger/Left", mappedValue.c_str());
    }
  }

  //if time between triggers is greater than 500 milliseconds then send zero
  if (millis() - leftTriggerTime > 500)
  {
    leftTriggerTime = millis();
    MQTTClient.publish("XBOX360/1/Trigger/Left", "0");
  }

  if (message.startsWith(F("r2:")))
  {
    //if time between triggers is greater than 100 milliseconds then send
    if (millis() - rightTriggerTime > 200)
    {
      rightTriggerTime = millis();

      auto mappedValue = String(map(atol(mqttValue), 0, 255, 0, 100));

      MQTTClient.publish("XBOX360/1/Trigger/Right", mappedValue.c_str());
    }
  }

  //if time between triggers is greater than 500 milliseconds then send zero
  if (millis() - rightTriggerTime > 500)
  {
    rightTriggerTime = millis();
    MQTTClient.publish("XBOX360/1/Trigger/Right", "0");
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

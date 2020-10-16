#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include "SSD1306.h"
#include <PubSubClient.h>

#define LED_BUILTIN 2

WiFiClient client;
SSD1306 display(0x3c, 21, 22);
QueueHandle_t queue;
TaskHandle_t displayMessageTask;
PubSubClient MQTTClient;

unsigned long messageStartTime;
unsigned long messageCount = 0;
unsigned long maxMessageCount = 0; //874 seems to be it (with OLED display this is down to 402), (770 if not writing to serial and writing to OLED every second)

void displayMessage(void *parameter);
void sendMQTTmessage(String message);
void updateMessageCount();

void setup()
{
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  // Init I2C bus & OLED
  Wire.begin();

  Serial.begin(115200);
  Serial2.begin(115200);

  //turn off bluetooth
  btStop();

  if (!display.init())
  {
    Serial.println(F("SSD1306 allocation failed"));
    delay(10000);
    ESP.restart();
  }

  display.flipScreenVertically();
  display.invertDisplay();

  //WIFI start up
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }

  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  //set this to be a large enough value to allow an MQTT message containing a 22Kb JPEG to be sent
  MQTTClient.setBufferSize(30000);

  Serial.println("Connecting to MQTT server");
  MQTTClient.setClient(client);
  MQTTClient.setServer(MQTT_SERVER, 1883);

  Serial.println("connect mqtt...");
  if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_KEY))
  {
    Serial.println("Connected to MQTT server");
  }

  // Create the queue with
  queue = xQueueCreate(5, sizeof(String));

  xTaskCreatePinnedToCore(
      displayMessage,      /* Function to implement the task */
      "Display Message",   /* Name of the task */
      5000,                /* Stack size in words */
      NULL,                /* Task input parameter */
      2,                   /* Priority of the task */
      &displayMessageTask, /* Task handle. */
      1);                  /* Core where the task should run */

  messageStartTime = millis();
}

void loop()
{
  updateMessageCount();

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto message = Serial2.readStringUntil('\n');

    Serial.println(message);

    sendMQTTmessage(message);

    messageCount++;

    digitalWrite(LED_BUILTIN, LOW);
  }

  if (!MQTTClient.connected())
  {
    if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_KEY))
    {
      Serial.println("Reconnected");
    }
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
    //Serial.print("Max Messages Recieved: " + (String)maxMessageCount);
    Serial.println("Max Messages Recieved: " + (String)maxMessageCount);

    String message = "Max: " + (String)maxMessageCount;

    xQueueSend(queue, &message, portMAX_DELAY);

    messageCount = 0;
  }
}

void displayMessage(void *parameter)
{
  for (;;)
  {
    String message;
    xQueueReceive(queue, &message, portMAX_DELAY);

    Serial.println("Worker - reading " + message);

    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, message);
    display.display();
  }
}

void sendMQTTmessage(String message)
{
  if (MQTTClient.connected())
  {
    //MQTTClient.publish(MQTT_TOPIC, fb->buf, fb_len);
  }
  else
  {
    if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_KEY))
    {
      Serial.println("Reconnected");
    }
  }
}
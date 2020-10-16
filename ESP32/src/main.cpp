#include <Arduino.h>
//#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include "SSD1306.h"
#include <PubSubClient.h>

#define LED_BUILTIN 2

// Set the OLED parameters
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

SSD1306 display(0x3c, 21, 22);

unsigned long messageStartTime;
unsigned long messageCount = 0;
unsigned long maxMessageCount = 0; //874 seems to be it (with OLED display this is down to 402), (770 if not writing to serial and writing to OLED every second)

void setup()
{
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  // Init I2C bus & OLED
  Wire.begin();

  //Serial.begin(115200);
  Serial2.begin(115200);

  //turn off bluetooth
  btStop();

  if (!display.init())
  {
    //Serial.println(F("SSD1306 allocation failed"));
    delay(10000);
    ESP.restart();
  }

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0, "Hello World");
  display.display();

  messageStartTime = millis();
}

void loop()
{
  //send reciever state every one second
  if (millis() - messageStartTime >= 1000)
  {
    if (messageCount > maxMessageCount)
    {
      maxMessageCount = messageCount;
    }
    messageStartTime = millis();
    //Serial.print(F("Max Messages Recieved: "));
    //Serial.println();

    String message = "Max:" + (String) maxMessageCount;

    display.clear();
    display.drawString(0, 0, message);
    display.display();

    messageCount = 0;
  }

  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto character = Serial2.readStringUntil('\n');

    //Serial.println(character);

    messageCount++;

    digitalWrite(LED_BUILTIN, LOW);
  }
}
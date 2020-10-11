#include <Arduino.h>

#define LED_BUILTIN 22

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  if (Serial2.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);

    auto character = Serial2.readStringUntil('\n');

    Serial.println(character);

    digitalWrite(LED_BUILTIN, LOW);
  }
}
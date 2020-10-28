/*
  Based on example sketch for the Xbox Wireless Reciver library - developed by Kristian Lauszus
  For more information see the blog post: http://blog.tkjelectronics.dk/2012/12/xbox-360-receiver-added-to-the-usb-host-library/ or
*/
#include <Arduino.h>
#include <XBOXRECV.h>
#include <SPI.h>
#include <avr/wdt.h>

USB Usb;
XBOXRECV Xbox(&Usb);

//timing and state variables
unsigned long rumbleStartTime;
bool rumbleState = false;
unsigned long ledOnStartTime;
bool ledOnState = false;
unsigned long batteryStateTime;
unsigned long connectionStateTime;
unsigned long controllerStateTime;
unsigned long baudRate = 14400; // 115200;


void setup()
{
  //disable the watchdog
  wdt_disable();

  //start USB
  Serial.begin(baudRate);

  //start two second watchdog timer
  wdt_enable(WDTO_2S);

  if (Usb.Init() == -1)
  {
    Serial.println(F("STOP"));
    while (1)
      ; //halt and wait for watchdog
  }

  //Serial.println(F("Xbox Wireless Receiver Library Started"));
  Serial.println(F("START"));

  //set start times
  rumbleStartTime = millis();
  ledOnStartTime = millis();
  batteryStateTime = millis();
  connectionStateTime = millis() - 300000;
  controllerStateTime = millis() - 200000;

  //reset watchdog timer
  wdt_reset();
}

void loop()
{
  Usb.Task();

  //send reciever state every 10 seconds
  if (millis() - connectionStateTime >= 10000)
  {
    connectionStateTime = millis();
    Serial.print(F("XRC:"));
    Serial.println(Xbox.XboxReceiverConnected);
  }

  if (Xbox.XboxReceiverConnected)
  {
    //We'll deal with multiple controllers another day
    // for (uint8_t i = 0; i < 4; i++)
    // {
    uint8_t i = 0;

    //send controller state every X seconds
    if (millis() - controllerStateTime >= 10000)
    {
      controllerStateTime = millis();
      Serial.print(F("XCC:"));
      Serial.println(Xbox.Xbox360Connected[i]);
    }

    if (Xbox.Xbox360Connected[i])
    {
      //send battery state every 10 seconds
      if (millis() - batteryStateTime >= 10000) //test whether the period has elapsed
      {
        batteryStateTime = millis();
        Serial.print(F("Battery:"));
        Serial.println(Xbox.getBatteryLevel(i)); // The battery level in the range 0-3
      }

      //LED fail safe, turn off after 60 seconds
      if (ledOnState == true)
      {
        if (millis() - ledOnStartTime >= 60000) //test whether the period has elapsed
        {
          Xbox.setLedOff(i);
          ledOnState = false;
        }
      }

      //sort out rumbles...
      //rumble fail safe , turn off after 10 seconds
      if (rumbleState == true)
      {
        if (millis() - rumbleStartTime >= 10000) //test whether the period has elapsed
        {
          Xbox.setRumbleOff(i);
          rumbleState = false;
        }
      }

      //see if there is a command on the Serial line
      if (Serial.available())
      {
        String command = Serial.readStringUntil('\n');

        // Serial.print("Echo:'");
        // Serial.print(command);
        // Serial.println("'");

        if (command.startsWith("setBigRumbleOn:") == true)
        {
          String value = command.substring(command.indexOf(':') + 1);

          // Serial.print("Value Recieved '");
          // Serial.print(value.toInt());
          // Serial.println("'");

          Xbox.setRumbleOn(value.toInt(), 0, i);
          rumbleState = true;
          rumbleStartTime = millis();
        }
        if (command.startsWith("setSmallRumbleOn:") == true)
        {
          String value = command.substring(command.indexOf(':') + 1);

          Xbox.setRumbleOn(0, value.toInt(), i);
          rumbleState = true;
          rumbleStartTime = millis();
        }
        if (command.startsWith("setBothRumbleOn:") == true)
        {
          String value = command.substring(command.indexOf(':') + 1);

          Xbox.setRumbleOn(value.toInt(), value.toInt(), i);
          rumbleState = true;
          rumbleStartTime = millis();
        }
        if (command == "setRumbleOff")
        {
          Xbox.setRumbleOff(i);
          rumbleState = false;
        }

        if (command == "setLedOn:1")
        {
          Xbox.setLedOn(LED1, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedOn:2")
        {
          Xbox.setLedOn(LED2, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedOn:3")
        {
          Xbox.setLedOn(LED3, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedOn:4")
        {
          Xbox.setLedOn(LED4, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedMode:All")
        {
          Xbox.setLedOn(ALL, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedMode:Alternating")
        {
          Xbox.setLedMode(ALTERNATING, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedMode:Rotating")
        {
          Xbox.setLedMode(ROTATING, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedMode:FastBlink")
        {
          Xbox.setLedMode(FASTBLINK, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if (command == "setLedMode:SlowBlink")
        {
          Xbox.setLedMode(SLOWBLINK, i);
          ledOnState = true;
          ledOnStartTime = millis();
        }

        if ((command == "setLedOff") || (command == "setLedMode:Off"))
        {
          //Xbox.setLedOn(ALL, i);
          Xbox.setLedOff(i);
          ledOnState = false;
        }
      }

      //now get the button and stick states
      if (Xbox.getButtonPress(L2, i))
      {
        Serial.print("L2:");
        Serial.println(Xbox.getButtonPress(L2, i));
      }

      if (Xbox.getButtonPress(R2, i))
      {
        Serial.print("R2:");
        Serial.println(Xbox.getButtonPress(R2, i));
      }

      if (Xbox.getAnalogHat(LeftHatX, i) > 7500 || Xbox.getAnalogHat(LeftHatX, i) < -7500 || Xbox.getAnalogHat(LeftHatY, i) > 7500 || Xbox.getAnalogHat(LeftHatY, i) < -7500 || Xbox.getAnalogHat(RightHatX, i) > 7500 || Xbox.getAnalogHat(RightHatX, i) < -7500 || Xbox.getAnalogHat(RightHatY, i) > 7500 || Xbox.getAnalogHat(RightHatY, i) < -7500)
      {
        if (Xbox.getAnalogHat(LeftHatX, i) > 7500 || Xbox.getAnalogHat(LeftHatX, i) < -7500)
        {
          Serial.print(F("LHX:"));
          Serial.println(Xbox.getAnalogHat(LeftHatX, i));
        }
        if (Xbox.getAnalogHat(LeftHatY, i) > 7500 || Xbox.getAnalogHat(LeftHatY, i) < -7500)
        {
          Serial.print(F("LHY:"));
          Serial.println(Xbox.getAnalogHat(LeftHatY, i));
        }
        if (Xbox.getAnalogHat(RightHatX, i) > 7500 || Xbox.getAnalogHat(RightHatX, i) < -7500)
        {
          Serial.print(F("RHX:"));
          Serial.println(Xbox.getAnalogHat(RightHatX, i));
        }
        if (Xbox.getAnalogHat(RightHatY, i) > 7500 || Xbox.getAnalogHat(RightHatY, i) < -7500)
        {
          Serial.print(F("RHY:"));
          Serial.println(Xbox.getAnalogHat(RightHatY, i));
        }
      }

      if (Xbox.getButtonPress(UP, i) == 1)
      {
        Serial.println(F("U:P"));
      }
      if (Xbox.getButtonPress(DOWN, i) == 1)
      {
        Serial.println(F("D:P"));
      }
      if (Xbox.getButtonPress(LEFT, i) == 1)
      {
        Serial.println(F("L:P"));
      }
      if (Xbox.getButtonPress(RIGHT, i) == 1)
      {
        Serial.println(F("R:P"));
      }

      if (Xbox.getButtonClick(UP, i))
      {
        Serial.println(F("U:C"));
      }
      if (Xbox.getButtonClick(DOWN, i))
      {
        Serial.println(F("D:C"));
      }
      if (Xbox.getButtonClick(LEFT, i))
      {
        Serial.println(F("L:C"));
      }
      if (Xbox.getButtonClick(RIGHT, i))
      {
        Serial.println(F("R:C"));
      }

      if (Xbox.getButtonClick(START, i))
      {
        Serial.println(F("START:C"));
      }
      if (Xbox.getButtonClick(BACK, i))
      {
        Serial.println(F("BACK:C"));
      }
      if (Xbox.getButtonClick(L3, i))
      {
        Serial.println(F("L3:C"));
      }
      if (Xbox.getButtonClick(R3, i))
      {
        Serial.println(F("R3:C"));
      }
      if (Xbox.getButtonClick(L1, i))
      {
        Serial.println(F("L1:C"));
      }
      if (Xbox.getButtonClick(R1, i))
      {
        Serial.println(F("R1:C"));
      }

      if (Xbox.getButtonClick(XBOX, i))
      {
        Serial.println(F("XBOX:C"));
      }

      if (Xbox.getButtonClick(SYNC, i))
      {
        Serial.println(F("SYNC:C"));
        Xbox.disconnect(i);
      }

      if (Xbox.getButtonPress(L1, i))
      {
        Serial.println(F("L1:P"));
      }
      if (Xbox.getButtonPress(R1, i))
      {
        Serial.println(F("R1:P"));
      }

      if (Xbox.getButtonClick(A, i))
      {
        Serial.println(F("A:C"));
      }
      if (Xbox.getButtonClick(B, i))
      {
        Serial.println(F("B:C"));
      }
      if (Xbox.getButtonClick(X, i))
      {
        Serial.println(F("X:C"));
      }
      if (Xbox.getButtonClick(Y, i))
      {
        Serial.println(F("Y:C"));
      }

      if (Xbox.getButtonPress(A, i) == 1)
      {
        Serial.println(F("A:P"));
      }
      if (Xbox.getButtonPress(B, i) == 1)
      {
        Serial.println(F("B:P"));
      }
      if (Xbox.getButtonPress(X, i) == 1)
      {
        Serial.println(F("X:P"));
      }
      if (Xbox.getButtonPress(Y, i) == 1)
      {
        Serial.println(F("Y:P"));
      }
      //} multiple controller looop
    }

    //reset watchdog timer
    wdt_reset();
  }

  Serial.flush();

  yield();
}

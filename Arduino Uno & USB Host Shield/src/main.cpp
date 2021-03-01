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
unsigned long baudRate = 38400; // 115200;

String dealWithButton(ButtonEnum b, uint8_t controller, String topic);
void dealWithIncomingCommands(uint8_t i);

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
  //connectionStateTime = millis() - 300000;
  //controllerStateTime = millis() - 200000;

  //reset watchdog timer
  wdt_reset();
}

//String msg

void loop()
{
  Usb.Task();

  //send reciever state every 10 seconds
  if (millis() - connectionStateTime >= 10000)
  {
    connectionStateTime = millis();

    String msg = "XRC:" + (String)Xbox.XboxReceiverConnected;

    Serial.println(msg);

    // Serial.print(F("XRC:"));
    // Serial.println(Xbox.XboxReceiverConnected);
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

      String msg = "XCC:" + (String)Xbox.Xbox360Connected[i];

      Serial.println(msg);

      // Serial.print(F("XCC:"));
      // Serial.println(Xbox.Xbox360Connected[i]);
    }

    if (Xbox.Xbox360Connected[i])
    {
      //send battery state every 10 seconds
      if (millis() - batteryStateTime >= 10000) //test whether the period has elapsed
      {
        batteryStateTime = millis();

        // The battery level in the range 0-3
        String msg = "BAT:" + (String)Xbox.getBatteryLevel(i);

        Serial.println(msg);

        // Serial.print(F("Battery:"));
        // Serial.println(Xbox.getBatteryLevel(i)); // The battery level in the range 0-3
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
      dealWithIncomingCommands(i);

      //now get the button and stick states
      auto LHX = Xbox.getAnalogHat(LeftHatX, i);
      auto LHY = Xbox.getAnalogHat(LeftHatY, i);

      auto RHX = Xbox.getAnalogHat(RightHatX, i);
      auto RHY = Xbox.getAnalogHat(RightHatY, i);

      if (LHX > 8000 || LHX < -8000 || LHY > 8000 || LHY < -8000)
      {
        String msg = "LH:";

        if (LHX > 8000 || LHX < -8000)
        {
          msg += "X,";
          msg += (String)LHX;
        }
        else
        {
          msg += "X,0";
        }

        if (LHY > 8000 || LHY < -8000)
        {
          msg += ",Y,";
          msg += (String)LHY;
        }
        else
        {
          msg += ",Y,0";
        }
        Serial.println(msg);
        Serial.flush();
      }

      if (RHX > 8000 || RHX < -8000 || RHY > 8000 || RHY < -8000)
      {
        String msg = "RH:";

        if (RHX > 8000 || RHX < -8000)
        {
          msg += "X,";
          msg += (String)RHX;
        }
        else
        {
          msg += "X,0";
        }

        if (RHY > 8000 || RHY < -8000)
        {
          msg += ",Y,";
          msg += (String)RHY;
        }
        else
        {
          msg += ",Y,0";
        }

        Serial.println(msg);
        Serial.flush();
      }

      //do the trigger buttons (including dead stick)
      if (Xbox.getButtonPress(L2, i) > 80)
      {
        auto L2val = Xbox.getButtonPress(L2, i);
        String msg = "BTN:R2," + L2val;

        Serial.println(msg);
        Serial.flush();
      }

      if (Xbox.getButtonPress(R2, i) > 80)
      {
        auto R2val = Xbox.getButtonPress(R2, i);
        String msg = "BTN:R2," + R2val;

        Serial.println(msg);
        Serial.flush();
      }

      //now do the buttons
      String msg = "";

      msg += dealWithButton(UP, i, "UP");
      msg += dealWithButton(DOWN, i, "DOWN");
      msg += dealWithButton(LEFT, i, "LEFT");
      msg += dealWithButton(RIGHT, i, "RIGHT");

      msg += dealWithButton(START, i, "START");
      msg += dealWithButton(BACK, i, "BACK");
      msg += dealWithButton(L3, i, "L3");
      msg += dealWithButton(R3, i, "R3");

      msg += dealWithButton(XBOX, i, "XBOX");
      msg += dealWithButton(SYNC, i, "SYNC");

      msg += dealWithButton(L1, i, "L1");
      msg += dealWithButton(R1, i, "R1");

      msg += dealWithButton(A, i, "A");
      msg += dealWithButton(B, i, "B");

      msg += dealWithButton(X, i, "X");
      msg += dealWithButton(Y, i, "Y");

      if (msg.length() > 0)
      {
        msg = "BTN:" + msg;
        msg = msg.substring(0, msg.length() - 1);

        Serial.println(msg);
        Serial.flush();

        msg = "";
      }

      if (Xbox.getButtonClick(SYNC, i))
      {
        Xbox.disconnect(i);
      }
    }
  }

  //reset watchdog timer
  wdt_reset();

  delay(5);
}

String dealWithButton(ButtonEnum b, uint8_t controller, String topic)
{
  if (Xbox.getButtonPress(b, controller) == 1 || Xbox.getButtonClick(b, controller) == true)
  {
    return topic + ",";
  }

  return "";
}

void dealWithIncomingCommands(uint8_t i)
{
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
}
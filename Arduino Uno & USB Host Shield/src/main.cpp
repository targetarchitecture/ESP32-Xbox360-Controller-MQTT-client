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
unsigned long baudRate = 115200;
//unsigned long baudRate = 38400;

String dealWithButton(ButtonEnum b, uint8_t controller, String topic);
void dealWithIncomingCommands(uint8_t i);
void triggers(uint8_t controller);
void leftHat(uint8_t controller);
void rightHat(uint8_t controller);
void sendButtonMessage(uint8_t controller);

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

  Serial.println(F("START"));

  //set start times
  rumbleStartTime = millis();
  ledOnStartTime = millis();
  batteryStateTime = millis();

  //reset watchdog timer
  wdt_reset();
}

void loop()
{
  //reset watchdog timer
  wdt_reset();

  delay(15);
  //delay(5);

  //USB task
  Usb.Task();

  //send reciever state every 10 seconds
  if (millis() - connectionStateTime >= 10000)
  {
    connectionStateTime = millis();

    String msg = "XRC:" + (String)Xbox.XboxReceiverConnected;

    Serial.println(msg);
    Serial.flush();
    delay(1);
  }

  if (Xbox.XboxReceiverConnected)
  {
    //We'll deal with multiple controllers another day
    for (size_t controller = 0; controller < 2; controller++)
    {
      //send controller state every X seconds
      if (millis() - controllerStateTime >= 10000)
      {
        controllerStateTime = millis();

        String msg;
        msg.concat(controller);
        msg.concat("#XCC:");
        msg.concat(Xbox.Xbox360Connected[controller]);

        Serial.println(msg);
      }

      if (Xbox.Xbox360Connected[controller])
      {
        //send battery state every 10 seconds
        if (millis() - batteryStateTime >= 10000) //test whether the period has elapsed
        {
          batteryStateTime = millis();

          // The battery level in the range 0-3
          String msg;
          msg.concat(controller);
          msg.concat("#BAT:");
          msg.concat(Xbox.getBatteryLevel(controller));

          Serial.println(msg);
          Serial.flush();
          delay(1);
        }

        //LED fail safe, turn off after 60 seconds
        if (ledOnState == true)
        {
          if (millis() - ledOnStartTime >= 60000) //test whether the period has elapsed
          {
            Xbox.setLedOff(controller);
            ledOnState = false;
          }
        }

        //sort out rumbles...
        //rumble fail safe , turn off after 10 seconds
        if (rumbleState == true)
        {
          if (millis() - rumbleStartTime >= 10000) //test whether the period has elapsed
          {
            Xbox.setRumbleOff(controller);
            rumbleState = false;
          }
        }

        //see if there is a command on the Serial line
        dealWithIncomingCommands(controller);

        //now get the button and stick states
        leftHat(controller);
        rightHat(controller);

        //do the trigger buttons (including dead stick)
        triggers(controller);

        //now do the buttons
        sendButtonMessage(controller);

        if (Xbox.getButtonClick(SYNC, controller))
        {
          Xbox.disconnect(controller);
        }
      }
    }
  }
}

void sendButtonMessage(uint8_t controller)
{
  String btnValues;

  btnValues.concat(dealWithButton(UP, controller, "UP"));
  btnValues.concat(dealWithButton(DOWN, controller, "DOWN"));
  btnValues.concat(dealWithButton(LEFT, controller, "LEFT"));
  btnValues.concat(dealWithButton(RIGHT, controller, "RIGHT"));

  btnValues.concat(dealWithButton(START, controller, "START"));
  btnValues.concat(dealWithButton(BACK, controller, "BACK"));
  btnValues.concat(dealWithButton(L3, controller, "L3"));
  btnValues.concat(dealWithButton(R3, controller, "R3"));

  btnValues.concat(dealWithButton(XBOX, controller, "XBOX"));
  btnValues.concat(dealWithButton(SYNC, controller, "SYNC"));

  btnValues.concat(dealWithButton(L1, controller, "L1"));
  btnValues.concat(dealWithButton(R1, controller, "R1"));

  btnValues.concat(dealWithButton(A, controller, "A"));
  btnValues.concat(dealWithButton(B, controller, "B"));

  btnValues.concat(dealWithButton(X, controller, "X"));
  btnValues.concat(dealWithButton(Y, controller, "Y"));

  if (btnValues.length() > 0)
  {
    btnValues = btnValues.substring(0, btnValues.length() - 1);
    String msg;

    msg.concat(controller);
    msg.concat("#BTN:");
    msg.concat(btnValues);

    Serial.println(msg);
    Serial.flush();
    delay(1);
  }
}

void triggers(uint8_t controller)
{
  if (Xbox.getButtonPress(L2, controller) > 85)
  {
    auto L2val = Xbox.getButtonPress(L2, controller);

    String L2msg;
    L2msg.concat(controller);
    L2msg.concat("#BTN:L2,");
    L2msg.concat(L2val);

    Serial.println(L2msg);
    Serial.flush();
    delay(1);
  }

  if (Xbox.getButtonPress(R2, controller) > 85)
  {
    auto R2val = Xbox.getButtonPress(R2, controller);

    String R2msg;
    R2msg.concat(controller);
    R2msg.concat("#BTN:R2,");
    R2msg.concat(R2val);

    Serial.println(R2msg);
    Serial.flush();
    delay(1);
  }
}

void rightHat(uint8_t controller)
{
  auto RHX = Xbox.getAnalogHat(RightHatX, controller);
  auto RHY = Xbox.getAnalogHat(RightHatY, controller);

  if (RHX > 8000 || RHX < -8000 || RHY > 14500 || RHY < -8000)
  {
    String msg;

    msg.concat(controller);
    msg.concat("#RH:");

    if (RHX > 8000 || RHX < -8000)
    {
      msg.concat("X,");
      msg.concat(RHX);
    }
    else
    {
      msg.concat("X,0");
    }

    if (RHY > 8000 || RHY < -8000)
    {
      msg.concat(",Y,");
      msg.concat(RHY);
    }
    else
    {
      msg.concat(",Y,0");
    }

    Serial.println(msg);
    Serial.flush();
    delay(1);
  }
}

void leftHat(uint8_t controller)
{
  auto LHX = Xbox.getAnalogHat(LeftHatX, controller);
  auto LHY = Xbox.getAnalogHat(LeftHatY, controller);

  if (LHX > 8000 || LHX < -11200 || LHY > 8000 || LHY < -8000)
  {
    String msg;

    msg.concat(controller);
    msg.concat("#LH:");

    if (LHX > 8000 || LHX < -8000)
    {
      msg.concat("X,");
      msg.concat(LHX);
    }
    else
    {
      msg.concat("X,0");
    }

    if (LHY > 8000 || LHY < -8000)
    {
      msg.concat(",Y,");
      msg.concat(LHY);
    }
    else
    {
      msg.concat(",Y,0");
    }
    Serial.println(msg);
    Serial.flush();
  }
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

    if (command.startsWith("setBigRumbleOn:") == true)
    {
      String value = command.substring(command.indexOf(':') + 1);

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
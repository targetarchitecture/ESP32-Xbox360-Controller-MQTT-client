#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <Wire.h>
#include <xboxlogo.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LED_BUILTIN 2

WiFiClient client;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
// QueueHandle_t messageQueue;
// TaskHandle_t displayMessageTask;
PubSubClient MQTTClient;

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

  // Init I2C bus & OLED
  Wire.begin();

  Serial.begin(115200);
  Serial2.begin(115200);

  //turn off bluetooth
  btStop();

  // Create the queues with
  //messageQueue = xQueueCreate(2, sizeof(String));

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C) == false)
  {
    delay(1000);
    ESP.restart();
  }
  else
  {
    display.clearDisplay(); //for Clearing the display
    //https://javl.github.io/image2cpp/
    display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
    display.display();
  }

  //WIFI start up
  displayMessage("Connecting to " + (String)ssid);
  //Serial.printf("Connecting to %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
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

   Serial.println(message);

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
    //Serial.print("Max Messages Recieved: " + (String)maxMessageCount);
    displayMessage("Max Msg Recieved: " + (String)maxMessageCount);

    //String message = "Max: " + (String)maxMessageCount;

    // displayMessage(message);

    messageCount = 0;
  }
}

void displayMessage(String message)
{
  Serial.println(message);

  display.clearDisplay(); //for Clearing the display

  //https://javl.github.io/image2cpp/

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print((String)message);

  display.drawBitmap(0, 16, xboxLogo, 128, 48, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.display();
}

void sendMQTTmessage(String message)
{
  checkMQTTconnection();

  message.trim();

  MQTTClient.publish("value", message.c_str());
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




// void loopPS2(byte vibrate)
// {
//     /* You must Read Gamepad to get new values and set vibration values
//      ps2x.read_gamepad(small motor on/off, larger motor strength from 0-255)
//      if you don't enable the rumble, use ps2x.read_gamepad(); with no values
//      You should call this at least once a second
//    */

//     if (vibrate > 0)
//     {
//         publishMQTTmessage("Vibrate set to " + String(vibrate));
//     }

//     //DualShock Controller
//     ps2x.read_gamepad(false, vibrate); //read controller and set large motor to spin at 'vibrate' speed

//     long left_x_mapped = map(ps2x.Analog(PSS_LX), 0, 255, -100, 100);
//     long left_y_mapped = map(ps2x.Analog(PSS_LY), 0, 255, -100, 100);
//     long right_x_mapped = map(ps2x.Analog(PSS_RX), 0, 255, -100, 100);
//     long right_y_mapped = map(ps2x.Analog(PSS_RY), 0, 255, -100, 100);

//     //Serial.print("PSS_LX:" + String(left_x_mapped) + " PSS_LY:" + String(left_y_mapped));
//     //Serial.println("PSS_RX:" + String(right_x_mapped) + " PSS_RY:" + String(right_y_mapped));

//     // lastCommandSentMillis

//     bool sendMessage = false;

//     //check for centered zone and ignore
//     if (abs(left_x_mapped) > 5 ||
//         abs(left_y_mapped) > 5)
//     {
//         joystick["left_x_mapped"] = left_x_mapped;
//         joystick["left_y_mapped"] = left_y_mapped;

//         joystick["left_x"] = ps2x.Analog(PSS_LX);
//         joystick["left_y"] = ps2x.Analog(PSS_LY);
//     }

//     //check for centered zone and ignore
//     if (abs(right_x_mapped) > 5 ||
//         abs(right_y_mapped) > 5)
//     {
//         joystick["right_x_mapped"] = right_x_mapped;
//         joystick["right_y_mapped"] = right_y_mapped;

//         joystick["right_x"] = ps2x.Analog(PSS_RX);
//         joystick["right_y"] = ps2x.Analog(PSS_RY);
//     }

//     //only set if changed - so store the STATE !

//     if (ps2x.Button(PSB_START) != joystick["start"])
//     {
//         joystick["start"] = ps2x.Button(PSB_START);
//     }

//     if (ps2x.Button(PSB_SELECT) == true)
//     {
//         joystick["ps2_select"] = ps2x.Button(PSB_SELECT);
//     }

//     if (ps2x.Button(PSB_PAD_UP) == true)
//     {
//         joystick["pad_up"] = ps2x.Button(PSB_PAD_UP);
//     }

//     if (ps2x.Button(PSB_PAD_DOWN) == true)
//     {
//         joystick["pad_down"] = ps2x.Button(PSB_PAD_DOWN);
//     }

//     if (ps2x.Button(PSB_PAD_LEFT) == true)
//     {
//         joystick["pad_left"] = ps2x.Button(PSB_PAD_LEFT);
//     }

//     if (ps2x.Button(PSB_PAD_RIGHT) == true)
//     {
//         joystick["pad_right"] = ps2x.Button(PSB_PAD_RIGHT);
//     }

//     if (ps2x.Button(PSB_TRIANGLE) == true)
//     {
//         joystick["action_up"] = ps2x.Button(PSB_TRIANGLE);
//     }

//     if (ps2x.Button(PSB_CROSS) == true)
//     {
//         joystick["action_down"] = ps2x.Button(PSB_CROSS);
//     }

//     if (ps2x.Button(PSB_SQUARE) == true)
//     {
//         joystick["action_left"] = ps2x.Button(PSB_SQUARE);
//     }

//     if (ps2x.Button(PSB_CIRCLE) == true)
//     {
//         joystick["action_right"] = ps2x.Button(PSB_CIRCLE);
//     }

//     if (ps2x.Button(PSB_L1) == true)
//     {
//         joystick["shoulder_left"] = ps2x.Button(PSB_L1);
//     }

//     if (ps2x.Button(PSB_R1) == true)
//     {
//         joystick["shoulder_right"] = ps2x.Button(PSB_R1);
//     }

//     if (ps2x.Button(PSB_L2) == true)
//     {
//         joystick["trigger_left"] = ps2x.Button(PSB_L2);
//     }

//     if (ps2x.Button(PSB_R2) == true)
//     {
//         joystick["trigger_right"] = ps2x.Button(PSB_R2);
//     }

//     if (ps2x.Button(PSB_L3) == true)
//     {
//         joystick["left"] = ps2x.Button(PSB_L3);
//     }

//     if (ps2x.Button(PSB_R3) == true)
//     {
//         joystick["right"] = ps2x.Button(PSB_R3);
//     }

//     // joystick["xbox360_back"] = false;
//     // joystick["xbox360_guide"] = false;
//     // joystick["trigger_left_analog"] = 0;
//     // joystick["trigger_right_analog"] = 0;
//     //}

//     JsonObject joystickObj = joystick.as<JsonObject>();

//     // size_t joystickObjSize = joystickObj.size();
//     // Serial.println(joystickObjSize);

//     if (joystickObj.size() > 0)
//     {
//         joystick["dial"] = dial;
//         joystick["make"] = "PS2";

//         String json;

//         serializeJson(joystick, json);

//         //Serial.println(json);

//         publishMQTTmessage(json);

//         digitalWrite(LED_BUILTIN, LOW); //set LED to flash on

//         lastCommandSentMillis = millis(); //reset timer
//     }
// }

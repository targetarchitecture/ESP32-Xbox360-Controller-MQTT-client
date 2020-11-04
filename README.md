# XBOX360 Wireless MQTT Controller

![Failsafe Film](/images/failsafe.png)

Two years on after I started learning about robotics and electronic engineering. I decided to revisit a problem that I couldn't find a solution for when building Zebra (Ada's first robot). I wanted to use my old Xbox 360 controllers and their wireless receiver to operate robots directly. 

It sounds like a relatively simple problem, however the XBOX360 wireless receiver is very picky about the host controller it's plugged into, and the host controller can also be very picky about the device that it's plugged into. After weeks of failing to get this to work, I packed the XBOX360 controllers away.

It's 2020 and everything is an IoT MQTT device - even your 15 year old XBOX360 controllers. I still had the original Ardiuno Uno and a USB host shield from Keyestudio that I had purchased to build Zebra. The Uno and the USB shield had always been a stable combination when using the XBOX360 wireless controller - however at the time I lacked the knowledge to turn this into a working robot controller.

The simplest solution was to connect the Arduino Uno to the ESP32 via the arduinos serial pins and to the second serial (UART) on the ESP32. The ESP32 is much easier to use in this respect as it has multiple UARTs compared to the ESP8266.

The ESP8266, it claims to have to two UARTs howevers both of them are basically in use - one for programming and the other for the flash memory access.

The ESP32 and the Arduino have differant logic levels (5v & 3.3v), the hardware incorporates a logic level shifter for both the transmit and recieve lines.

I added a small SSD1306 OLED display to show some stats and the XBOX360 logo.





# Actions

<table>
  <tr>
    <th>Serial Command</th>
    <th>Serial Value</th>
    <th>MQTT Topic</th>
    <th>MQTT Value</th>
  </tr>

<tr><td>STOP</td><td></td><td></td><td></td></tr>
<tr><td>START</td><td></td><td></td><td></td></tr>
<tr><td>XRC:</td><td>True</td><td></td><td></td></tr>
<tr><td>XCC:</td><td>1</td><td></td><td></td></tr>

<tr><td>Battery:</td><td>0-3</td><td>XBOX360/Battery</td><td>0/3</td></tr>

<tr><td>L2:</td><td>0-255</td><td>XBOX360/Trigger/Left</td><td>0/100</td></tr>
<tr><td>R2:</td><td>0-255</td><td>XBOX360/Trigger/Right</td><td>0/100</td></tr>

<tr><td>LHX:</td><td>-32768/32767</td><td>XBOX360/Stick/Left/X</td><td>-100/100</td></tr>
<tr><td>LHY:</td><td>-32768/32767</td><td>XBOX360/Stick/Left/Y</td><td>-100/100</td></tr>
<tr><td>L3:</td><td>C</td><td>XBOX360/Stick/Left</td><td>Click</td></tr>

<tr><td>RHX:</td><td>-32768/32767</td><td>XBOX360/Stick/Right/X</td><td>-100/100</td></tr>
<tr><td>RHY:</td><td>-32768/32767</td><td>XBOX360/Stick/Right/Y</td><td>-100/100</td></tr>
<tr><td>R3:</td><td>C</td><td>XBOX360/Stick/Right</td><td>Click</td></tr>

<tr><td>U:</td><td>C</td><td>XBOX360/D-Pad/Up</td><td>Click</td></tr>
<tr><td>U:</td><td>P</td><td>XBOX360/D-Pad/Up</td><td>Press</td></tr>
<tr><td>D:</td><td>C</td><td>XBOX360/D-Pad/Down</td><td>Click</td></tr>
<tr><td>D:</td><td>P</td><td>XBOX360/D-Pad/Down</td><td>Press</td></tr>
<tr><td>L:</td><td>C</td><td>XBOX360/D-Pad/Left</td><td>Click</td></tr>
<tr><td>L:</td><td>p</td><td>XBOX360/D-Pad/Left</td><td>Press</td></tr>
<tr><td>R:</td><td>C</td><td>XBOX360/D-Pad/Right</td><td>Click</td></tr>
<tr><td>R:</td><td>P</td><td>XBOX360/D-Pad/Right</td><td>Press</td></tr>

<tr><td>START:</td><td>C</td><td>XBOX360/Button/Start</td><td>Click</td></tr>
<tr><td>BACK:</td><td>C</td><td>XBOX360/Button/Back</td><td>Click</td></tr>
<tr><td>XBOX:</td><td>C</td><td>XBOX360/Button/XBOX</td><td>Click</td></tr>
<tr><td>SYNC:</td><td>C</td><td>XBOX360/Button/SYNC</td><td>Click</td></tr>
 
<tr><td>A:</td><td>C</td><td>XBOX360/Button/A</td><td>Click</td></tr>
<tr><td>B:</td><td>C</td><td>XBOX360/Button/B</td><td>Click</td></tr>
<tr><td>X:</td><td>C</td><td>XBOX360/Button/X</td><td>Click</td></tr>
<tr><td>Y:</td><td>C</td><td>XBOX360/Button/Y</td><td>Click</td></tr>
<tr><td>A:</td><td>P</td><td>XBOX360/Button/A</td><td>Press</td></tr>
<tr><td>B:</td><td>P</td><td>XBOX360/Button/B</td><td>Press</td></tr>
<tr><td>X:</td><td>P</td><td>XBOX360/Button/X</td><td>Press</td></tr>
<tr><td>Y:</td><td>P</td><td>XBOX360/Button/Y</td><td>Press</td></tr>

<tr><td>L1:</td><td>C</td><td>XBOX360/Bumper/Left</td><td>Click</td></tr>
<tr><td>L1:</td><td>P</td><td>XBOX360/Bumper/Left</td><td>Press</td></tr>
<tr><td>R1:</td><td>C</td><td>XBOX360/Bumper/Right</td><td>Click</td></tr>
<tr><td>R1:</td><td>P</td><td>XBOX360/Bumper/Right</td><td>Press</td></tr>

</table>

# Commands

<table>
  <tr>
    <th>MQTT Topic</th>
    <th>MQTT Value</th>
    <th>Serial Command</th>
    <th>Serial Value</th>
  </tr>


  <tr><td>setBigRumbleOn:</td><td>0-3</td><td>XBOX360/1/Battery</td><td>0/3</td></tr>

</table>


setBigRumbleOn: 0-255
setSmallRumbleOn: 0-255
setBothRumbleOn: 0-255
setRumbleOff
setLedOn: 1
setLedOn: 2
setLedOn: 3
setLedOn: 4
setLedMode: All
setLedMode: Alternating
setLedMode: Rotating
setLedMode: FastBlink
setLedMode: SlowBlink
setLedMode: Off
setLedOff
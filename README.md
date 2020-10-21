# xbox360-MQTT-client
A convoluted solution involving an Arduino Uno, USB host controller v1.3 and an ESP32


# Actions

<table>
  <tr>
    <th>Serial Command</th>
    <th>Serial Value</th>
    <th>MQTT Topic</th>
    <th>MQTT Value</th>
  </tr>

<tr><td>Battery:</td><td>0-3</td><td>XBOX360/Battery</td><td>0/3</td></tr>
<tr><td>L2:</td><td>0-255</td><td>XBOX360/Trigger/Left</td><td>0/100</td></tr>
<tr><td>R2:</td><td>0-255</td><td>XBOX360/Trigger/Right</td><td>0/100</td></tr>

<tr><td>LeftHatX:</td><td>-32768/32767</td><td>XBOX360/Stick/Left/X</td><td>-100/100</td></tr>
<tr><td>LeftHatY:</td><td>-32768/32767</td><td>XBOX360/Stick/Left/Y</td><td>-100/100</td></tr>
<tr><td>Click: L3</td><td></td><td>XBOX360/Stick/Left</td><td>Click</td></tr>

<tr><td>RightHatX:</td><td>-32768/32767</td><td>XBOX360/Stick/Right/X</td><td>-100/100</td></tr>
<tr><td>RightHatY:</td><td>-32768/32767</td><td>XBOX360/Stick/Right/Y</td><td>-100/100</td></tr>
<tr><td>Click: R3</td><td></td><td>XBOX360/Stick/Right</td><td>Click</td></tr>

<tr><td>Click: Up</td><td></td><td>XBOX360/D-Pad/Up</td><td>Click</td></tr>
<tr><td>Press: Up</td><td></td><td>XBOX360/D-Pad/Up</td><td>Press</td></tr>
<tr><td>Click: Down</td><td></td><td>XBOX360/D-Pad/Down</td><td>Click</td></tr>
<tr><td>Press: Down</td><td></td><td>XBOX360/D-Pad/Down</td><td>Press</td></tr>
<tr><td>Click: Left</td><td></td><td>XBOX360/D-Pad/Left</td><td>Click</td></tr>
<tr><td>Press: Left</td><td></td><td>XBOX360/D-Pad/Left</td><td>Press</td></tr>
<tr><td>Click: Right</td><td></td><td>XBOX360/D-Pad/Right</td><td>Click</td></tr>
<tr><td>Press: Right</td><td></td><td>XBOX360/D-Pad/Right</td><td>Press</td></tr>

<tr><td>Click: Start</td><td></td><td>XBOX360/Button/Start</td><td>Click</td></tr>
<tr><td>Click: Back</td><td></td><td>XBOX360/Button/Back</td><td>Click</td></tr>
<tr><td>Click: XBOX</td><td></td><td>XBOX360/Button/XBOX</td><td>Click</td></tr>
<tr><td>Click: SYNC</td><td></td><td>XBOX360/Button/SYNC</td><td>Click</td></tr>
 
<tr><td>Click: A</td><td></td><td>XBOX360/Button/A</td><td>Click</td></tr>
<tr><td>Click: B</td><td></td><td>XBOX360/Button/B</td><td>Click</td></tr>
<tr><td>Click: X</td><td></td><td>XBOX360/Button/X</td><td>Click</td></tr>
<tr><td>Click: Y</td><td></td><td>XBOX360/Button/Y</td><td>Click</td></tr>
<tr><td>Press: A</td><td></td><td>XBOX360/Button/A</td><td>Press</td></tr>
<tr><td>Press: B</td><td></td><td>XBOX360/Button/B</td><td>Press</td></tr>
<tr><td>Press: X</td><td></td><td>XBOX360/Button/X</td><td>Press</td></tr>
<tr><td>Press: Y</td><td></td><td>XBOX360/Button/Y</td><td>Press</td></tr>

<tr><td>Click: L1</td><td></td><td>XBOX360/Bumper/Left</td><td>Click</td></tr>
<tr><td>Press: L1</td><td></td><td>XBOX360/Bumper/Left</td><td>Press</td></tr>
<tr><td>Click: R1</td><td></td><td>XBOX360/Bumper/Right</td><td>Click</td></tr>
<tr><td>Press: R1</td><td></td><td>XBOX360/Bumper/Right</td><td>Press</td></tr>

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
# Stranger Things LED Wall (ESP8266) 💡🚲

A web-controlled, interactive LED wall inspired by Stranger Things. Built using a Wemos D1 Mini (ESP8266) and a 50-pixel WS2811/WS2812B addressable LED strip.

## Features
* **Web UI Control:** Connect to the ESP8266's IP address from your phone to control the wall.
* **Spell Mode:** Type a message and watch the wall spell it out letter by letter.
* **Live Keypad:** Tap letters on your phone to instantly light them up on the wall.
* **Upside Down Ambience:** Ambient LEDs breathe a creepy red and randomly flicker.
* **Lightning Strike:** Trigger a massive blue/white lightning flash across the room.
* **Serpentine Routing:** Code handles zigzag (serpentine) physical LED wiring automatically.

## Hardware Used
* Wemos D1 Mini (ESP-12F)
* 50x Addressable RGB LEDs (WS2811 or WS2812B)
* 5V Power Supply (Recommended for full brightness)

## Wiring Map
* **Data Pin:** `D6` on the D1 Mini
* **Letters:** Mapped to Odd physical LEDs (1, 3, 5...)
* **Ambient:** Mapped to Even physical LEDs (0, 2, 4...)
* **Row 1 (A-H):** Left to Right
* **Row 2 (I-Q):** Right to Left (Serpentine)
* **Row 3 (R-Z):** Left to Right

## Setup Instructions
1. Open `Stranger_Wall.ino` in the Arduino IDE.
2. Install the `Adafruit_NeoPixel` library.
3. Change `WIFI_SSID` and `WIFI_PASS` to your network credentials.
4. Upload to your D1 Mini.
5. Open the Serial Monitor (115200 baud) to find the assigned IP address.
6. Type that IP address into your phone's browser to open the control panel.

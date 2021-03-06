# Homie RGB LED controller sketch (HSI)

## Required Libraries
[Homie For ESP8266](http://marvinroger.github.io/homie-esp8266/)

ArduinJSON Library

## What is this for
It is for use with RGB LEDs, allowing you to send standard MQTT, or MQTT JSON commands. It works with Home Assistant natively, and will work with HomeKit using [MQTTTHING](https://github.com/arachnetech/homebridge-mqttthing#readme) for [HomeBridge](https://homebridge.io). It uses HSI colors, so you can adjust brightness with minimal color change. And it supports transition( in seconds) when sending commands in JSON

## How To Use
- Set LED pins, and preferences in "config.h"
- Upload the sketch to the board
- Open the [Homie Config](http://marvinroger.github.io/homie-esp8266/configurators/v2/) site in your browser
- Boot up, look for (and connect) to a WiFi network that looks something like lh-#########
- It will take a few minutes for the config site to see the device, but when it does, enter your WiFi/MQTT settings
- Now the device is on the network, all that's left is to setup [HomeBridge](https://homebridge.io), and [Home Assistant](https://home-assistant.io)

## Commands
To send a state to any topic just add "/set" to the end.
So, to get JSON state "**/light/JSON", and to set that topic "**/light/JSON/set"

JSON:

Topic: **/light/JSON

```json
{
    "state": "ON",
    "brightness": 255,
    "color": {
        "h": 360,
        "s": 100
    },
    "transition": 5
}
```

MQTT Topics:

State: **/light/on ( true/false )

Brightness: **/light/brightness ( 0-255 )

Brightness Percent: **/light/brightnesspct ( 0-100 )

Hue/Saturation: **/light/hs ( hue (0-360),sat (0-100) )

Hue/Saturation/Brightness Percent: **/light/hsb ( h (0-360),s (0-100),b (0-100) )

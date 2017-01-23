# ESP8622 MQTT RGB LEDs Using JSON for Home Assistant

This project is primarliy a fork of [corbanmailloux](https://home-assistant.io/components/light.mqtt_json/)'s repository [esp-mqtt-rgb-led](https://github.com/corbanmailloux/esp-mqtt-rgb-led) with a means of controlling LED Strips or neopixels.

After making my own RGB led lamp with a WS2812B Ring, a NodeMCU V3, some hot glue and half a bird feeder I wanted to intergrate it with home assistant.

![alt text](https://raw.githubusercontent.com/JammyDodger231/esp-mqtt-rgb-led/master/RGBLamp.png)

I first tried with using a REST approch with HTTP requests but there wasn't an intergration in home assistant, so I was linked to corbanmailloux's repository.

## Installation/Configuration

To set this system up, you need to configure the [MQTT JSON light](https://home-assistant.io/components/light.mqtt_json/) component in Home Assistant and set up a light to control. This guide assumes that you already have Home Assistant set up and running. If not, see the installation guides [here](https://home-assistant.io/getting-started/). As well as these you also have to install the appropriate libraries in your arduino IDE

### In Home Assistant
1. In your `configuration.yaml`, add the following:

    ```yaml
    light:
      + platform: mqtt_json
        name: mqtt_json_light_1
        state_topic: "home/rgb1"
        command_topic: "home/rgb1/set"
        brightness: true
        rgb: true
        optimistic: false
        qos: 0
    ```
2. Set the `name`, `state_topic`, and `command_topic` to values that make sense for you. Make sure these are unique to every light unless you would like the lights on the same group.

3. Restart Home Assistant. Depending on how you installed it, the process differs. For a Raspberry Pi All-in-One install, use `sudo systemctl restart home-assistant.service` (or just restart the Pi).

### The Light Side
For this I used a NodeMCU v3 which used a ESP8266-01 microcontroller. The NodeMCU is bigger than a raw ESP8266 but it has a voltage regulator and is breadboard friendly.
You'll need an ESP set up to work with the Arduino IDE. This needs to be added to your boards manager, you can read [here](https://github.com/esp8266/Arduino) for instructions on how to do this.

1. Using the Library Manager in the Arduino IDE, install [ArduinoJSON](https://github.com/bblanchon/ArduinoJson/),  [PubSubClient](http://pubsubclient.knolleary.net/) and [FastLED](https://github.com/FastLED/FastLED). After downloading all the zipped libraries you can install them with "Sketch" -> "Include libraries" -> "Add from ZIP"
2. Open the `mqtt_nodemcuv3_rgb.ino` file in the arduino IDE
3. Configure FastLED using the Data pin, color order, chipset and led number variables.
4. Configure the MQTT broker using MQTT server, MQTT username, MQTT port (optional) and MQTT password.
5. Configure your wifi connection with your SSID and Password.
6. Set the `client_id` variable to a unique value for your network.
7. Set `light_state_topic` and `light_command_topic` to match the values you put in your `configuration.yaml`.
8. Upload to your nodemcu board after selecting "NodeMCU 1.0".


#### Wiring
When wiring to your nodemcu you will want to have a data pin, +5v and ground. If running a small amount of LEDs you can run directly from the board itself provided your microusb power supply has sufficient Amperage. If not then connect ground and 5v to a proper power supply and then conenct it to your nodemcu.

![RGB Wiring](https://raw.githubusercontent.com/corbanmailloux/esp-mqtt-rgb-led/master/ESP%20RGB%20LED_bb.png)

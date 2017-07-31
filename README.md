# ESP8266 MQTT RGB LEDs Using JSON for Home Assistant

This project adds an easy way to create DIY lighting for [Home Assistant](https://home-assistant.io/), an amazing, extensible, open-source home automation system.

I was frustrated that the built-in MQTT light didn't support transitions (fading between colors/brightnesses), and that it uses multiple separate calls to set the different values (state (on/off), brightness, color), so I decided to make my own version. As of version 0.26, the [MQTT JSON light platform](https://home-assistant.io/components/light.mqtt_json/) has been merged into Home Assistant.

By sending a JSON payload (in an MQTT message), Home Assistant can include whichever fields are necessary, reducing the round trips from 3 to 1. For example, this is a sample payload including most of the fields:
```json
{
  "state": "ON",
  "brightness": 120,
  "color": {
    "r": 255,
    "g": 100,
    "b": 100
  },
  "transition": 5
}
```

## Installation/Configuration

To set this system up, you need to configure the [MQTT JSON light](https://home-assistant.io/components/light.mqtt_json/) component in Home Assistant and set up a light to control. This guide assumes that you already have Home Assistant set up and running. If not, see the installation guides [here](https://home-assistant.io/getting-started/).

### The Home Assistant Side
1. In your `configuration.yaml`, add the following, depending on the supported features of the light:

    ```yaml
    # Only one color:
    light:
      - platform: mqtt_json
        name: mqtt_json_light_1
        state_topic: "home/json_brightness"
        command_topic: "home/json_brightness/set"
        brightness: true
        optimistic: false
        qos: 0

    # OR: RGB:
    light:
      - platform: mqtt_json
        name: mqtt_json_light_2
        state_topic: "home/rgb1"
        command_topic: "home/rgb1/set"
        brightness: true
        rgb: true
        optimistic: false
        qos: 0

    # OR: RGBW
    light:
      - platform: mqtt_json
        name: mqtt_json_light_3
        state_topic: "home/rgbw1"
        command_topic: "home/rgbw1/set"
        brightness: true
        rgb: true
        white_value: true
        optimistic: false
        qos: 0
    ```
2. Set the `name`, `state_topic`, and `command_topic` to values that make sense for you.
3. Restart Home Assistant. Depending on how you installed it, the process differs. For a Raspberry Pi All-in-One install, use `sudo systemctl restart home-assistant.service` (or just restart the Pi).

### The Light Side
I'm using ESP8266-01 microcontrollers for my lights because they are so cheap and small. The downside of the size and price is that programming them can be a bit of a hassle. There are many sites that go into detail, so I won't do it here. You'll need an ESP set up to work with the Arduino IDE. See the readme [here](https://github.com/esp8266/Arduino) for instructions.

1. Using the Library Manager in the Arduino IDE, install [ArduinoJSON](https://github.com/bblanchon/ArduinoJson/) and [PubSubClient](http://pubsubclient.knolleary.net/). You can find the Library Manager in the "Sketch" menu under "Include Library" -> "Manage Libraries..."
2. Open the appropriate folder for your lights. For an RGB light, use `mqtt_esp8266_rgb`. For a light that only supports brightness, use `mqtt_esp8266_brightness`, and for an RGBW light, use `mqtt_esp8266_rgbw`.
3. Update the `config-sample.h` file with your settings for pin numbers, WiFi settings, and MQTT settings.
4. Ensure that the `CONFIG_MQTT_CLIENT_ID` setting is a unique value for your network.
5. Set `CONFIG_MQTT_TOPIC_STATE` and `CONFIG_MQTT_TOPIC_SET` to match the values you put in your `configuration.yaml`.
6. Save the configuration file as `config.h`.
7. Open the `.ino` file in the Arduino IDE and upload to an ESP with the correct connections.


#### Wiring
For an RGB LED strip using N-MOSFETs for control, you'll want to wire it similar to this:
![RGB Wiring](https://raw.githubusercontent.com/corbanmailloux/esp-mqtt-rgb-led/master/ESP%20RGB%20LED_bb.png)

Note that the MOSFETs have pull-**up** resistors in this setup. This means that the lights may flash on when the module resets, but it was necessary to keep the ESP's pins in the right start state.

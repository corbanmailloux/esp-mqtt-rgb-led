# ESP8622 MQTT RGB LEDs Using JSON for Home Assistant

This project adds an easy way to create custom lighting for [Home Assistant](https://home-assistant.io/), an amazing, extensible, open-source home automation system.

I was frustrated that the built-in MQTT light didn't support transitions (fading between colors/brightnesses), and that it uses multiple separate calls to set the different values (state (on/off), brightness, color), so I decided to make my own version.

By sending a JSON payload (in an MQTT message), Home Assistant can include whichever fields are necessary, reducing the round trips from 3 to 1. For example, this is a sample payload including all of the fields:
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

To set this system up, you need to install and configure the custom component in Home Assistant and set up a light to control. This also assumes that you already have Home Assistant set up and running. If not, see the installation guides [here](https://home-assistant.io/getting-started/).

### The Home Assistant Side
1. Place the `mqtt_json.py` file in `{HASS-CONFIG-DIRECTORY}/custom_components/light/`, where `{HASS-CONFIG-DIRECTORY}` is your Home Assistant configuration directory. For a Raspberry Pi All-in-One install, this is `/home/hass/.homeassistant/`.
2. In your `configuration.yaml`, add the following:

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
3. Set the `name`, `state_topic`, and `command_topic` to values that make sense for you.
4. Restart Home Assistant. Depending on how you installed it, the process differs. For a Raspberry Pi All-in-One install, use `sudo systemctl restart home-assistant.service` (or just restart the Pi).

### The Light Side
I'm using ESP8266-01 microcontrollers for my lights because they are so cheap and small. The downside of the size and price is that programming them can be a bit of a hassle. There are many sites that go into detail, so I won't do it here. You'll need an ESP set up to work with the Arduino IDE. See the readme [here](https://github.com/esp8266/Arduino) for instructions.

1. Using the Library Manager in the Arduino IDE, install [ArduinoJSON](https://github.com/bblanchon/ArduinoJson/) and [PubSubClient](http://pubsubclient.knolleary.net/). You can find the Library Manager in the "Sketch" menu under "Include Library" -> "Manage Libraries..."
2. Open the appropriate `.ino` file. For an RGB light, use `mqtt_esp8266_rgb.ino`. For a light that only supports brightness, use `mqtt_esp8266_brightness.ino`.
3. Set the pin numbers, WiFi SSID and password, MQTT server, MQTT username, and MQTT password to appropriate values.
4. Set the `client_id` variable to a unique value for your network.
5. Set `light_state_topic` and `light_command_topic` to match the values you put in your `configuration.yaml`.
6. Upload to an ESP with the correct connections.

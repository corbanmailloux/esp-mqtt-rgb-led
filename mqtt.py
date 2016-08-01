"""
Support for custom MQTT lights. 
See: https://github.com/corbanmailloux/esp-mqtt-rgb-led

Most of this code is borrowed from the standard MQTT light
that comes with Home Assistant.

Currently, this file replaces the base MQTT light file.
Place it in:
{HASS-CONFIG-DIRECTORY}/custom_components/light/

Add the following to your configuration.yaml:

light:
  - platform: mqtt
    name: mqtt_transition_test
    state_topic: "home/rgb1"
    command_topic: "home/rgb1/set"
    brightness: true
    rgb: true
    optimistic: false
    qos: 0
"""

import logging
from functools import partial

import json

import voluptuous as vol

import homeassistant.components.mqtt as mqtt
from homeassistant.components.light import (
    ATTR_BRIGHTNESS, ATTR_RGB_COLOR, ATTR_TRANSITION, Light)
from homeassistant.const import CONF_NAME, CONF_OPTIMISTIC
from homeassistant.components.mqtt import (
    CONF_STATE_TOPIC, CONF_COMMAND_TOPIC, CONF_QOS, CONF_RETAIN)
import homeassistant.helpers.config_validation as cv

_LOGGER = logging.getLogger(__name__)

DEPENDENCIES = ['mqtt']

DEFAULT_NAME = 'MQTT Light'
DEFAULT_OPTIMISTIC = False
DEFAULT_BRIGHTNESS = False
DEFAULT_RGB = False

CONF_BRIGHTNESS = 'brightness'
CONF_RGB = 'rgb'

PLATFORM_SCHEMA = mqtt.MQTT_RW_PLATFORM_SCHEMA.extend({
    vol.Optional(CONF_NAME, default=DEFAULT_NAME): cv.string,
    vol.Optional(CONF_OPTIMISTIC, default=DEFAULT_OPTIMISTIC): cv.boolean,
    vol.Optional(CONF_BRIGHTNESS, default=DEFAULT_BRIGHTNESS): cv.boolean,
    vol.Optional(CONF_RGB, default=DEFAULT_RGB): cv.boolean
})


def setup_platform(hass, config, add_devices_callback, discovery_info=None):
    """Add MQTT Light."""
    add_devices_callback([MqttLight(
        hass,
        config[CONF_NAME],
        {
            key: config.get(key) for key in (
                CONF_STATE_TOPIC,
                CONF_COMMAND_TOPIC
            )
        },
        config[CONF_QOS],
        config[CONF_RETAIN],
        config[CONF_OPTIMISTIC],
        config[CONF_BRIGHTNESS],
        config[CONF_RGB]
    )])


class MqttLight(Light):
    """MQTT light."""

    # pylint: disable=too-many-arguments,too-many-instance-attributes
    def __init__(self, hass, name, topic, qos, retain,
                 optimistic, brightness, rgb):
        """Initialize MQTT light."""
        self._hass = hass
        self._name = name
        self._topic = topic
        self._qos = qos
        self._retain = retain
        self._optimistic = optimistic or topic["state_topic"] is None
        self._state = False
        if brightness:
            self._brightness = 255
        else:
            self._brightness = None

        if rgb:
            self._rgb = [0, 0, 0]
        else:
            self._rgb = None

        def state_received(topic, payload, qos):
            """A new MQTT message has been received."""
            values = json.loads(payload)

            if values["state"] == "ON":
                self._state = True
            elif values["state"] == "OFF":
                self._state = False

            try:
                r = int(values["color"]["r"])
                g = int(values["color"]["g"])
                b = int(values["color"]["b"])

                self._rgb = [r, g, b]
            except KeyError:
                pass

            try:
                self._brightness = int(values["brightness"])
            except KeyError:
                pass

            self.update_ha_state()

        if self._topic["state_topic"] is not None:
            mqtt.subscribe(self._hass, self._topic["state_topic"],
                           state_received, self._qos)

    @property
    def brightness(self):
        """Return the brightness of this light between 0..255."""
        return self._brightness

    @property
    def rgb_color(self):
        """Return the RGB color value."""
        return self._rgb

    @property
    def should_poll(self):
        """No polling needed for a MQTT light."""
        return False

    @property
    def name(self):
        """Return the name of the device if any."""
        return self._name

    @property
    def is_on(self):
        """Return true if device is on."""
        return self._state

    @property
    def assumed_state(self):
        """Return true if we do optimistic updates."""
        return self._optimistic

    def turn_on(self, **kwargs):
        """Turn the device on."""
        should_update = False

        message = {"state": "ON"}

        if ATTR_RGB_COLOR in kwargs:
            message["color"] = {
                "r": kwargs[ATTR_RGB_COLOR][0], 
                "g": kwargs[ATTR_RGB_COLOR][1], 
                "b": kwargs[ATTR_RGB_COLOR][2]
            }

            if self._optimistic:
                self._rgb = kwargs[ATTR_RGB_COLOR]
                should_update = True

        if ATTR_TRANSITION in kwargs:
            message["transition"] = int(kwargs[ATTR_TRANSITION])

        if ATTR_BRIGHTNESS in kwargs:
            message["brightness"] = int(kwargs[ATTR_BRIGHTNESS])

            if self._optimistic:
                self._brightness = kwargs[ATTR_BRIGHTNESS]
                should_update = True

        mqtt.publish(self._hass, self._topic["command_topic"],
                     json.dumps(message), self._qos, self._retain)

        if self._optimistic:
            # Optimistically assume that switch has changed state.
            self._state = True
            should_update = True

        if should_update:
            self.update_ha_state()

    def turn_off(self, **kwargs):
        """Turn the device off."""
        message = {"state": "OFF"}

        if ATTR_TRANSITION in kwargs:
            message["transition"] = int(kwargs[ATTR_TRANSITION])

        mqtt.publish(self._hass, self._topic["command_topic"],
                     json.dumps(message), self._qos, self._retain)

        if self._optimistic:
            # Optimistically assume that switch has changed state.
            self._state = False
            self.update_ha_state()

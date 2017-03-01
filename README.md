# Sonoff-Ceiling-Lighting
Sonoff code for MQTT/OTA/switch control of domestic lighting using extra GPIO pin

This code is designed for the 'original' Sonoff.

The main purpose is to allow it to have a conventional light switch wired up to a GPIO pin to operate the light.

This is mainly useful in the UK - our domestic light switches do not have a neutral wire (they are just permanent and switched live)

Therefore, the logical place to fit the Sonoff is in the ceiling void. 

This code supports an MQTT command topic, an MQTT status topic as well as OTA updating of firmware (important if the device is inaccessible in the ceiling!)


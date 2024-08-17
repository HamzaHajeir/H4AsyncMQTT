![mbm](assets/pangoplain.jpg)

# Asynchronous Arduino MQTT 3.1.1 and 5.0 client library for ESP8266, ESP32, and RP2040

---

Licence: ![licence](https://i.creativecommons.org/l/by-nc-sa/4.0/80x15.png) Creative Commons: Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0) [Legal Text](https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)

# ***N.B. This licence expressly prohibits commercial use***

---

## If you are able, please [Support me on Patreon](https://patreon.com/esparto) and/or subscribe to my [Youtube channel (instructional videos)](https://www.youtube.com/channel/UCYi-Ko76_3p9hBUtleZRY6g)

---

###### Version 0.0.1 [Release Notes](docs/rn300.md)

* [Features](#features)
* [Performance](#performance)
* [Unique features](#features-you-may-not-find-in-other-libraries)
* [H4AsyncMQTT's place in "The Menagerie"](#the-menagerie-roadmap)
* [Installation](#installation)
* [Issues](#issues)

# Other Documents

* [Getting Started](docs/101.md)
* [Payload Handling and "expert" functions](docs/pl.md)
* [Full API specification](docs/api.md)
* [MQTT Version 5.0](docs/mqtt5.md)
* [Using TLS](docs/tls.md)
* [Challenges of embedded MQTT](docs/qos.md)
  
---

# Features

* Full* MQTT v3.1.1, MQTT v5.0 Qos0/1/2 compliance, session management and recovery
* Payload size limited only by available heap (~20kB on ESP8266, ~120kb on ESP32)
* TLS support (ESP32 only)
* Compile-time multilevel diagnostic
* Utility functions to view binary payloads and handle as:
  * C-style string (`char*`)
  * `std::string`
  * Arduino `String`
  * < any arithmetic type >
* Full error-handling including:
  * QoS failures
  * Subscribe failure
  * "Killer" packets ( > free available heap) both inbound and outbound

---

# Performance

![perf](assets/performance.jpg)

---

# Features you may not find in other libraries:

## Large Payloads

H4AsyncMQTT automatically fragments outbound packets and reassembles inbound packets of any size up to about 1/2 the free heap. User code simply gets a copy of the full packet - irrespective of its size - without any fuss and requiring *zero* code on the user's part.

## Full QoS1 / QoS2 compatibility / recovery

H4AsyncMQTT's author is unaware of any similar libraries for ESP8266/ESP32/RP2040 that *fully* and *correctly* implement QoS1/2 *in all circumstances*.

They *may* sometimes work, e.g. with only small packets and/or slow send rates but fail when either increases. None will correctly recover "lost" messages on unexpected disconnect. Given that this is pretty much the *only* purpose of QoS1/2 then any library *not* fulfilling this promise *cannot* legitimately claim to be QoS1/2 compatible. If a library does not do this:

![mqtt spec](assets/pv2mqtt.jpg)

Then it ***does not support QoS1/2*** no matter what claims it may make.

---
---

# The "menagerie" roadmap

H4AsyncMQTT is one of a family of firmware packages for simple *robust* and rapid ***asynchronous*** IOT development on ESP8266 / ESP32 / RP2040

SEE [Installation](#installation)

![roadmap](assets/common/menagerieroadmap.jpg)

## The related / dependent libraries

|| Name | Provides | Notes |
| :---: | :----------  | :--- | :--- |
||[H4AsyncTCP](https://github.com/hamzahajeir/H4AsyncTCP)|TCP and TLS communication | |
||[H4AsyncWebServer](https://github.com/hamzahajair/H4AsyncWebServer)| Basis of webUI in H4Plugins| Several major bugfixes |
|![roadmap](assets/common/tools_icon.jpg)|[PMB Tools](https://github.com/hamzahajeir/H4Tools)|'32/'8266 HAL and utility functions| |
|![roadmap](assets/common/H4Async_icon.jpg)|[H4AsyncTCP](https://github.com/hamzahajeir/H4AsyncClient)|Simple Large-payload Async TCP| API-compatible with ESPAsyncTCP, seamless TLS/SSL |
|![roadmap](assets/common/pangolin_icon.jpg)|[H4AsyncMQTT](https://github.com/hamzahajeir/H4AsyncMQTT)|Async MQTT Client|QoS 0/1/2 Fully 3.1.1 compliant. Large payloads |
|![roadmap](assets/common/armadillo_icon.jpg)|[ArmadilloHTTP](https://github.com/hamzahajeir/ArmadilloHTTP)|Async HTTP/S Client| Simple send/callback of large payloads |
|![roadmap](assets/common/h4_icon.jpg)|[H4](https://github.com/hamzahajeir/H4)|Scheduler/Async Timers| |
|![roadmap](assets/common/h4p_icon.jpg)|[H4/Plugins](https://github.com/hamzahajeir/h4plugins)|Full Async IOT Firmware| Webserver, MQTT, OTA, NTP, HTTP etc etc |
||[H4Plugins_Env](https://github.com/hamzahajeir/h4plugins_env)| PlatformIO Development Environment | With suitable versions across libraries |

## H4AsyncMQTT installation path

![pango path](assets/roadmapPangolin.jpg)

---

## Compiling

![performance](assets/lwip.jpg)

---

# Installation
## PlatformIO

One can get a homogeneous H4 Stack versions from the [PlatformIO H4Plugins Environment](https://github.com/hamzahajeir/h4plugins_env). One can reuse the environment directly, or copy the parts of interest in the configuration file `platformio.ini` in a new project.

## Arduino IDE

Simply download the zip of this repository and install as an Arduino library: `Sketch/Include Library/Add .ZIP Library...`

Up to come: See [H4 Installer](https://github.com/philbowles/h4installer)

# Issues

## If you want a *quick* resolution, please follow these rules:

1. As with all H4 and H4Plugins libraries, please make sure you have read *all* the relevant documentation relating to the issue and watched any videos on the [Youtube channel (instructional videos)](https://www.youtube.com/channel/UCYi-Ko76_3p9hBUtleZRY6g). Please also subscribe to the channel for notifications of news and updates.

2. If you still think there is a problem, then join the [Facebook H4  Support / Discussion](https://www.facebook.com/groups/444344099599131/) group and report the issue briefly there. This is because I visit the group every day, whereas I do not have time to visit 11 github repos every day. Furthermore, it alerts other users to potential problems and allows an initial assessment.

3. If there is a genuine issue then you will be referred to [Raising H4/H4Plugins issues](https://github.com/philbowles/h4plugins/blob/master/docs/issues.md) after which you are advised to create a full github issue report.

4. Failing to make an initial report in the [Facebook H4  Support / Discussion](https://www.facebook.com/groups/444344099599131/) group and simply starting with a github issue, or failing to include all of the information required in [Raising H4/H4Plugins issues](https://github.com/philbowles/h4plugins/blob/master/docs/issues.md) is likely to result in a ***long*** delay before it gets picked up.

---

(c) 2021 Phil Bowles <h4plugins@gmail.com>

* [Support me on Patreon](https://patreon.com/esparto)
* [Youtube channel (instructional videos)](https://www.youtube.com/channel/UCYi-Ko76_3p9hBUtleZRY6g)
* [Facebook H4  Support / Discussion](https://www.facebook.com/groups/444344099599131/)
* [Facebook General ESP8266 / ESP32](https://www.facebook.com/groups/2125820374390340/)
* [Facebook ESP8266 Programming Questions](https://www.facebook.com/groups/esp8266questions/)
* [Facebook ESP Developers (moderator)](https://www.facebook.com/groups/ESP8266/)

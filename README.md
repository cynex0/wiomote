# WIOmote
[![GitLab Wiki Badge](https://img.shields.io/badge/GitLab-Wiki-d94a34.svg?logo=gitlab)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote.wiki.git) [![GitLab Pipeline Badge](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/badges/main/pipeline.svg)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/jobs) [![Download Latest Artifact](https://img.shields.io/badge/Download-APK-d94a34.svg?logo=android&logoColor=white&color=green)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/jobs/artifacts/main/raw/app/build/outputs/apk/release/app-release.apk?job=build_android)
\
\
A wireless controlled configurable remote built on the [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html).

## Project Description

The project is named “*WIOmote*”, symbolizes the integration of the Wio Seed Terminal as the core device with the functionality of a remote, representing the product's purpose. 
\
\
The product in itself is a universal remote, which is intended to serve as a substitute for standard infrared based remotes, consolidating control of several devices into one. This is achieved using an IR sensor and emitter, navigated by a wirelessly connected device. It allows for the collection of numerous configurations, as well as providing users with the option to share their configurations. Users can also send commands to a terminal for immediate action or schedule them for later execution. 
\
\
For basic functions like turning on and off or basic navigation, the terminal can operate independently of a mobile device but is intended to be paired with a device to access the more advanced features.

## System Design

The system's architecture is based on an *MQTT Broker* alongside various hardware components (infrared receiver and emiter, buzzer, vibrator) controlled by software modules. These modules receive commands from the broker, manage configurations, and interpret infrared configuration data.

- **WIO Terminal:** The hardware component responsible for receiving commands and controlling devices.

- **Android App:** The mobile application responsible for sending commands to the WIO Terminal and establishing wireless communication.

![Architecture](assets/Architecture.svg)

## Dependencies & Requirements
The following section lists the dependencies and requirements for the project:

1. [Wio Seeed Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)
2. [Arduino IDE](https://www.arduino.cc/en/software)

    2.1. Arduino Libraries — Add to Arduino IDE using the [Arduino Library Manager](https://support.arduino.cc/hc/en-us/articles/5145457742236-Add-libraries-to-Arduino-IDE).
    - [Wio Terminal Board Library](https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json)
    - [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
    - [Arduino Json](https://arduinojson.org/)
    - [PubSubClient](https://github.com/knolleary/pubsubclient)
    - [Seeed Arduino rpcBLE](https://github.com/Seeed-Studio/Seeed_Arduino_rpcBLE)
    - [Seeed Arduino rpcWiFi](https://github.com/Seeed-Studio/Seeed_Arduino_rpcWiFi)
    - [WIOmote IRLib](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote_irlib)
3. [Grove Sensor List](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/wikis/home#system-sensors)

## System Sensors

The team has considered the following sensors:

| Sensor                                       | URL                                                           |
|----------------------------------------------|---------------------------------------------------------------|
| Infrared Emitter (Grove - Infrared Emitter)  | [link](https://wiki.seeedstudio.com/Grove-Infrared_Emitter)   |
| Infrared Receiver (Grove - Infrared Receiver)| [link](https://wiki.seeedstudio.com/Grove-Infrared_Receiver/) |
| Buzzer (built-in)                            | -                                                             |
| Vibration Motor (Grove - Vibration Motor)    | [link](https://wiki.seeedstudio.com/Grove-Vibration_Motor/)   |

## Instalation & Usage
_To be added during Sprint 2_

## Acknowledgments

The project is being built on:

- [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html) platform developed by [Seeed Studio](https://www.seeedstudio.com/).
- [Android Open Source Project (AOSP)](https://source.android.com/) using the [Android Software Development Kit (SDK)](https://developer.android.com/studio).

## License
The project is licensed under the *MIT License*. Refer to the [LICENSE](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/blob/main/LICENSE?ref_type=heads) file for more information.
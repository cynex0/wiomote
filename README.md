# WIOmote
[![GitLab Wiki Badge](https://img.shields.io/badge/GitLab-Wiki-d94a34.svg?logo=gitlab)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote.wiki.git) [![Pipeline Status](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/badges/main/pipeline.svg)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/pipelines)
\
\
A wireless controlled configurable remote built on the [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html).

## Project Description

The project is named “*WIOmote*”, symbolizes the integration of the Wio Seed Terminal as the core device with the functionality of a remote, representing the product's purpose. 
\
\
The product in itself is a universal remote, which is intended to serve as a substitute for standard remotes, consolidating control of several devices into one. This is achieved using an IR sensor and emitter, navigated by a wirelessly connected device. It allows for the collection of numerous configurations, as well as providing users with the option to share their configurations. Users can also send commands to a terminal for immediate action or schedule them for later execution. 
\
\
For basic functions like turning on and off or basic navigation, the terminal can operate independently of a mobile device but is intended to be paired with a device to access the more advanced features.

## System Design

The system's architecture is based on an *MQTT Broker* alongside various hardware components (infrared receiver and emiter, buzzer, vibrator) controlled by software modules. These modules receive commands from the broker, manage configurations, and interpret infrared configuration data.

- **WIO Terminal:** The hardware component responsible for receiving commands and controlling devices.

- **Android App:** The mobile application responsible for sending commands to the WIO Terminal and establishing wireless communication.

![Architecture](assets/Architecture.svg)

## System Sensors

The team has considered the following sensors:

| Sensor                                       | URL                                                           |
|----------------------------------------------|---------------------------------------------------------------|
| Infrared Emitter (IR 940nm, built-in)        | -                                                             |
| Infrared Receiver (Grove - Infrared Emitter) | [link](https://wiki.seeedstudio.com/Grove-Infrared_Receiver/) |
| Buzzer (built-in)                            | -                                                             |
| Vibration Motor (Grove - Vibration Motor)    | [link](https://wiki.seeedstudio.com/Grove-Vibration_Motor/)   |


## Acknowledgments

The project is being built on:

- [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html) platform developed by [Seeed Studio](https://www.seeedstudio.com/).
- [Android Open Source Project (AOSP)](https://source.android.com/) using the [Android Software Development Kit (SDK)](https://developer.android.com/studio).

## License
The project is licensed under the *MIT License*. Refer to the [LICENSE](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote/-/blob/main/LICENSE?ref_type=heads) file for more information.
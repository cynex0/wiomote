# WIOmote
[![Stack Overflow Badge](https://img.shields.io/badge/GitLab-Wiki-d94a34.svg?logo=gitlab)](https://git.chalmers.se/courses/dit113/2024/group-9/wiomote.wiki.git)
A wireless controlled configurable remote built on the [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html).

---

# Components
WIOmote consists of two main components:

- **WIO Terminal:** The hardware component responsible for receiving commands and controlling devices.

- **Android Application:** The mobile application responsible for sending commands to the WIO Terminal and establishing wireless communication.

# Roadmap

#### Minimum Viable Product (MVP)

- [ ] Implement basic wireless communication functionality between WIO Terminal and the Android device.
- [ ] Create a primitive way to control the on-device sensors/emiters (IR Receiver and IR Blaster).
- [ ] Develop a primitive user interface for the Android device.

#### Minimum Marketable Product (MMP)

- [ ] Create a database of predefined configs.
- [ ] Finalize Android application design and interactions.
- [ ] Allow users to create their own configurations.
- [ ] Allow users to clone an existing remote.

# Acknowledgments

This project is being built on:

- [WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html) platform developed by [Seeed Studio](https://www.seeedstudio.com/).
- [Android Open Source Project (AOSP)](https://source.android.com/) using the [Android Software Development Kit (SDK)](https://developer.android.com/studio).
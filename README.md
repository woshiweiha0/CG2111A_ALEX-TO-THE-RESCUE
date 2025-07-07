# CG2111A_ALEX-TO-THE-RESCUE

Embedded Systems Project: Teleoperated Search-and-Rescue Robot with Arduino &amp; Raspberry Pi

## Overview

This repository contains my **CG2111A Engineering Principles and Practice II Project**:  
**“Alex to the Rescue” — A Teleoperated Search-and-Rescue Robot**

Alex is designed to operate in hazardous or confined spaces such as mazes or simulated disaster zones.  
The robot integrates:
- Remote teleoperation via a Raspberry Pi with SLAM and LiDAR for real-time mapping
- Low-level motor, servo, and sensor control using an Arduino Mega 2560
- Reliable serial communication using a custom packet protocol with checksum and magic number verification
- Object manipulation with a custom-designed front claw and medpack delivery arm
- Colour-based victim identification using a TCS3200 Colour Sensor

---

## Platforms & Tools

- **Arduino IDE & C++**: microcontroller firmware development with encoder interrupts and servo control
- **Raspberry Pi with C++ & Python**: high-level command processing, SLAM, LiDAR, and teleoperation interface
- **Serial Communication**: custom protocol with error checking for robust Arduino-Pi interaction
- **Bokeh**: real-time SLAM map visualisation through a web server
- **RPLidar A1M8, TCS3200 Colour Sensor, A3144 Wheel Encoders, SG90 Micro Servos**: core hardware modules

---

## My Contributions

- Assisted in developing and debugging the **serial communication protocol** in both C++ and Python for Arduino-Pi interaction
- Implemented **Arduino firmware** for motor and servo control, colour sensing, and interrupt-driven encoder tracking
- Assisted in integrating **non-blocking single-character input** for increased responsive teleoperation commands on the RPi
- Improved colour detection accuracy using the **KNN colour detection logic**

---

## Team Contributions

- Designed and built the robot’s chassis, claw mechanism, and medpack delivery arm
- Arranged, mounted, and wired all hardware components to ensure optimal placement, stability, and weight distribution.
- Integrated **SLAM and LiDAR modules** with real-time 2D mapping and web visualisation via the Bokeh browswer
- Developed high-level command logic, teleoperation interface, and system architecture
- Implemented the colour detection system for identifying astronaut colours
- Identified and resolved issues with **packet desynchronisation**, improving system stability
- Completed the search-and-rescue mission flow, including remote teleoperation, mapping, victim identification, claw rescue, and medpack delivery

---

## Repository Contents

- `/report` — Final project report PDF
- `/sourcecode/arduino` — Arduino `.ino` files for robot motor, servo control and sensors
- `/sourcecode/raspberrypi` — Raspberry Pi C++ and Python source files for serial communication, SLAM, and teleoperation
- `/sourcecode/shared` — Common header files for packet structures and constants used across both systems

---

## Note

This project was completed as part of the **CG2111A: Engineering Principles and Practice** course at NUS.  
It showcases a complete embedded system with robust communication, teleoperation, SLAM, and real-world rescue task implementation.

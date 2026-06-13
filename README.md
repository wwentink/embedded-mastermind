# Embedded P2P Mastermind

An implementation of the classic Mastermind board game built on an Infineon PSoC6 MCU using FreeRTOS.

Done as apart of UW-Madison's ECE 353.

Authors: Will Wentink and Joe Krachey

## Features

- **FreeRTOS Multitasking:** Runs a priority-based multitasking system dividing work into isolated tasks for sensors, actuators, and communication.
- **Peer-to-Peer Multiplayer:** Supports real-time gameplay between two microcontroller boards over a UART link using a custom synchronization protocol.
- **Capacitive Touch GUI:** A modular display controller rendering a color-coded board interface on a TFT LCD with touch coordinates mapped to inputs.
- **EEPROM Storage:** Saves and loads historical high scores (lowest guess count) persistently across power cycles.
- **Smart Theme Engine:** Reads ambient light levels and automatically transitions between dark and light UI themes with noise-filtering hysteresis.
- **Acoustic Feedback:** Operates a PWM-driven buzzer to alert players of invalid inputs, successful hits, wins, or losses.

## Hardware Communication Protocols

To interface with the board's sensors, memory, display, and peer link, the system implements three primary serial communication protocols:

### 1. I2C (Inter-Integrated Circuit)
The system operates a single, shared I2C master bus to communicate with three distinct external peripherals:
* **Capacitive Touch Panel:** Read periodically to fetch raw coordinate data (X and Y registers) for player input detection.
* **Ambient Light Sensor:** Polled at regular intervals to determine ambient lighting for theme switching.
* **EEPROM Memory:** Accessed on boot to load the high score and on game completion to check for a new high score.

*Semaphores:* FreeRTOS semaphores are used to control access to the I2C bus to prevent contention and race conditions.

### 2. SPI (Serial Peripheral Interface)
A high-speed SPI bus is dedicated to driving the TFT LCD screen.
* **Display Control:** Used to transmit pixel bitmaps, draw filled shapes, and render custom text fonts on the display.
* **Gatekeeping:** All SPI transactions are funneled through a LCD gatekeeper task. Other tasks send requests to this gatekeeper, ensuring that access to hardware in controlled. 

### 3. UART (Universal Asynchronous Receiver-Transmitter)
Two independent hardware UART channels are used:
* **Debug Console:** Configured for standard I/O redirection (`printf`) to allow terminal logging and command-line interactions during development.
* **P2P Game Link (IPC):** Connects the two players' boards directly. It uses an interrupt-driven UART driver along with circular buffers for packet transmission.

## P2P Networking & Packet Structure

Game states are synchronized over the serial UART link using structured message packets (`hw05_uart_packet_t`):

| Offset (Bytes) | Field Name | Description |
|---|---|---|
| 0 | `start_byte` | Frame delimiter |
| 1 | `cmd` | Message type (Ready, Guess, Feedback, Turn End, Game Over, Restart) |
| 2 - 3 | `sequence_num` | Unique incrementing frame ID |
| 4 - 7 | `digits` | Play digits/guesses |
| 8 | `exact` | Number of correct digits in the correct position |
| 9 | `misplaced` | Number of correct digits in the incorrect position |
| 10 | `guess_count` | Running score metric |
| 11 | `flags` | Synchronization flags |
| 12 | `checksum` | 8-bit modular checksum for data validation |

## Peripheral Driver Implementation

- **EEPROM High Score Storage:** On boot, the system checks for a proprietary magic byte (`0xA5`) at a specific offset in the EEPROM. If present, it loads the saved high score. When a player completes a game in fewer guesses, the new score is written and verified by reading it back immediately.
- **Hysteresis Theme Switching:** The ambient light sensor is read at regular intervals. To prevent the screen from flickering on minor light fluctuations:
  - Theme switches from Dark to Light only if ambient light exceeds `2900` Lux.
  - Theme switches from Light to Dark only if ambient light falls below `2100` Lux.
- **Touch Coordinate Mapping:** Reads raw touch registers via I2C, checks for debounced coordinates, and maps pixels (X, Y) to the corresponding grid boundary of the screen's rendering layout.

## Environment & Build Tools

- **Microcontroller:** Infineon PSoC6 MCU (ARM Cortex-M33 core).
- **Development Toolchain:** ModusToolbox v3.3, GNU Arm Embedded Toolchain (`GCC_ARM`).
- **Language:** C
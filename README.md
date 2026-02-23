# Desktop Reactor: HIL Telemetry & Firmware Validation Pipeline

## Executive Summary
The Desktop Reactor is a bare-metal, Hardware-in-the-Loop (HIL) telemetry monitor built on the RP2040 microcontroller. Designed with strict Post-Silicon Validation methodologies, the system receives real-time host PC metrics over USB-CDC, parses them via a custom binary protocol, and drives a 240x240 SPI display at a deterministic ~27 FPS. 

The project heavily emphasizes firmware reliability, utilizing automated Python fuzzing (PyTest) and physical hardware profiling (Analog Discovery 3) to guarantee system stability, strict timing constraints, and 100% fault tolerance against corrupted data streams.

*Note: For a full visual overview of this project, including hardware setup photos, validation suite outputs, and hardware trace profiling, please visit my portfolio.*

---

## Hardware Architecture & Bill of Materials
* **MCU:** Raspberry Pi Pico (RP2040, ARM Cortex-M0+)
* **Display:** GC9A01 1.28" Circular TFT LCD (SPI)
* **Instrumentation:** Digilent Analog Discovery 3 (AD3) Logic Analyzer / Oscilloscope

### Physical Pin Mapping (DUT to Host/Tools)
| Component | RP2040 Pin | Function |
| :--- | :--- | :--- |
| **GC9A01 Display** | GP18 | SPI0 SCK (Clock) |
| | GP19 | SPI0 TX (MOSI / SDA) |
| | GP17 | SPI0 CSn (Chip Select) |
| | GP13 | SPI0 DC (Data/Command) |
| | GP12 | RESET |
| | GP16 | BLK (Hardware PWM Backlight) |
| **AD3 Logic Analyzer** | GP14 | DIO 0 (`RX_TIME` - Packet read profiling) |
| | GP15 | DIO 1 (`RENDER_TIME` - SPI execution profiling) |

---

## Firmware Architecture (C++)
To bypass the latency of standard ASCII string parsing, the host-to-device communication relies on a strict, 4-byte binary packet structure: `[0xAA Header | Sequence ID | CPU Load | XOR Checksum]`.

* **Non-Blocking State Machine:** Utilizing `getchar_timeout_us()`, the firmware operates on an RTOS-style non-blocking loop. This prevents the primary 50Hz main loop from hanging during fragmented or delayed USB transfers.
* **Differential Rendering Engine:** Early prototypes suffered from framebuffer bottlenecks over the 2MHz SPI bus. The graphics driver was rewritten using a custom "Donut Eraser" algorithm, utilizing fast integer geometry to selectively erase and redraw only the exact delta between frames, entirely eliminating visual artifacts without requiring external VRAM.

---

## Verification & Validation Methodology
Standard "happy path" testing is insufficient for robust embedded systems. A dedicated `pytest` framework acts as Automated Test Equipment (ATE), subjecting the firmware to rigorous fault injection:

1. **Protocol Fuzzing:** The Python suite intentionally injects invalid checksums and truncated byte-frames to trigger edge-case failures.
2. **Hardware Watchdog (WDT):** The RP2040’s hardware watchdog is armed with a 500ms timeout threshold to ensure automated physical recovery in the event of an infinite loop or bus lockup.
3. **State Machine Armor:** The C++ state machine successfully discards all malformed packets without halting, validated by a 0% crash rate during the PyTest regression suite.

---

## Hardware Profiling & Key Results
To move beyond software-level assumptions, the firmware was instrumented with dedicated debug GPIOs, and physical system latency was profiled using a Digilent Analog Discovery 3 (AD3).

* **Worst-Case Execution Time (WCET):** Hardware traces confirmed a maximum frame rendering latency of **36 milliseconds** over the 2MHz SPI bus, mathematically guaranteeing the system's ability to maintain a stable ~27 FPS output without task starvation.
* **Fault Tolerance:** Visual trace verification confirms the firmware successfully intercepts corrupted packets and actively bypasses the graphics engine to save clock cycles.

---

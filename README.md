# FSR402 Drone Force Sensor Monitor

Real-time 8-channel force sensor data logger built on the **ESP32-S3**.  
Streams 1000 samples/second per sensor to a browser over Wi-Fi, displays live kg readings, and exports recorded sessions as `.xlsx`.

---

## Table of Contents

1. [Hardware Requirements](#1-hardware-requirements)
2. [Wiring](#2-wiring)
3. [Software Requirements](#3-software-requirements)
4. [Installation — Step by Step](#4-installation--step-by-step)
5. [First-Time Use](#5-first-time-use)
6. [Web UI Guide](#6-web-ui-guide)
7. [Calibration](#7-calibration)
8. [Project Structure](#8-project-structure)
9. [Technical Details](#9-technical-details)

---

## 1. Hardware Requirements

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32-S3-DevKitC-1 (16 MB flash) | 1 | Any ESP32-S3 with ≥ 8 MB flash works |
| SOUSHINE FSR402 Long-Tail | up to 8 | 14.7 mm active area, 20 g – 10 kg range |
| 10 kΩ resistor | 1 per sensor | Pull-down for voltage divider |
| Breadboard + jumper wires | — | |
| USB-C cable | 1 | For programming |

---

## 2. Wiring

### Single Sensor (repeat for each)

```
3.3V ───┬─── FSR402 (pin 1)
        │
        └─── FSR402 (pin 2) ───┬─── GPIO pin (ADC1)
                               │
                             10 kΩ
                               │
                             GND
```

The FSR402 is a **resistive divider** element — it has no polarity.  
With no force applied, FSR resistance ≈ ∞ → GPIO reads ~0 V → ADC ≈ 0.  
With maximum force (~10 kg), FSR resistance ≈ 200 Ω → GPIO reads ~3.2 V → ADC ≈ 3900.

### Pin Mapping (ESP32-S3-DevKitC-1)

| Sensor | GPIO | DevKit Label | ADC Channel |
|--------|------|--------------|-------------|
| Sensor 1 | GPIO 1 | IO1 | ADC1_CH0 |
| Sensor 2 | GPIO 2 | IO2 | ADC1_CH1 |
| Sensor 3 | GPIO 3 | IO3 | ADC1_CH2 |
| Sensor 4 | GPIO 4 | IO4 | ADC1_CH3 |
| Sensor 5 | GPIO 5 | IO5 | ADC1_CH4 |
| Sensor 6 | GPIO 6 | IO6 | ADC1_CH5 |
| Sensor 7 | GPIO 7 | IO7 | ADC1_CH6 |
| Sensor 8 | GPIO 8 | IO8 | ADC1_CH7 |

> **Important:** Only ADC1 pins are used. ADC2 is shared with Wi-Fi and gives unstable readings when Wi-Fi is active.

### Full 8-Sensor Wiring Diagram

```
ESP32-S3-DevKitC-1
┌─────────────────────────────┐
│  3V3 ───────────────────────┼──── FSR1[+] ── FSR1[-] ──┬── IO1
│                             │                           10kΩ
│                             │                           GND
│                             │
│  3V3 ───────────────────────┼──── FSR2[+] ── FSR2[-] ──┬── IO2
│                             │                           10kΩ
│                             │                           GND
│                             │
│       (repeat for IO3–IO8)  │
│                             │
│  GND ───────────────────────┼──── Common GND rail
└─────────────────────────────┘
```

> All 10 kΩ pull-down resistors share the same GND rail.  
> All FSR402 top pins share the same 3.3 V rail.

---

## 3. Software Requirements

| Tool | Version | Download |
|------|---------|----------|
| VS Code | Latest | https://code.visualstudio.com |
| PlatformIO IDE extension | Latest | VS Code Extensions marketplace |
| Python 3.x | 3.8 + | https://python.org (needed by PlatformIO) |
| Git | Latest | https://git-scm.com |

---

## 4. Installation — Step by Step

### Step 1 — Install VS Code

1. Download VS Code from https://code.visualstudio.com
2. Run the installer, accepting defaults
3. Launch VS Code

### Step 2 — Install PlatformIO Extension

1. Click the **Extensions** icon in the VS Code sidebar (or press `Ctrl+Shift+X`)
2. Search for **PlatformIO IDE**
3. Click **Install**
4. Wait for installation to complete (it downloads PlatformIO Core — takes 2–5 minutes)
5. **Restart VS Code** when prompted

### Step 3 — Clone this Repository

Open a terminal in VS Code (`Ctrl+`\``) and run:

```bash
git clone https://github.com/iammannan/FSR402-Drone-Monitor.git
cd FSR402-Drone-Monitor
```

Or open the folder directly:

1. `File` → `Open Folder`
2. Select the cloned `FSR402-Drone-Monitor` folder
3. VS Code will detect `platformio.ini` and activate PlatformIO automatically

### Step 4 — Install the `intelhex` Python Package

PlatformIO's esptool requires this package. Open a terminal and run:

```bash
# Windows
pip install intelhex

# If pip is not in PATH, use the py launcher
py -m pip install intelhex
```

### Step 5 — Connect the ESP32-S3

1. Connect the ESP32-S3 DevKit via USB-C
2. Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac) for the COM port
3. Update `platformio.ini` if your port is different from `COM15`:

```ini
upload_port = COM15      ; ← change to your port
monitor_port = COM15     ; ← change to your port
```

### Step 6 — Upload the Firmware

In VS Code, click the **PlatformIO sidebar** (alien icon) → **Upload**, or run in the terminal:

```
C:\Users\<you>\.platformio\penv\Scripts\platformio.exe run --target upload
```

You should see:

```
Writing at 0x00010000... (100 %)
Hash of data verified.
========================= [SUCCESS] =========================
```

### Step 7 — Upload the Web UI (HTML filesystem)

This uploads the `data/index.html` file to the ESP32's LittleFS flash partition:

```
C:\Users\<you>\.platformio\penv\Scripts\platformio.exe run --target uploadfs
```

> **Note:** You must do this step separately from firmware upload. Both steps are required on first install.

### Step 8 — Verify

Open the Serial Monitor (`Ctrl+Alt+S` or PlatformIO sidebar → Monitor). You should see:

```
[FS] LittleFS OK
[AP] SSID: "Data Plotter for Drone"  IP: 192.168.4.1
[HTTP] Server ready → http://192.168.4.1
```

---

## 5. First-Time Use

1. On your phone or laptop, connect to Wi-Fi:
   - **SSID:** `Data Plotter for Drone`
   - **Password:** `00009999`

2. Open a browser and go to:
   ```
   http://192.168.4.1
   ```

3. The dashboard loads automatically. The status dot turns **green** when connected.

4. With no sensors pressed, click **⊘ Tare All** to zero all channels.

5. Press a sensor — you should see the reading change in real time.

---

## 6. Web UI Guide

```
┌─────────────────────────────────────────────────────────┐
│ ⚡ Force Sensor Monitor              ● Connected        │
├─────────────────────────────────────────────────────────┤
│ ⊘ Tare All │ ▶ Start Recording │ ⏸ Stop │ ↙ XLSX │ ⚙ │
├─────────────────────────────────────────────────────────┤
│ S1 raw ADC: 1842        Rate: ~1000 sps                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│              Live Chart (kg vs time)                    │
│                                                         │
├─────────────────────────────────────────────────────────┤
│ ● S1  ● S2  ● S3  ● S4  ● S5  ● S6  ● S7  ● S8        │
├──────────┬──────────┬──────────┬───────────────────────┤
│ Sensor 1 │ Sensor 2 │ Sensor 3 │ Sensor 4              │
│  2.341kg │  0.000kg │  0.000kg │  0.000kg              │
├──────────┼──────────┼──────────┼───────────────────────┤
│ Sensor 5 │ Sensor 6 │ Sensor 7 │ Sensor 8              │
│  0.000kg │  0.000kg │  0.000kg │  0.000kg              │
└──────────┴──────────┴──────────┴───────────────────────┘
```

| Button | Function |
|--------|----------|
| **⊘ Tare All** | Zeros all 8 sensors at current load |
| **▶ Start Recording** | Begins logging all samples to memory |
| **⏸ Stop Recording** | Ends logging, enables download |
| **↙ Download XLSX** | Downloads recorded data as Excel file |
| **⚙ Settings** | Opens per-sensor calibration panel |

---

## 7. Calibration

The FSR402 is a **non-linear** resistive sensor. The conversion formula used is:

```
R_fsr      = R_pull × (4095 / ADC_raw − 1)
conductance = ADC_raw / (R_pull × (4095 − ADC_raw))
weight (kg) = CalFactor × conductance / 1000
```

### Procedure

1. Click **⚙ Settings**
2. Set **Max Display (kg)** = `10` (FSR402 maximum)
3. Set **Pull-down Resistor (Ω)** = `10000` (your 10 kΩ resistor)
4. Place the board on a flat surface with **no weight** on any sensor
5. Click **⊘ Tare All**
6. Place a **known weight** (e.g. a 500 g object) on Sensor 1
7. In the Settings panel, watch the **Live ADC** column for Sensor 1
8. Adjust **Cal Factor** for Sensor 1 until the main display reads `0.500 kg`
9. Typical Cal Factor range: `800,000,000` – `1,200,000,000`
10. Repeat for each sensor, then click **Save**

Settings are saved to **browser localStorage** — they persist across page refreshes.

---

## 8. Project Structure

```
FSR402-Drone-Monitor/
├── src/
│   └── main.cpp          # ESP32 firmware (WiFi AP, WebSocket, 1kHz sampler)
├── data/
│   └── index.html        # Web dashboard (served from LittleFS)
├── platformio.ini         # PlatformIO build configuration
└── README.md
```

---

## 9. Technical Details

| Parameter | Value |
|-----------|-------|
| Sampling rate | 1000 samples/sec per sensor |
| WebSocket packet rate | 20 packets/sec (50 samples each) |
| ADC resolution | 12-bit (0 – 4095) |
| ADC attenuation | 11 dB (0 – 3.3 V range) |
| Wi-Fi mode | Access Point (no router needed) |
| IP address | 192.168.4.1 |
| Filesystem | LittleFS |
| Flash size | 16 MB |
| Partition | default_16MB.csv |

### Libraries Used

| Library | Purpose |
|---------|---------|
| [ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) | Async HTTP + WebSocket server |
| [ArduinoJson](https://arduinojson.org) | JSON serialization |
| LittleFS (built-in) | Flash filesystem for web files |

### How Batched Sampling Works

```
Every 1 ms  → read 8 ADC channels → store in write buffer
Every 50 ms → swap buffers → send one WebSocket JSON frame
             (frame contains 50 timestamped rows, ~2.8 KB)

Browser     → receives 20 frames/sec
            → unpacks all 50 rows per frame into rolling chart
            → renders at 60 fps (requestAnimationFrame)
```

This design avoids WebSocket queue overflow (which causes disconnects)  
while still delivering the full 1000 samples/sec to the browser.

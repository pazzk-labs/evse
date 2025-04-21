# Pazzk EVSE Development Board

[Get the hardware →](https://pazzk.net/ko/products#hardware)

<img src="images/dev-board-testing.jpg" alt="Development Board Under Test" style="width:100%; max-width:700px; margin-top:1em;" />

<p align="center"><em>Photo: Development boards undergoing functional testing</em></p>

## Features

### System & Sensing
- Integrated posture and impact detection sensor
- GFCI and relay weld detection
- Input and output power monitoring
- Internal temperature sensor for charger housing
- Emergency stop button (EMO)

### User Interface
- GUI-based LCD display (optional)
- Status LEDs and audible feedback (buzzer, speaker)

### Communication Capabilities
- Built-in PLC modem (ISO 15118 compliant)
- Multiple communication interfaces supported:
  - Ethernet (10/100 Mbps, RJ45)
  - Wi-Fi (802.11 b/g/n)
  - BLE (Bluetooth 5.0)
  - USB Type-C (USB 2.0)

## Specification

| Type                          | Value                                   | Unit        |
|-------------------------------|-----------------------------------------|-------------|
| Charging Mode                 | AC Level 2 (Single-phase)               |             |
| Rated AC Power Output         | 11                                      | kW          |
| Nominal AC Frequency          | 50 / 60 (Auto-switching)                | Hz          |
| Ground Fault Detection        | 13                                      | mA          |
| Input Voltage Range           | 100 – 300                               | VAC         |
| Power Measurement Accuracy    | ±0.5                                    | %           |
| Operating Temperature         | -30 to +70                              | °C          |
| Storage Temperature           | -40 to +85                              | °C          |
| Dimensions (L × W × H)        | 110 x 125.58 x 54.53                    | mm          |
| Weight                        | 302                                     | g           |
| Communication Interfaces      | Ethernet, Wi-Fi, BLE, PLC, USB          |             |
| Display & Indicators          | LCD, LED, Buzzer, Speaker               |             |

## Temperature Characteristics by Component

| Component     | Operating Temp (°C) | Storage Temp (°C) | Note                                |
|---------------|---------------------|-------------------|-------------------------------------|
| QCA7005       | -40 to +85          | -40 to +150       | PLC modem (ISO 15118 core module)  |
| AZSR250       | -40 to +85          | -                 | Relay                              |
| HLW8112       | -40 to +85          | -65 to +150       | Power measurement AFE              |
| ESP32-S3      | -40 to +85          | -40 to +150       | Wi-Fi/BLE MCU                      |
| IRM-20        | -30 to +70          | -40 to +85        | AC/DC power supply (SMPS module)   |
| LIS2DW12      | -40 to +85          | -40 to +125       | Motion and vibration sensor        |
| TMP102        | -55 to +150         | -60 to +150       | Temperature sensor                 |
| MLT-8530      | -20 to +70          | -40 to +85        | Buzzer                             |
| TCA9539       | -40 to +85          | -65 to +150       | IO Expander                        |
| ADC122S051    | -40 to +85          | -65 to +150       | ADC                                |
| CA-IS3722     | -40 to +125         | -65 to +150       | Digital isolator                   |
| TCLT1007      | -55 to +100         | -55 to +125       | Optocoupler                        |
| 74LVC1G34     | -40 to +125         | -65 to +150       | Buffer                             |
| 74AHCT1G08    | -40 to +125         | -65 to +150       | Logic AND gate                     |
| ES8311        | -40 to +85          | -65 to +150       | Audio codec                        |
| NS4150        | -40 to +85          | -65 to +150       | Audio amplifier                    |
| CA-IS3741     | -40 to +125         | -65 to +150       | Digital isolator                   |
| EL357N        | -55 to +110         | -55 to +125       | Photocoupler                       |
| SIC438AED     | -40 to +105         | -65 to +150       | DC-DC converter                    |
| DPU01M        | -40 to +90          | -55 to +125       | DC-DC converter                    |
| SPX1117M3     | -40 to +125         | -65 to +150       | Voltage regulator                  |
| TLV1117       | -40 to +125         | -65 to +150       | Voltage regulator                  |
| RT9193        | -40 to +85          | -65 to +150       | Voltage regulator                  |
| RFMM-0505S    | -40 to +85          | -                 | Isolated DC-DC converter           |

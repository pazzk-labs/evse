# Input Section

Pin Descriptions and Debouncing Requirements

## Input Pins Overview

| Pin Name      | Description              | Debouncing Required | Purpose    | Notes |
| ------------- | ------------------------ | ------------------- | ---------- | ----- |
| `LINE_PE_IN`  | Input power statusk      | Yes                 | Safety     | Pulse |
| `LINE_N_RLY`  | Output power status      | Yes                 | Safety     | Pulse |
| `IO0_BOOT`    | Debug input button       | Yes                 | Debug      | Active Low  |
| `METER_INT1`  | Meter pulse input        | No                  | Essentials | Active Low  |
| `QCA7005_INT` | QCA7005 interrupt        | No                  | Essentials | Active High |
| `W5500_INT`   | W5500 interrupt          | No                  | Essentials | Active Low  |
| `XPNDR_INT`   | IO expander interrupt    | No                  | Essentials | Active Low  |
| `nEMERGENCY`  | Emergency stop button    | Yes                 | Safety     | Active Low(connected through IO expander)  |
| `USB_DETECT`  | USB connection detection | Yes                 | Add-ons    | Active High(connected through IO expander) |
| `LIS_INT`     | Accelerometer interrupt  | No                  | Add-ons    | Active High(connected through IO expander) |
| `TMP_ALERT`   | Temperature sensor alert | No                  | Add-ons    | Active Low(connected through IO expander)  |

## Debouncing Requirements
- Pins Requiring Debouncing:
  - Pins associated with buttons, power status detection, and emergency stop functions, where signal stability is critical to system reliability.
  - Adequate signal conditioning is necessary to avoid erroneous signal interpretation caused by noise or transient fluctuations.

## Implementation Notes
- Check every rising and falling time of the signal to determine if the system catches it exactly.
- Use an interrupt-driven approach to initiate the debouncing process. During this period, disable the associated interrupt to prevent redundant or spurious interrupt handling, thereby conserving processing resources.
  - For instance, `LINE_PE_IN` and `LINE_N_RLY` pins may generate frequent interrupts at a microsecond scale if not managed correctly during debouncing, leading to resource inefficiency.

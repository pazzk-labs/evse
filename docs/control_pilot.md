# Control Pilot

This document outlines the technical specifications, measurement process, and
testing guidelines for the control pilot system. Key takeaways include:

1. ADC sampling achieves a resolution of 2 μs with error <0.5%.
2. Voltage fluctuation thresholds and hysteresis values are configurable to ensure reliable operation.
3. Testing guidelines provide clear criteria for verifying state-specific voltages.

## Overview
- Frequency: 1 kHz (1 ms per cycle)
- ADC: 500 kSPS @ 8 MHz; 500 samples per ms
  - Resolution: 2 μs, measurement error < 0.5%
- Sampling Period: 10 ms
- Duty Cycle:
  - Adjustable range: 2–100%
    - Measurements below 1% are constrained by hardware limitations.
  - Measurable range: 1–100%
  - Resolution: 1%, error < 0.5%

## Compliance with Standards
This design aligns with the following international standards:

- IEC 61851: Electric vehicle conductive charging system requirements.
- SAE J1772: Standards for EV charging connectors and protocols.

## Measurement Process
1. ADC Sampling
2. Conversion to millivolts
3. High/Low signal distinction
4. Outlier Removal
5. Duty Cycle Calculation
6. Boundary Check (for anomaly detection)
7. State Update

### Implementation Notes
- Sampling Details:
  - ADC sampling capacitor: 30 pF; resistance: 500 Ω; sufficient sampling time.
  - 500 samples per 1 ms (2 μs/sample).
- Boundary Definition:
  - Boundary values are derived from theoretical calculations and can be referenced in this [spreadsheet](https://docs.google.com/spreadsheets/d/1GBLa0a5506phaczR-4YLWwTQRjZHd1QTdfgkcr8PGlE).
  - Anomaly detection functionality captures real-world measurement variances, and configurable voltage hysteresis can compensate for these variances.
- Voltage Segmentation:
  - High/low distinction is set at 1V and is configurable.
  - Unused voltages in IEC 61851/J1772 standards minimize false filtering.
- Hysteresis:
  - 500 mV gap for each state (configurable):
  - Example: In state B (9V), transition to:
    - State A (12V): ≥ 10.75V
    - State C (6V): ≤ 4.25V
- Anomaly Handling:
  - Voltage fluctuation is typically below 30 mV; an additional 20 mV margin has been included.
    - ≥ 50 mV considered abnormal (configurable).
  - Rising/falling transitions are typically done within 20 μs (10 samples); a 10 μs margin has been added.
    - 30 μs considered abnormal (configurable).
- Adjustable Parameters:
  - Sampling interval
  - Voltage for high/low distinction
  - Noise margin
  - Transition time
  - State voltage hysteresis
  - Number of ADC samples
- When adjusting the sampling interval, the number of buffers introduced for lock-free programming should also be reviewed. 

The following table summarizes the measured voltage ranges for different duty cycles under various states (12V, 9V, 6V). Values represent millivolts measured at the corresponding duty cycle.

| Duty | 12V_H | 12V_L | 9V_H | 9V_L | 6V_H | 6V_L |
|------|-------|-------|------|------|------|------|
| 100% | 3.141 |     0 | 2.871|    0 | 2.541|    0 |
|  75% | 3.141 |   554 | 2.871|  554 | 2.541|  554 |
|  50% | 3.141 |   554 | 2.871|  554 | 2.541|  554 |
|  25% | 3.138 |   554 | 2.871|  554 | 2.541|  554 |
|   5% | 3.135 |   554 | 2.871|  554 | 2.541|  554 |

## Production Testing Guidelines
- Verify status-specific voltages fall within the expected range:
  - Command: test cp (via CLI).

| Status | High Voltage | Low Voltage |
|--------|--------------|-------------|
| A(12V) | 3.279V       | 3.065V      |
| B(9V)  | 2.958V       | 2.744V      |
| C(6V)  | 2.637V       | 2.424V      |

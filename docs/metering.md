# Metering System Design

## Energy Measurement and Storage

### Core Principles

The metering system maintains two energy values:
- Runtime accumulator (`energy`): Continuously updated with new measurements
- Persistent storage (`param.energy`): Last saved state in non-volatile memory

### Storage Strategy

Energy data is saved to non-volatile storage when either condition is met:
- Accumulated delta exceeds `METERING_ENERGY_SAVE_THRESHOLD_WH` (1.7kWh)
- Time since last save exceeds `METERING_ENERGY_SAVE_INTERVAL_MIN` (10 minutes)

These thresholds are chosen to balance:
- Data loss risk (max ~550원 at 11kW charging)
- Flash memory write cycles
- System reliability

### Storage Mechanisms

Energy data can be saved in two ways:
1. Automatic saving via `step()`
   - Called periodically during normal operation
   - Checks time and threshold conditions
   - Ensures consistent saving during charging

2. Manual saving via `metering_save_energy()`
   - Available for immediate save requests
   - Used during system shutdown or state transitions
   - Same save conditions apply

### Charging Scenarios

1. 11kW AC charging:
   - Energy per minute: 183Wh
   - 10 minutes = 1.83kWh ≈ 550원
   - 1.7kWh threshold reached in ~9.3 minutes

2. 7kW AC charging:
   - Energy per minute: 117Wh
   - 10 minutes = 1.17kWh ≈ 350원
   - 1.7kWh threshold reached in ~14.5 minutes

In both cases:
- Time-based save (10 min) may trigger before threshold
- Maximum potential loss limited to ~550원 (11kW) or ~350원 (7kW)
- System maintains balance between data protection and storage wear

### Implementation Details

1. Runtime Accumulation
   - `energy` field continuously accumulates measurements
   - Always greater than or equal to last saved value
   - Maintains most recent energy state

2. Persistent Storage
   - `param.energy` reflects last saved state
   - Updated only on successful storage write
   - Serves as recovery point after power loss

3. Save Operation
   ```c
   if (current_time - last_save_time >= METERING_ENERGY_SAVE_INTERVAL_MIN ||
       energy.wh - param.energy.wh >= METERING_ENERGY_SAVE_THRESHOLD_WH) {
       // Save energy to non-volatile storage
   }
   ```

### Example Scenarios

1. Normal Charging (11kW):
   - 10 minutes = 1.83kWh ≈ 550원
   - Triggers save by time interval

2. Power Loss:
   - Maximum data loss: 10 minutes of charging
   - Recovery from last saved state in param.energy

## API Usage

```c
// Create metering instance with storage callback
struct metering *meter = metering_create(METERING_TYPE_HLW8112, 
                                       &param, save_cb, ctx);

// Periodic energy save check
metering_save_energy(meter);
```

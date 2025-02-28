# Pazzk Firmware Operating Modes

Pazzk firmware supports four different operating modes, each optimized for a specific use case. The features and limitations vary depending on the selected mode.

---

## 1. Development Mode
### Description
- Mode for development and debugging
- No restrictions on configuration changes
- Logging and debug interfaces enabled

### Key Features
- Detailed logging (Level: Debug)
- Firmware and configuration changes allowed
- Security mechanisms (e.g., firmware verification) can be disabled
- Remote debugging and test APIs enabled

### Use Case
Used during development to test new features and debug issues.

---

## 2. Manufacturing Mode
### Description
- Mode for factory testing and initial configuration
- Used in production lines to verify device functionality

### Key Features
- Factory test features enabled
- Automatic calibration and self-diagnostics
- Hardware functionality checks
- Limited configuration changes allowed
- Firmware verification enabled

### Use Case
Used before shipment to verify hardware and firmware functionality.

---

## 3. Installing Mode
### Description
- Used during on-site installation and configuration
- Allows installation engineers to set up network and system configurations

### Key Features
- Network setup and certificate installation enabled
- Basic operational tests allowed
- Configuration changes allowed (until transition to production mode)
- Debug logging enabled (less detailed than development mode)

### Use Case
Used by CPOs or operators to configure devices during deployment.

---

## 4. Production Mode
### Description
- Normal operating mode after installation
- Charging services are fully operational

### Key Features
- Charging service active
- Configuration changes restricted (ensuring security and integrity)
- Remote software updates allowed
- Security enforcement and firmware verification enabled
- System protection and automatic recovery features enabled

### Use Case
Used in production environments where the charging station is fully operational.

---

## Summary
| Mode Name | Description | Features |
|-----------|-------------|-----------|
| development | Development and debugging mode | Debug logging, config changes, test API enabled |
| manufacturing | Factory testing mode | Hardware testing, calibration, verification |
| installing | Installation and setup mode | Network setup, certificate deployment, initial testing |
| production | Production mode | Normal operation, security enforcement, configuration restricted |

---

This document defines the Pazzk firmware operating modes and describes their purpose and configuration methods. Ensure that mode transitions follow security and operational guidelines.

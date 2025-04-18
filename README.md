# Pazzk EVSE

🌐 **Available languages**: [English](README.md) | [한국어](README_ko.md)<br />
🔗 **Official Website**: [pazzk.net](https://pazzk.net)

Pazzk EVSE is an open-source project designed to provide a reliable and scalable electric vehicle charging solution. It complies with various EV charging standards while maintaining a hardware-independent architecture to enable seamless integration across different environments.

This project is available under both the GPLv3 open-source license and a commercial license. The open-source version is free to use, while the commercial license includes additional features and support services.

[![pazzk-logo](docs/images/pazzk-logo.png)](https://pazzk.net)

> [!WARNING]
> Pazzk EVSE firmware directly controls electrical and electronic components.<br />
> Incorrect configuration or installation may cause safety hazards. Ensure compliance with electrical safety regulations and certifications before deployment.

## Key Features
- Supports various charging methods:
  - Backend authentication (OCPP-based)
  - Vehicle automatic authentication (ISO 15118-based)
  - Unauthenticated charging
- Enhanced safety features:
  - GFCI and relay stuck detection
  - Input and output power monitoring
- Modular architecture:
  - Components can be added or removed based on requirements
    - Example: Audio, LCD display, RFID reader, etc.
- Network capabilities:
  - Supports Ethernet, Wi-Fi, and BLE (Bluetooth Low Energy)
  - Automatic connection recovery and management
- Flexible platform support:
  - Compatible with various MCUs and host platforms
  - Can be used as a host simulator
- Reference hardware:
  - Open-source hardware available ([separate repository](https://github.com/pazzk-labs/evse-hw))

## Getting Started
For more details, please refer to the developer documentation: [docs.pazzk.net](https://docs.pazzk.net).

### Project Structure
(Block diagram to be added)

### Directory Structure

```
.
├── docs
├── driver
├── external
│   ├── libmcu
│   └── ocpp
├── include
├── ports
│   └── esp-idf
├── projects
├── src
│   ├── charger
│   └── cli
└── tests
```

| Directory  | Description                                               |
| ---------- | --------------------------------------------------------- |
| docs       | Project documentation                                     |
| driver     | Device drivers. Independently maintainable code           |
| external   | Third-party libraries and SDKs (ESP-IDF, OCPP, etc.)      |
| include    | Project header files                                      |
| ports      | Third-party libraries and hardware interface code         |
| projects   | Platform/IDE build configurations (Makefile, CMake, etc.) |
| src        | Application code. No hardware or platform-specific code   |
| tests      | Unit tests and mock test code                             |

### Build and Run

> [!NOTE]
> Additional configuration may be required depending on the build environment. Refer to the [documentation](docs/markdown/build.md) for details.

(Real hardware image to be added)

## Community
Pazzk EVSE is growing with the support of the open-source community. You can contribute in the following ways:

- Report issues and request features: [Issues Page](https://github.com/pazzk-labs/evse/issues)
- Fix bugs and improve documentation: [Pull Requests](https://github.com/pazzk-labs/evse/pulls)
- Feedback or collaboration proposals: [team@pazzk.net](mailto:team@pazzk.net)

## Use Cases
- **Developers:**
  - Expand functionality easily due to hardware-independent architecture
  - Simulate various charging scenarios based on ISO 15118 and OCPP
  - Rapid prototyping with open-source code

- **Businesses:**
  - Accelerate development of commercial solutions compliant with PnC and OCPP
  - Reduce initial development and maintenance costs with an open-source platform
  - Customize solutions to build competitive commercial products

- **Researchers:**
  - Test experimental charging technologies with open-source hardware
  - Study and experiment with network and authentication protocols
  - Collaborate with the community to share knowledge and trends

## License
This project is available under the following two licensing options:

### GPLv3 (Community Edition)
The source code is freely available for modification and distribution for personal or non-commercial use.

### Commercial License (Proprietary)
A separate contract is required to integrate or customize this software for commercial products. The commercial license includes additional features and technical support.  
[Learn More](https://pazzk.net/commercial-license).

## Disclaimer
- Before deploying this firmware and hardware in real-world environments, ensure compliance with local electrical safety regulations and certification requirements.
- This open-source code is provided "as is," and the project contributors are not responsible for any direct or indirect damages resulting from its use.
- If planning to release a commercial product or sell to customers, professional verification and legal/technical consultation are strongly recommended.

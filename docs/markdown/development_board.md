[한국어](#evse-development-board---korean-version)

# EVSE Development Board - Korean Version
[Get the hardware →](https://pazzk.net/ko/products#hardware)

<img src="images/dev-board-testing.jpg" alt="Development Board Under Test" style="width:100%; max-width:700px; margin-top:1em;" />

<p align="center"><em>Photo: Development boards undergoing functional testing</em></p>

## Support and Pricing
### Basic Package
When you purchase the Pazzk EVSE Development Board, the following items are included:

- 11kW Single-phase AC EVSE reference hardware
- Complete hardware design source
- Open-source EVSE firmware (Community Edition)
- Technical support is limited to unofficial assistance via community forums and GitHub Issues.

> [!NOTE]
> Dedicated technical support or individual consulting is not provided.
> Additional support requires a separate contract.

### Optional Additional Support (Separate Contract Required)
Pazzk also offers the following professional support services under separate agreements:

- Firmware porting support for customer-owned hardware
- Hardware and firmware customization
- Support for ISO15118 VAS-based vehicle battery monitoring, required for Korea's Fire Prevention EVSE Certification (administered by the Ministry of Environment)
- Compliance support for safety certifications
- OCPP 1.6 certification support
- Smart charging and load management functionality support
- Production and installation operation mode support
- HLC (High Level Communication) charging mode support (ISO15118-based)
- Charger status monitoring services

> [!NOTE]
> All paid support services are quoted separately based on project scope, duration, and complexity.

## Target Users and Use Cases
### EV Charger Manufacturers
Manufacturers can rapidly develop EVSE products without maintaining a dedicated
development team. From basic chargers without user authentication to advanced
OCPP and PLC-based automated authentication chargers, all are supported.
With continuously improving firmware, this solution reduces both initial
development risks and long-term maintenance costs.

### CPOs (Charge Point Operators)
CPOs can easily plan and develop their own branded chargers based on proven
hardware. By securing a production-ready prototype from the planning stage,
business speed and market responsiveness are significantly improved.

### EV Startups and Small Development Teams
Startups and small teams can build charger prototypes at minimal cost,
even with limited resources. The platform is designed to accommodate smart
charging, load management, and advanced feature extensions, making it the
optimal choice for early market entry and concept validation phases.

### DIY Enthusiasts
Individual EV owners can purchase and install the development board for
personal use. It is also well-suited for custom charger builds and DIY projects.

## Features

### System & Sensing
- Integrated posture and impact detection sensor
- GFCI and relay stuck detection
- Input and output power monitoring
- Internal temperature sensor for charger housing
- Emergency stop button

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

# EVSE Development Board (Korean Version)

## 지원 내용 및 비용
### 기본 제공
Pazzk EVSE Development Board를 구매하시면 다음 항목이 기본 제공됩니다:

- 11kW급 단상 AC 완속 충전기 레퍼런스 하드웨어
- 하드웨어 설계 자료
- 오픈소스 EVSE 펌웨어 (커뮤니티 버전)
- 기술 지원은 커뮤니티 포럼 및 GitHub Issue를 통한 비공식 지원 범위에 한정됩니다.

> [!NOTE]
> 별도의 전용 기술 지원이나 개별 상담은 제공되지 않으며,
> 추가 지원이 필요한 경우 별도 계약이 필요합니다.

### 선택적 추가 지원 (별도 계약)
Pazzk은 다음과 같은 전문 지원을 별도 계약을 통해 제공합니다:

- 고객 보유 하드웨어 대상 펌웨어 포팅 지원
- 커스터마이징 지원
- ISO15118 기반 화재 예방형 충전기 기능 제공
- 안전 인증 대응
- 화재 예방형 충전기 인증 대응
- OCPP 1.6 인증 대응
- 스마트 차징 및 부하 관리 기능 제공
- 생산 및 설치 운영모드 기능 및 방안 제공
- HLC(High Level Communication) 충전 모드 지원 (ISO15118 기반)
- 충전기 상태 모니터링

> [!NOTE]
> 모든 유상 지원은 프로젝트 범위, 기간, 난이도에 따라 별도 견적이 제안됩니다.

## 주요 사용처와 필요성
### 충전기 제조사
개발팀 없이도 완속 충전기 제품을 신속히 개발할 수 있습니다. 별도의 사용자
인증이 필요 없는 단순 충전기부터, OCPP 및 PLC 기반 자동 인증 충전기까지
지원하며, 지속적으로 개선되는 펌웨어를 제공합니다. 초기 개발 리스크는 물론,
장기 유지보수 비용까지 절감할 수 있습니다.

### CPO (Charge Point Operator)
자체 브랜드 충전기 기획 및 개발 시, 검증된 하드웨어를 기반으로 손쉽게 제품화할
수 있습니다.  기획 단계에서부터 즉시 사용 가능한 프로토타입을 확보하여,
사업 진행 속도와 시장 대응력을 높일 수 있습니다.

### 인증기관 및 인증 준비 고객
각종 인증에 대응할 수 있는 구조를 기반으로 테스트 및 인증 준비를 진행할 수
있습니다. 별도의 개발 리소스 없이 효율적인 인증 준비가 가능하며, 필요 시
OCPP 인증과 화재예방형 충전기 인증은 직접 대행도 가능합니다.

### EV 스타트업 및 소규모 개발팀
제한된 리소스 환경에서도 최소 비용으로 충전기 프로토타입을 제작할 수 있습니다.
스마트 충전, 부하 관리, 고급 기능 확장까지 고려한 설계가 가능하여, 초기 시장
진입 및 기획 검증 단계에 최적화된 선택입니다.

### 연구개발 및 교육기관
연구개발 프로젝트, 파일럿 테스트, 교육 목적으로 활용할 수 있습니다.
고급 통신 및 제어 기능까지 탑재되어 있어 다양한 실험 및 실습에 적합합니다.

### DIY 제작자
개인 전기차 오너도 구매하여 직접 설치 및 사용이 가능합니다.
자작 충전기 제작 및 커스터마이징에도 적합합니다.

## 주요 기능

### 시스템 및 센싱
- 자세 및 충격 감지 센서 통합
- GFCI 및 릴레이 stuck 감지
- 입력 및 출력 전력 모니터링
- 충전기 하우징 내부 온도 센서
- 비상 정지 버튼

### 사용자 인터페이스
- GUI 기반 LCD 디스플레이 (옵션)
- 상태 LED 및 음성 피드백 (부저, 스피커)

### 통신 기능
- 내장형 PLC 모뎀 (ISO 15118 준수)
- 지원되는 다양한 통신 인터페이스:
  - 이더넷 (10/100 Mbps, RJ45)
  - Wi-Fi (802.11 b/g/n)
  - BLE (블루투스 5.0)
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

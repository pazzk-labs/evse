[한국어](#quickstart-ko)

# Quickstart

This guide will walk you through setting up your environment and building the firmware to run on a compatible EVSE (Electric Vehicle Supply Equipment) device.

## Supported Environments

The Pazzk firmware can be built and executed across multiple environments. Currently, the following build targets are supported:

- **esp32s3**: ESP32-S3 based embedded EVSE firmware (default target)
- **host**: x86_64 or aarch64 host-native binary for local testing and debugging

> Additional targets and platforms will be supported in future releases.

Please refer to the target-specific instructions in the sections below.

## Prerequisites

Before you begin, ensure the following tools are installed on your development machine:

- Git (version >= 2.30)
- Python (version >= 3.9)
- ESP-IDF (version >= 5.4) for ESP32 builds only
- CMake (version >= 3.16)
- Ninja or GNU Make

## Clone the Repository

Clone the project repository and initialize submodules:

```bash
git clone --recursive https://github.com/pazzk-labs/evse.git
cd evse
```

If you already cloned the repository without `--recursive`, you can initialize submodules manually:

```bash
git submodule update --init --recursive
```

## Set Up the ESP-IDF Environment (esp32 target only)

Ensure you have the ESP-IDF toolchain properly installed. If not, follow the official ESP-IDF installation guide:
[https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/get-started](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/get-started/index.html)

Once installed, activate the environment:

```bash
. $IDF_PATH/export.sh
```

> Replace `$IDF_PATH` with the path to your local esp-idf clone if not set globally.

## Build the Firmware

### Quick Build (Recommended)

Use the unified Makefile interface:

```bash
make PLATFORM=esp32s3   # for ESP32-S3 firmware
make PLATFORM=host      # for host-native binary
```

Artifacts will be located in the `build/` directory for each platform.

### Advanced (Optional)

If you prefer using toolchain-specific commands:

#### ESP32-S3

```bash
idf.py build
```

#### Host

Ensure required system dependencies are installed.

```bash
cmake -B build/host -DTARGET_PLATFORM=host -G Ninja
cmake --build build/host
```

## Run Host CLI Simulation

The host-targeted firmware includes a lightweight command-line interface (CLI) for simulating EVSE behavior.

```bash
make PLATFORM=host run
```

Once launched, you can interact using the following commands:

```text
> help
> config show
> chg set pilot a
> idtag your_id
```

To exit the CLI:

```text
> exit
```

This is useful for verifying logic without flashing to hardware.

> A web-based simulator frontend is under development to provide a graphical interface on top of the CLI.

## Flash the Firmware (esp32 only)

Connect your ESP32-S3 device via USB and flash the firmware:

```bash
idf.py flash --port /dev/ttyUSB0
```

> Replace `/dev/ttyUSB0` with the appropriate serial port for your platform (e.g., `COM3` on Windows).

## Monitor the Output (esp32 only)

To monitor the serial output of the device after flashing:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

You should see logs from the bootloader, system init, and application start-up.

## Next Steps

- Check available CLI commands and syntax in the [CLI Reference](https://docs.pazzk.net/cli/cli_commands.html).
- Learn how to customize firmware behavior in [Configuration Guide](https://docs.pazzk.net/firmware/configuration.html).
- Review the [Architecture Overview](https://docs.pazzk.net/architecture/charger.html) to understand the system structure.
- Explore unit tests directly in the [tests/ directory](https://github.com/pazzk-labs/evse/tree/main/tests/src).

# Quickstart-ko

이 가이드는 EVSE(Electric Vehicle Supply Equipment) 장치에서 실행할 수 있는 펌웨어를 설정하고 빌드하는 방법을 안내합니다.

## 지원 환경

Pazzk 펌웨어는 여러 환경에서 빌드 및 실행할 수 있습니다. 현재 지원되는 빌드 대상은 다음과 같습니다:

- **esp32s3**: ESP32-S3 기반의 임베디드 EVSE 펌웨어 (기본 대상)
- **host**: x86_64 또는 aarch64 호스트에서 로컬 테스트 및 디버깅을 위한 네이티브 바이너리

> 향후 추가적인 대상 및 플랫폼이 지원될 예정입니다.

아래 섹션에서 대상별 지침을 참고하세요.

## 사전 요구 사항

개발을 시작하기 전에 다음 도구들이 개발 머신에 설치되어 있는지 확인하세요:

- Git (버전 >= 2.30)
- Python (버전 >= 3.9)
- ESP-IDF (버전 >= 5.4) - ESP32 빌드에만 필요
- CMake (버전 >= 3.16)
- Ninja 또는 GNU Make

## 저장소 클론

프로젝트 저장소를 클론하고 서브모듈을 초기화하세요:

```bash
git clone --recursive https://github.com/pazzk-labs/evse.git
cd evse
```

이미 `--recursive` 없이 클론한 경우, 서브모듈을 수동으로 초기화할 수 있습니다:

```bash
git submodule update --init --recursive
```

## ESP-IDF 환경 설정 (esp32 대상 전용)

ESP-IDF 툴체인이 올바르게 설치되어 있는지 확인하세요. 설치되지 않았다면, 공식 ESP-IDF 설치 가이드를 참고하세요:
[https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/get-started](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/get-started/index.html)

설치 후, 환경을 활성화하세요:

```bash
. $IDF_PATH/export.sh
```

> `$IDF_PATH`는 로컬 esp-idf 클론의 경로로, 전역으로 설정되지 않은 경우 직접 지정해야 합니다.

## 펌웨어 빌드

### 빠른 빌드 (권장)

통합된 Makefile 인터페이스를 사용하세요:

```bash
make PLATFORM=esp32s3   # ESP32-S3 펌웨어 빌드
make PLATFORM=host      # 호스트 네이티브 바이너리 빌드
```

각 플랫폼의 아티팩트는 `build/` 디렉토리에 생성됩니다.

### 고급 빌드 (옵션)

툴체인별 명령어를 선호하는 경우:

#### ESP32-S3

```bash
idf.py build
```

#### 호스트

필요한 시스템 종속성이 설치되어 있는지 확인하세요.

```bash
cmake -B build/host -DTARGET_PLATFORM=host -G Ninja
cmake --build build/host
```

## 호스트 CLI 시뮬레이션 실행

호스트 대상 펌웨어에는 EVSE 동작을 시뮬레이션할 수 있는 경량의 커맨드라인 인터페이스(CLI)가 포함되어 있습니다.

```bash
make PLATFORM=host run
```

실행 후, 다음과 같은 명령어로 상호작용할 수 있습니다:

```text
> help
> config show
> chg set pilot a
> idtag your_id
```

CLI를 종료하려면:

```text
> exit
```

하드웨어에 플래시하지 않고도 로직을 검증하는 데 유용합니다.

> 웹 기반 시뮬레이터 프론트엔드를 개발 중입니다. 기여 및 피드백을 환영합니다.

## 펌웨어 플래시 (esp32 전용)

ESP32-S3 장치를 USB를 통해 연결하고 펌웨어를 플래시하세요:

```bash
idf.py flash --port /dev/ttyUSB0
```

> `/dev/ttyUSB0`를 플랫폼에 맞는 적절한 시리얼 포트로 교체하세요 (예: Windows에서는 `COM3`).

## 출력 모니터링 (esp32 전용)

플래시 후 장치의 시리얼 출력을 모니터링하려면:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

부트로더, 시스템 초기화 및 애플리케이션 시작 로그를 확인할 수 있습니다.

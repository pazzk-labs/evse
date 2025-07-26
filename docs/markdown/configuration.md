# Configuration

> [!Warning]
> 설정을 추가하거나 삭제하는 경우(전체 구조에 변경이 발생하는 경우) 반드시
> 설정 버전을 증가해 기존 설정이 올바르게 마이그레이션되도록 해야 합니다.

## 설정 키 목록 및 사용법

### 설정 키 종류

| 설정 키               | 설명                    | 값 타입       | 디폴트 값   |
| --------------------- | ----------------------- | ------------- | ----------- |
| `version`             | 설정 버전               | `uint32_t`    | read-only   |
| `crc`                 | 설정 CRC                | `uint32_t`    | read-only   |
| `device.id`           | 디바이스 ID             | `char[24]`    | read-only   |
| `device.name`         | 디바이스 이름           | `char[32]`    | -           |
| `device.mode`         | 디바이스 동작 모드      | `uint8_t`     | -           |
| `log.mode`            | 로그 모드               | `uint8_t`     | None        |
| `log.level`           | 로그 레벨               | `uint8_t`     | Debug       |
| `dfu.reboot_manually` | DFU 재부팅 설정         | `bool`        | false       |
| `chg.mode`            | 충전기 모드             | `char[8]`     | free        |
| `chg.param`           | 충전기 설정             | `uint8_t[16]` | -           |
| `chg.count`           | 충전기 커넥터 수        | `uint8_t`     | 1           |
| `chg.c1.cp`           | 충전기 커넥터 1 CP      | `uint8_t[30]` | -           |
| `chg.c1.metering`     | 충전기 커넥터 1 미터링  | `uint8_t[16]` | -           |
| `chg.c1.plc_mac`      | 충전기 커넥터 1 PLC MAC | `uint8_t[6]`  | 02:00:00:fe:ed:00 |
| `net.mac`             | MAC 주소                | `uint8_t[6]`  | 00:f2:00:00:00:00 |
| `net.health`          | Health Check 주기       | `uint32_t`    | 60초        |
| `net.server.url`      | 서버 URL                | `char[256]`   | wss://csms.pazzk.net |
| `net.server.id`       | 서버 ID                 | `char[32]`    | -           |
| `net.server.pass`     | 서버 비밀번호           | `char[40]`    | -           |
| `net.server.ping`     | 서버 Ping 주기          | `uint32_t`    | 120초       |
| `net.opt`             | 네트워크 옵션           | `uint8_t`     | 0: Ethernet enabled |
| `x509.ca`             | 서버 인증서             | `char[2048]`  | -           |
| `x509.cert`           | 디바이스 인증서         | `char[2048]`  | -           |
| `ocpp.config`         | OCPP 설정               | `char[546]`   | -           |
| `ocpp.checkpoint`     | OCPP 체크포인트         | `char[16]`    | -           |
| `ocpp.vendor`         | OCPP 벤더               | `char[21]`    | net.pazzk   |
| `ocpp.model`          | OCPP 모델               | `char[21]`    | EVSE-7S     |

### 설정 읽기 (`config_read`)

```c
char did[CONFIG_DEVICE_ID_MAXLEN];
if (config_read("device.id", did, sizeof(did)) == 0) {
    printf("Device ID: %s\n", did);
}
```

### 설정 쓰기 (`config_write`)

```c
const char *name = "My charger";
if (config_write("device.name", name, strlen(name) + 1) == 0) {
    printf("Device name updated\n");
}
```

### 변경된 설정 저장 (`config_save`)

```c
if (config_save() == 0) {
    printf("Config saved successfully\n");
}
```

### 설정 초기화 (`config_reset`)

```c
config_reset(NULL);
printf("Factory reset completed\n");
```

### JSON 기반 설정 업데이트 (`config_update_json`)

```c
const char *json = "{\"log.mode\":\"console\",\"log.level\":\"debug\"}";
config_update_json(json, strlen(json));
```

### CLI를 통한 설정 변경
설정 값은 CLI에서도 변경할 수 있습니다. CLI 명령어를 활용한 설정 관리 방법은 [docs/cli_commands.md](docs/cli_commands.md) 문서를 참고하세요.

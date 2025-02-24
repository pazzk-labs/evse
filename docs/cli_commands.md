## 커맨드 목록
- command 혹은 subcommand 뒤에 값이 없으면 현재 설정값을 보여줍니다.
- 커맨드별 세부사항 및 상세 설명은 하단 [커맨드 설명](#커맨드-설명)을 참고하세요.

| Command   | Description                             |
| --------- | --------------------------------------- |
| `config`  | 설정 확인 및 변경                       |
| `exit`    | CLI 종료                                |
| `help`    | 도움말                                  |
| `info`    | 시스템 정보                             |
| `log`     | 시스템 로그                             |
| `md`      | 메모리 덤프                             |
| `metrics` | 메트릭 정보                             |
| `net`     | 네트워크 정보                           |
| `pilot`   | Control Pilot                           |
| `plc`     | PLC 모뎀 설정 확인 및 변경              |
| `reboot`  | 시스템 재부팅                           |
| `sec`     | 보안 설정                               |
| `test`    | 기능 테스트                             |
| `xmodem`  | 시리얼 터미널 XMODEM 통신               |

## 커맨드 설명
### `config`

| subcommand          | Description | Example               | Note            |
| ------------------- | ----------- | --------------------- | --------------- |
| set {chg\|mode}     | 설정 변경   | `config set chg/mode free` | 설정 리스트는 아래 테이블 참고 |
| get {chg\|mode\|id} | 설정 확인   | `config get chg`      |                 |

#### 설정 목록

| config | Description    | Options               | Note                  |
| ------ | -------------- | --------------------- | --------------------- |
| id     | 디바이스 ID    |                       |                       |
| mode   | 운영 모드      | `manufacturing`, `installing`, `production`, `development` | |
| chg/mode | 충전 모드    | `free`, `ocpp`, `hlc` |                        |
| chg/vol          | 입력 전압      |              | 단위: V               |
| chg/freq         | 입력 주파수    |              | 단위: Hz              |
| chg/input_curr   | 입력 최대 전류 |              | 단위: A               |
| chg/max_out_curr | 출력 최대 전류 |              | 단위: A               |
| chg/min_out_curr | 출력 최소 전류 |              | 단위: A               |
| chg/param | 충전 파라미터(vol,input_curr,freq,min_out_curr,max_out_curr) | | e.g. `config set chg/param 220 32 60 6 32`|
| x509/ca          | CA 인증서       |             |                      |
| x509/cert        | 디바이스 인증서 |             |                      |

> ![NOTE]
> 디바이스 인증서 업데이트시 저장된 CA 인증서로 인증서 유효성 검사를 수행합니다. 따라서 CA 인증서를 먼저 업데이트하세요.

### `exit`
### `help`
### `info`
버전, 디바이스 시리얼, CPU 로드, 힙 메모리등 시스템 정보를 출력합니다.

### `log`

| subcommand | Description    | Example               | Note |
| ---------- | -------------- | --------------------- | ---- |
| clear      | 모든 로그 삭제 | `log clear`           |      |
| rm         | 특정 로그 삭제 | `log rm 1734652800`   |      |
| show       | 특정 로그 보기 | `log show 1734652800` |      |
| level      | 로그 레벨 확인 | `log level`           |      |
| level      | 로그 레벨 설정 | `log level info`      | debug, info, error, none |
| set        | 로그 출력 설정 | `log set console`     | console, file, all, none |

- 로그 레벨 설정(`level`)은 `debug`가 디폴트이며, `info`로 설정하면 `debug` 로그가 출력되지 않습니다.
- 로그 출력 설정(`set`)은 `all`이 디폴트이며, `stdout`로 설정하면 파일로 로그를 출력하지 않습니다. `file`로 설정하면 파일로 로그를 출력하고 `stdout`로 로그를 출력하지 않습니다.

### `md`
메모리 덤프를 출력합니다. `md [주소] [길이]` 형식으로 사용합니다. 주소는 16진수로 입력합니다. 길이는 10진수로 입력합니다. 주소와 길이는 생략 가능합니다.

### `metrics`

| subcommand | Description          | Example        | Note |
| ---------- | -------------------- | -------------- | ---- |
| `clear`    | 메트릭 데이터 초기화 | `metric clear` |      |
| `show`     | 메트릭 데이터 보기   | `metric show`  |      |

### `net`

| subcommand       | Description          | Example                | Note |
| ---------------- | ----------------- | ---------------------- | ---- |
| `enable`         | 네트워크 활성화   | `net enable`           |      |
| `disable`        | 네트워크 비활성화 | `net disable`          |      |
| `ping {ip addr}` | 네트워크 테스트   | `net ping 192.168.0.1` |      |
| `mac`            | MAC 주소          | `net mac 11:22:33:44:55:66` |      |
| `url`            | 서버 URL 설정     | `net url wss://csms.pazzk.net:9000` | |
| `id`             | 인증 ID 설정      | `net id id_str`   |      |
| `pw`             | 인증 PW 설정      | `net pw pass_str` |      |
| `ws/ping`        | 웹소켓 핑 주기    | `net ws/ping 60`       | 단위: 초. 0일 경우 비활성화 |
| `health`         | 네트워크 health check 주기 | `net health 60`  | 단위: 초. 0일 경우 비활성화 |

### `pilot`

| subcommand | Description                             | Example | Note |
| ---------- | --------------------------------------- | ------- | ---- |
| scan_interval | CP 측정주기를 설정 | `pilot scan_interval 10` | 단위: ms |
| cutoff | 설정 전압보다 높으면 high로 인식 | `pilot cutoff 1996` | 단위: mV |
| hysteresis {upward\|downward} {A\|B\|C\|D\|E} | 상태별 전압 히스테리시스 설정 | `pilot hysteresis upward A 3038` | 단위: mV |
| sample_count | ADC 샘플링 갯수 설정 | `pilot sample_count 500` |      |
| noise_tolerance | 노이즈 마진 설정 | `pilot noise_tolerance 50` | 단위: mV |
| transition_clock | 상태 변환 시간 설정 | `pilot transition_clock 15` | 단위: ADC 샘플링 클럭(샘플링 갯수) |

### `plc`

| subcommand | Description                      | Example     | Note |
| ---------- | -------------------------------- | ----------- | ---- |
| `reset`    | 모뎀 재시작                      | `plc reset` |      |
| `mac`      | MAC 주소 확인 또는 변경          | `plc mac [11:22:33:44:55:66]` ||
| `dak`      | Device Access Key 확인 또는 변경 | `plc dak`   |      |
| `ver`      | 펌웨어 버전 보기                 | `plc ver`   |      |
| `pib`      | PIB 파일 헤더 정보 보기          | `plc pib`   |      |
| `fw`       | FW 파일 헤더 정보 보기           | `plc fw`    |      |
| `read`     | 모뎀으로부터 데이터 읽기         | `plc read`  |      |

### `reboot`
### `sec`

| subcommand | Description                             | Example | Note |
| ---------- | --------------------------------------- | ------- | ---- |
| `dfu {image\|sign}` | DFU 암호키 변경  | `sec dfu image 1234567890` | |
| `x509/key` | X.509 인증서 비밀키 생성  | `sec x509/key`             | |
| `x509/key/csr` | X.509 인증서 CSR 생성 및 읽기 | `sec x509/key/csr` | 생성할 경우 CN, C, O, E 순으로 입력. e.g. `sec x509/key/csr PZKC12411190001 KR Pazzk op@pazzk.net` |

- `image`: DFU 이미지 암호화용 AES-128 대칭키
- `sign`: DFU 서명용 비대칭키

> [!WARNING]
> CSR 생성시 CLI를 통해 입력한 정보는 유효성 검사를 거치지 않습니다. 올바른 정보를 입력하세요.

### `test`

| subcommand | Description          | Example         | Note |
| ---------- | -------------------- | --------------- | ---- |
| `led`      | LED 테스트           | `test led`      |      |
| `cp`       | Control Pilot 테스트 | `test cp`       |      |
| `plc`      | PLC 테스트           | `test plc`      |      |
| `relay`    | Relay 테스트         | `test relay`    |      |
| `buzzer`   | Buzzer 테스트        | `test buzzer`   |      |
| `temp`     | 온도 센서 테스트     | `test temp`     |      |
| `adc`      | ADC 테스트           | `test adc`      |      |
| `acc`      | 가속도 센서 테스트   | `test acc`      |      |
| `metering` | 전력량계 테스트      | `test metering` |      |
| `net`      | 네트워크 테스트      | `test net`      |      |
| `power`    | 전원 테스트          | `test power`    |      |
| `all`      | 모든 테스트          | `test all`      |      |

### `xmodem`

| subcommand | Description                               | Example      | Note |
| ---------- | ----------------------------------------- | ------------ | ---- |
| `dfu`      | DFU 이미지 다운로드 후 업데이트           | `xmodem dfu` |      |
| `test`     | 아무 파일이나 다운로드 후 삭제            | `xmodem test`|      |
| `pib`      | PLC PIB 파일을 `plc/pib.nvm`에 다운로드   | `xmodem pib` |      |
| `nvm`      | PLC 펌웨어 파일을 `plc/fw.nvm`에 다운로드 | `xmodem nvm` |      |

- XMODEM 통신 링크가 로그 출력 포트와 동일할 경우, 서로 간섭이 발생합니다. 따라서 파일을 다운로드할 때는 로그를 none으로 설정하고 진행하세요.

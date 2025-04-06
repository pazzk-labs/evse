# UID Storage Architecture

이 문서는 UID 인증 캐시 또는 Local Authorization List를 flash 파일 시스템
기반으로 저장하고 조회하기 위한 설계 원칙을 설명합니다. 목표는 수십만 개 UID를
안정적으로 저장하면서도, 메모리 캐시를 활용하여 빠른 인증 응답을 제공하는
것입니다.

향후에는 TTL 기반 만료 및 GC(garbage collection), 파일 압축(compaction)을 통한
추가 최적화도 계획하고 있습니다.

## 저장 구조 개요

| 항목 | 값 |
|------|-----|
| 저장 파일 시스템 | littlefs |
| UID 단위 구조체 크기 | 52 바이트 (`struct uid`) |
| 파일당 UID 수 | 512개 (총 26KB) |
| 저장 포맷 | packed struct array (고정 크기 binary format) |
| 디렉토리 구조 | 해시 기반 1바이트 prefix 디렉토리 샤딩 |
| GC 및 compaction 전략 | 추후 지원 예정 |

## 저장 구조체 정의

```c
struct uid_entry {
    uid_id_t id;         // 20B
    uid_id_t pid;        // 20B
    time_t expiry;       // 8B
    uid_status_t status; // 4B
};
```

- 총 52 바이트로 고정
- 파일에 512개 저장 → 1파일 = 26KB
- littlefs의 typical block size (4KB~64KB)에 적합

## 디렉토리 샤딩 전략

디렉토리 구조는 UID 또는 해시값의 상위 1바이트를 기준으로 분산됩니다.
이 구조는 디렉토리당 파일 수를 제한하고, littlefs의 GC 및 탐색 부담을 분산시킵니다.

### 샤딩 구조 예시 (1바이트 기준)

```
uid/
 ├── A3/
 │    ├── 00.bin
 │    └── 01.bin
 ├── 7F/
 │    └── 00.bin
```

### 샤딩 기준

- UID 또는 해시값의 첫 바이트를 디렉토리 이름으로 사용
- 총 256개 디렉토리 (`00/` ~ `FF/`) 생성 가능
- 각 디렉토리에 수십 개의 `XX.bin` 파일을 저장
- 파일이 없을 경우 생성 (create-on-write)
- 모든 파일은 binary packed struct 배열 형태로 저장됨

> ID가 편향된 구조라도 해시로 비교적 균등하게 분산 가능

### 구현 시 고려사항

- 파일 오픈 시:
  ```c
  char path[64];
  snprintf(path, sizeof(path), "uid/%02X/%02d.bin", id[0], page_index);
  ```
- 페이지마다 최대 512개 `uid_entry` 저장

### UID 검색 흐름

```c
1. id[0] → 디렉토리 결정 (예: A3 → uid/A3/)
2. 해당 디렉토리 내 모든 XX.bin 열어서:
    - 각 UID entry를 메모리로 로딩 or 스트리밍 탐색
    - 일치하는 UID 발견 시 → 캐시에 promote
```

> 파일당 데이터 수가 제한되므로 탐색 비용이 선형이라도 최대 수백 ms 이내 처리 가능

## 캐시 계층 (RAM)

- 최근 사용된 UID를 메모리에 유지
- LRU 기반으로 삽입/삭제
- 캐시 미스 시에만 flash 탐색 → 이후 캐시에 promote

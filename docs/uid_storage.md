# UID Storage Architecture

이 문서는 UID 인증 캐시 또는 Local Authorization List를 flash 파일 시스템을
기반으로 저장 및 조회하기 위한 설계를 정리합니다. 목표는 수십만 개의 UID를
안정적으로 저장하면서도, 메모리 캐시를 통해 빠른 검색 성능을 확보하는 것입니다.

이 구조는 littlefs 기반 flash 환경에서 안정성과 성능을 모두 고려한 설계입니다.
추후 GC 및 TTL 기반 compact 전략을 통해 추가 최적화가 가능합니다.

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
typedef struct __attribute__((packed)) {
    uid_id_t id;       // 20B
    uid_id_t pid;      // 20B
    time_t expiry;     // 8B
    uid_status_t status; // 4B
} uid_entry_t;
```

- 총 52 바이트로 고정
- 파일에 512개 저장 → 1파일 = 26KB
- littlefs의 typical block size (4KB~64KB)에 적합

## 디렉토리 샤딩 전략

디렉토리 하위에 UID를 해시 기반으로 분산 저장하여 디렉토리당 파일 수를 제한하고, 
littlefs의 디렉토리 탐색 비용과 GC 부담을 분산시킵니다.

### 샤딩 구조 예시 (1바이트 기준)

```
uid/
 ├── A3/
 │    ├── page_00.bin
 │    └── page_01.bin
 ├── 7F/
 │    └── page_00.bin
```

### 샤딩 기준

- UID 또는 해시값의 첫 바이트를 디렉토리 이름으로 사용
- 총 256개 디렉토리 (`00/` ~ `FF/`) 생성 가능
- 각 디렉토리에 수십 개의 `page_XX.bin` 파일을 저장

> ID가 편향된 구조라도 비교적 균등하게 분산 가능

### 구현 시 고려사항

- 파일 오픈 시:
  ```c
  char path[64];
  snprintf(path, sizeof(path), "uid/%02X/page_%02d.bin", id[0], page_index);
  ```
- 파일이 존재하지 않으면 생성
- 페이지마다 최대 512개 `uid_entry_t` 저장

### UID 검색 흐름

```c
1. id[0] → 디렉토리 결정 (예: A3 → uid/A3/)
2. 해당 디렉토리 내 모든 page_XX.bin 열어서:
    - 각 UID entry를 메모리로 로딩 or 스트리밍 탐색
    - 일치하는 UID 발견 시 → 캐시에 promote
```

> 파일당 데이터 수가 제한되므로 탐색 비용이 선형이라도 수 ms 이내 처리 가능

## 캐시 계층 (RAM)

- 최근 사용된 UID를 메모리에 유지
- LRU 기반으로 삽입/삭제
- 캐시 미스 시에만 flash 탐색 → 이후 캐시에 promote

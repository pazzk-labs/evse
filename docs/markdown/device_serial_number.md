# Device Serial Number

생산 고유 번호(Production Serial Number) 또는 디바이스 시리얼 번호(Device Serial Number)

[디바이스 리스트](https://docs.google.com/spreadsheets/d/13ubAndYqas2xgpq8aJKXZfMFF4OqCst7IUKbqI_G4Yo/)

> [!NOTE]
> 국가코드를 앞에 붙이면 IEC 63119-2 형식으로 사용 가능
> 예: `KR*PZK*EC12412240001`.
> 국가코드(`KR`) + 운영자코드(`PZK`) + 충전소 포트 식별번호(`EC1241224001`)

## 디바이스 시리얼 번호 형식

`PZK-EC1-241119-0001`(19자리) 또는 `PZKEC12411190001`(16자리)

- 첫번째 항목: `PZK`
  - PAZZK의 약자로 고정 접두어
- 두번째 항목: `C1`
  - 제품 라인과 제품 종류
  - `E`는 EVSE
- 세번째 항목: `241119`
  - 제조 날짜
- 네번째 항목: `0001`
  - 제조 날짜별 일련번호

## 제품 종류(모델명)

| 라인업   | 제품 종류| 코드 | 설명                      |
| -------- | -------- | ---- | ------------------------- |
| Core     | EVSE-7S  | S1   | 표준 기본 기능 모델, 7kW  |
| Core     | EVSE-11S | S2   | 표준 기본 기능 모델, 11kW |
| Connect  | EVSE-7C  | C1   | PnC 지원 모델, 7kW        |
| Connect  | EVSE-11C | C2   | PnC 지원 모델, 11kW       |
| GridLink | EVSE-7G  | G1   | V2G 지원 모델, 7kW        |
| GridLink | EVSE-11G | G2   | V2G 지원 모델, 11kW       |
| Flux     | EVSE-22F | F1   | 고출력 모델               |

## 노트
- 제조 날짜별 일련번호를 사용
  - 디바이스가 제조된 날짜와 시간을 기반으로 한 고유 식별자를 생성하면, 직관적이고 관리하기 쉬운 시리얼 번호를 만들 수 있음
  - 날짜와 일련번호가 포함되어 있어서
    - 언제 제조되었는지 쉽게 파악할 수 있고,
    - 일련번호를 통해 순차적으로 관리가 가능
  - 날짜 기반이므로 생산량이 많은 경우에도 유용하며, 간단한 방법으로 관리할 수 있음
- 일련번호는 4자리 사용하고 빈자리를 0으로 채움
  - 빈자리를 0으로 채우면
    - 숫자 크기에 관계없이 고정된 길이를 유지
      - 파일시스템이나 DB에서 정렬 시 숫자 크기와 관계없이 올바르게 정렬
    - 일관성 유지
      - 시각적으로 관리가 용이하고, 번호를 더 잘 인식할 수 있음

## SAN vs CN

디바이스가 TLS 인증서를 통해 자신을 식별할 때,
X.509 인증서의 다음 필드 중 어디에 ID를 넣어야 할지 명확한 기준 필요.

- CN (Common Name)
  - Subject 필드에 포함된 대표 문자열 (예: `CN=PZK-EC1-241119-0001`)
- SAN (Subject Alternative Name)
  - 확장 필드로, 여러 식별자를 구조화된 방식으로 표현 가능

| 항목 | CN | SAN |
|------|----|-----|
| 표준 권장 위치 | fallback 용도 | 우선 사용 (`RFC 5280`) |
| 다중 식별자 지원 | 불가 | 가능 (복수 DNS, IP, URI 등) |
| 의미론적 구조 표현 | 없음 | URI, otherName 등 가능 |
| 지원 범위 | 널리 호환됨 (레거시 포함) | 현대 TLS 스택 대부분 지원 |

### 실제 적용 예

- 예시 디바이스 ID
  - `PZK-EC1-241119-0001`
- 인증서 예시 구성

```yaml
subject:
  commonName: PZK-EC1-241119-0001
subjectAlternativeName:
    uris:
        - spiffe://pazzk.net/ec1/241119/0001
```

#### SPIFFE 기반 SAN URI (권장)
- `spiffe://pazzk.net/ec1/241119/0001`
  - `pazzk.net`는 조직 도메인
  - `ec1`은 제품 라인
  - `241119`는 제조 날짜
  - `0001`은 일련번호

### CN (Fallback 용도)
- `CN=PZK-EC1-241119-0001`
  - 레거시 시스템 호환을 위해 사용
  - SAN이 지원되지 않는 경우에만 사용

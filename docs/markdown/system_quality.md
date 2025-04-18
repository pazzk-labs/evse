# System Quality

Quality Attribute Tree는 소프트웨어 및 임베디드 시스템 설계 시 비기능
요구사항을 체계적으로 분류하고, 설계/구현/운영 상의 의사결정 기준을 명확히 하기
위한 도구입니다.

## 전체 트리 구조

```
System Quality Attributes
├── Functionality (기능성)
│   ├── Correctness (정확성)
│   └── Interoperability (상호운용성)
│
├── Reliability (신뢰성)
│   ├── Fault Tolerance (결함 허용)
│   ├── Availability (가용성)
│   └── Recoverability (복구성)
│
├── Performance (성능)
│   ├── Response Time (반응 시간)
│   ├── Throughput (처리량)
│   └── Resource Usage (CPU, RAM, Flash)
│
├── Maintainability (유지보수성)
│   ├── Modularity (모듈화)
│   ├── Readability (가독성)
│   ├── Testability (테스트 용이성)
│   └── Analyzability (분석 용이성)
│
├── Portability (이식성)
│   ├── Hardware Independence (HW 독립성)
│   └── OS / Toolchain Compatibility (도구 호환성)
│
├── Scalability (확장성)
│   ├── Functional Scaling (기능 확장)
│   └── Resource Scaling (하드웨어 확장)
│
├── Security (보안성)
│   ├── Confidentiality (기밀성)
│   ├── Integrity (무결성)
│   └── Authentication / Authorization (인증/인가)
│
├── Usability (사용성)
│   └── CLI, Web UI, Monitoring Tooling
│
└── Robustness (견고성)
    ├── Error Containment (오류 격리)
    └── Fail-Safe Behavior (안전한 실패)
```

## 각 특성 설명 및 임베디드 시스템 적용 예시

| 품질 특성 | 설명 | 임베디드 시스템 예시 |
|------------|------|----------------------|
| 신뢰성 Reliability | 주어진 시간 동안 오류 없이 동작할 확률 | 72시간 연속 동작 보장 |
| 복구성 Recoverability | 오류 후 빠르게 정상 상태로 복귀하는 능력 | watchdog 기반 재부팅 |
| 가용성 Availability | 시스템이 운영 가능한 시간의 비율 | 연간 99.9% uptime 보장 |
| 성능 Performance | 응답 시간, 처리량, 자원 사용 효율 | 100ms 이내 릴레이 응답 |
| 유지보수성 Maintainability | 코드 변경 및 점검의 용이성 | 계층 구조, 단위 테스트 구성 |
| 이식성 Portability | 다양한 HW/OS로 이전 가능성 | ESP32 외 다른 MCU 적용 가능성 |
| 확장성 Scalability | 기능 또는 자원 요구 증가 대응력 | 다채널 충전, 데이터 로깅 추가 |
| 보안성 Security | 무단 접근, 위변조 방지 | TLS, ID 검증, 인증서 기반 보안 |
| 견고성 Robustness | 예외 상황에서 안전하게 실패 | 릴레이 강제 차단, 전압 과다 시 차단 |

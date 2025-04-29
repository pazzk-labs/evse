# Smart Charging

## 요약

3가지 종류의 Profile을 제공함:

1. `ChargePointMaxProfile` - 충전기 전체의 최대 전력 제한
2. `TxDefaultProfile` -  트랜잭션 시작 시 자동 적용되는 기본 충전 프로파일
3. `TxProfile` - 특정 트랜잭션에 명시적으로 적용되는 프로파일

충전 스케줄링시 다음 순서로 적용됨:

`TxProfile` > `TxDefaultProfile` > `ChargePointMaxProfile`

즉, `ChargePointMaxProfile`은 충전소의 최상위 제한값이며, 그 위에 트랜잭션
단위로 더 제한적인 설정이 있으면 그것이 우선 적용됨.

`ChargePointMaxProfile` 은 단 하나의 프로필만 설정 가능하고, 새로 등록하면 덮어씀.

`TxDefaultProfile` 과 `TxProfile` 은 여러 개 등록 가능하고, 새로 등록하면 기존
프로필과 함께 동작함. 우선 순위는 StackLevel에 따라 결정됨. 즉,
적용 시점(validFrom ~ validTo)에 일치하는 프로필 중 StackLevel이 가장 높은 프로필이 적용됨.

Profile의 StackLevel은 중복될 수 없으며, 중복될 경우 새로운 프로필이 기존 프로필을 덮어씀.

Profile ID는 중복되어서는 안됨. 중복된 ID가 있을 경우, 동일한 type(purpose)의 프로필이면 기존 프로필을 덮어쓰고, 아니면 무시.

## Profile
- `ChargePointMaxProfile`
  - ConnectorId.0 만 가능
  - 여러개의 커넥터가 사용될 경우, 모든 커넥터의 출력 전력의 합이 ChargePointMaxProfile의 전력 제한을 초과할 수 없음
- `TxDefaultProfile`
  - ConnectorId.0 일 때, 모든 커넥터에 적용됨
  - 그 외에는 특정된 커넥터에만 적용됨
  - ConnectorId.0 으로 설정된 TxDefaultProfile이 있고, 특정 ConnectorId로 TxDefaultProfile이 새로 할당되면, 해당 ConnectorId에는 새로 할당된 TxDefaultProfile이 적용되고, ConnectorId.0 에는 여전히 이전 TxDefaultProfile이 적용됨
- `TxProfile`
  - ConnectorId.0 사용 불가능. 특정 ConnectorId에만 적용됨
  - 충전 중 상태가 아닌 경우 사용할 수 없음. 즉, 충전 중일 때만 사용 가능

## 노트
- 크게 3가지 방식으로 Smart Charging을 운영할 수 있음
  - Load balancing
    - EVSE가 독자적으로 전력 스케쥴링을 제어
  - Central smart charging
    - EVSE가 중앙 집중식으로 전력 스케쥴링을 제어
    - Charge Point에 Charge Point Max Profile을 설정하여, Charge Point 전체의 전력 제한을 설정
    - TxDefaultProfile과 TxProfile을 사용하여, 각 트랜잭션에 대한 전력 제한을 설정
  - Local smart charging
- CSMS 전제 조건
  - 동일한 배전망에 연결된 충전기를 그룹화하고 배전망이 제공하는 최대 전력량을 알고 있어야 함(설치시 입력)
- EVSE는 다음 설정에 응답해야 함
  - `ChargeProfileMaxStackLevel`
  - `ChargingScheduleAllowedChargingRateUnit`
  - `ChargingScheduleMaxPeriods`
  - `MaxChargingProfilesInstalled`
- 관련 OCPP 메시지
  - `ClearChargingProfile`
  - `GetCompositeSchedule`
  - `SetChargingProfile`
- role(ChargingPurpose)
  - `global`
  - `default`
  - `session`
  - `dynamic`
  - `override`
- OCPP의 Period가 여러개의 Profile로 분리되어 등록되기 때문에, pwrctrl 모듈에는 동일한 gid를(OCPP에서 profile ID) 갖는 여러개의 profile이 존재할 수 있음
  - 단, 동일한 gid와 함께 동일한 타임 period를 갖는 profile은 존재할 수 없음
- priority는 양수만 가능. 0은 reserved. 최대 < 0x10000
- profile priority에 앞서 scope priority가 우선 적용됨
  - OVERRIDE(0x80000) > DYNAMIC(0x40000) > SESSION(0x20000) > DEFAULT(0x10000) > GLOBAL(0)

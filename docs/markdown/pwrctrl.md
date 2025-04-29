[한국어](#power-control-system---korean-version)

# Power Control System

Power Control System Operational Specification.

## 1. Overview

The Power Control System manages dynamic adjustment of maximum output current based on registered profiles. It evaluates profiles according to their validity period and activation windows, ensuring that only appropriate limits are applied at any given time.

This document formalizes the operational rules, evaluation criteria, and API behavior related to power control.

## 2. Core Concepts

### 2.1 Profile Structure (`struct pwrctrl_profile`)
Each power control profile defines the following key fields:

- gid (`pwrctrl_profile_id_t`): Group ID of the profile.
- priority (`pwrctrl_priority_t`): Higher priority value overrides lower ones within the same scope.
- scope (`pwrctrl_scope_t`): Operational scope (Global, Default, Session, Dynamic, Override).
- activation_mode (`pwrctrl_activation_mode_t`): Activation mode (Absolute, Recurring).
- validity (`struct pwrctrl_period`): Valid date range (`from` and `to` timestamps).
- recurring (`pwrctrl_recurring_t`): Recurring pattern (Daily, Weekly, Yearly).
- activation_base (`time_t`): Base time for activation calculation.
- offset (`uint32_t`): Offset in seconds from the activation base.
- duration (`uint32_t`): Duration of activation in seconds.
- max_milliampere (`uint32_t`): Maximum allowed current in milliampere (mA).

### 2.2 Activation Conditions
A profile is considered **active** only if both of the following conditions are satisfied:

- Validity Check: The current time (`now`) must fall within the `validity.from` to `validity.to` range (inclusive).
- Activation Window Check: The current time must fall within the activation window defined by `activation_base`, `offset`, and `duration`, according to the `activation_mode` and `recurring` settings.

Both conditions must be true for a profile to be regarded as currently active.

## 3. API Behavior and Specifications

### 3.1 Profile Evaluation

#### `const struct pwrctrl_profile *pwrctrl_get_active(struct pwrctrl *pwrctrl, time_t now);`
- Returns a pointer to the currently active profile based on the current time.
- A profile is considered active only if it satisfies both validity and activation window.
- Returns `NULL` if no profile is active.

> [!Note]
> The returned profile is valid only momentarily. It must not be cached for future use without re-evaluation.

#### `uint32_t pwrctrl_get_max_milliampere(struct pwrctrl *pwrctrl, time_t now);`
- Returns the maximum output current in milliampere (mA) for the active profile at the current time.
- Returns `0` if no profile is active.

> [!Note]
> This reflects the active profile's `max_milliampere` only if both validity and activation window checks pass.

## 4. Overwrite and Conflict Resolution Logic

### 4.1 Overwrite Conditions
A newly added profile overwrites an existing profile if **all** the following conditions are met:

- `scope` matches.
- `priority` matches.
- `gid` differs.
- Activation windows overlap (activation_base + start_offset and duration).

If activation windows do **not** overlap, profiles can coexist even if scope and priority match.

### 4.2 GID Matching Special Rule
If an incoming profile has the **same gid** as an existing profile with the same scope and priority:

- Overwrite only if their activation windows overlap.
- Otherwise, allow coexistence (multiple profiles with different time periods under the same gid).

> [!Note]
> Overlapping activation windows between profiles with the same gid are considered configuration errors and must be resolved by overwrite.

## 5. Recurring Profile Behavior

### 5.1 Activation Modes
- Absolute: The activation window starts at `activation_base + offset` for `duration`.
- Recurring: Activation repeats at defined intervals (daily, weekly, etc.) starting from `activation_base`.

### 5.2 Recurring Period Units
| Recurring Type | Duration |
|:---|:---|
| Daily | 86400 seconds |
| Weekly | 604800 seconds |
| Yearly | 31536000 seconds |

> [!Note]
> For monthly and yearly recurring, approximate fixed seconds are used unless precise calendar logic is implemented.

## 6. Future Work

- Support for exact calendar month and year recurrences.

# Power Control System - Korean Version
Power Control System 운용 명세서 (Operational Specification)

## 1. 개요
Power Control System은 등록된 프로파일을 기반으로 최대 출력 전류를 동적으로
조정합니다. 현재 시간에 따라 프로파일의 유효 기간과 활성화 윈도우를 평가하여,
언제나 적절한 전류 제한이 적용되도록 합니다.

본 문서는 전력 제어에 관련된 운영 규칙, 평가 기준, API 동작을 공식화합니다.

## 2. 핵심 개념
### 2.1 프로파일 구조 (struct pwrctrl_profile)

각 전력 제어 프로파일은 다음 주요 필드들을 정의합니다:

- **gid** (`pwrctrl_profile_id_t`): 프로파일 그룹 ID.
- **priority** (`pwrctrl_priority_t`): 동일 scope 내에서 높은 priority 값이 우선합니다.
- **scope** (`pwrctrl_scope_t`): 적용 범위 (Global, Default, Session, Dynamic, Override).
- **activation_mode** (`pwrctrl_activation_mode_t`): 활성화 모드 (Absolute, Recurring).
- **validity** (`struct pwrctrl_period`): 유효 날짜 범위 (`from`, `to` 타임스탬프).
- **recurring** (`pwrctrl_recurring_t`): 반복 패턴 (Daily, Weekly, Yearly).
- **activation_base** (`time_t`): 활성화 기준 시간.
- **offset** (`uint32_t`): 기준 시간으로부터의 오프셋 (초 단위).
- **duration** (`uint32_t`): 활성화 지속 시간 (초 단위).
- **max_milliampere** (`uint32_t`): 허용 최대 전류 (밀리암페어, mA).

### 2.2 활성화 조건

프로파일은 다음 두 조건을 모두 만족할 때만 활성(active)으로 간주됩니다:

- 유효성 검사(Validity Check): 현재 시간(`now`)이 `validity.from` 이상이고 `validity.to` 이하 범위에 포함되어야 합니다. (포함 범위)
- 실행 윈도우 검사(Activation Window Check): `activation_base` + `offset` 기준으로 계산된 활성 구간 내에 현재 시간이 포함되어야 합니다.

## 3. API 동작 및 명세

### 3.1 프로파일 평가

#### `const struct pwrctrl_profile *pwrctrl_get_active(struct pwrctrl *pwrctrl, time_t now);`
- 현재 시간을 기준으로 활성 프로파일 포인터를 반환합니다.
- 유효성 검사와 실행 윈도우 검사를 모두 통과해야 합니다.
- 활성 프로파일이 없으면 `NULL`을 반환합니다.

#### `uint32_t pwrctrl_get_max_milliampere(struct pwrctrl *pwrctrl, time_t now);`
- 현재 시간에 해당하는 활성 프로파일의 최대 출력 전류(mA)를 반환합니다.
- 활성 프로파일이 없으면 `0`을 반환합니다.

## 4. 덮어쓰기(Overwrite) 및 충돌 해결 로직

### 4.1 덮어쓰기 발생 조건

신규 프로파일은 다음 모든 조건이 만족될 경우 기존 프로파일을 덮어쓰기(overwrite)합니다:

- `scope`가 동일할 것
- `priority`가 동일할 것
- `gid`가 다를 것
- 실행 윈도우가 겹칠 것

### 4.2 GID 일치 시 특별 규칙

- 들어오는 프로파일이 기존 프로파일과 `scope`, `priority`, `gid`가 모두 동일할 경우,
- 실행 윈도우가 겹치면 덮어쓰기 발생
- 실행 윈도우가 겹치지 않으면 둘 다 등록하여 서로 다른 활성 기간을 가질 수 있음

## 5. 반복 프로파일(Recurring Profile) 동작

### 5.1 활성화 모드

- Absolute: `activation_base + offset` 기준으로 1회 활성화
- Recurring: `activation_base` 기준으로 주기적으로 활성화 (daily, weekly 등)

### 5.2 반복 주기 단위

| 반복 유형 | 주기 시간(초) |
|:---|:---|
| Daily (일간) | 86400초 |
| Weekly (주간) | 604800초 |
| Yearly (연간) | 31536000초 |

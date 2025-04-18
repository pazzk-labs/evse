ADR Policy
===========

ADR 운영 규칙 (Architecture Decision Record Policy)

목적
-----
Pazzk 프로젝트는 설계 결정의 투명성, 일관성, 유지보수성을 위해 ADR(Architecture Decision Record)을 도입합니다. 이 문서는 ADR 작성, 관리, 리뷰에 대한 내부 규칙을 명시합니다. 본 프로젝트에서는 `npryce/adr-tools <https://github.com/npryce/adr-tools>`_ 를 사용하여 ADR을 작성 및 관리합니다.

작성 시점
---------
ADR은 다음 조건에 해당할 경우 반드시 작성해야 합니다:

- 시스템 구조에 영향을 주는 설계 변경
- 주요 품질 속성(성능, 보안, 유지보수성 등)에 영향이 있는 선택
- 외부 도구, 프로토콜, 기술 채택 또는 제거
- 팀 간 명시적 합의가 필요한 사안

단순 기능 구현, 버그 수정, UI 텍스트 변경 등은 ADR 작성 대상이 아닙니다.

작성 방법
---------
- ``docs/adr/`` 디렉토리에 마크다운 파일로 작성 (예: ``0004-enable-secure-boot.md``)
- 다음 필드를 포함해야 합니다:

  - 제목 (파일명과 일치)
  - 상태 (Proposed, Accepted, Superseded, Deprecated, Rejected)
  - 날짜
  - 결정 내용
  - 결정 근거 (이유, 대안, 품질 속성 영향 등)
  - 관련 PR 링크

템플릿 예시
^^^^^^^^^^^^
- 참고 링크: `adr-tools 템플릿 예시 <https://github.com/npryce/adr-tools/blob/master/src/template.md>`_

GitHub PR 연동
---------------
- PR에 설계 결정이 포함될 경우, ADR을 함께 포함해야 함
- PR 설명에 다음 형식 사용::

   Related ADR: `0004-enable-secure-boot </docs/adr/0004-enable-secure-boot.md>`_

- ADR이 이전 ADR을 대체할 경우:

  - 새 ADR에 ``Supersedes: 0002`` 추가
  - 기존 ADR 상태를 ``Superseded`` 로 갱신

리뷰 및 머지 정책
------------------
- 모든 ADR은 최소 1인 이상의 리뷰 후 ``Accepted`` 로 상태 변경
- 상태 변경은 PR에서 직접 수행하거나 리뷰 후 반영 가능
- 논쟁이 있는 결정은 임시로 ``Proposed`` 상태로 유지하며, 팀 합의 시까지 보류

관련 리소스
------------
- `adr-tools <https://github.com/npryce/adr-tools>`_
- `Michael Nygard, “Documenting Architecture Decisions” <https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions>`_
- ISO/IEC 25010 품질 속성 모델

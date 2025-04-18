.. Pazzk documentation master file, created by
   sphinx-quickstart on Tue Apr  8 16:48:47 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Pazzk Developer Documentation
=============================

Pazzk is built on certificate-based communication, real-time control, and
a maintainable firmware architecture. This documentation transparently outlines
the technical foundations, enabling anyone to verify and extend the system
with confidence.

Pazzk는 인증서 기반 통신, 실시간성 제어, 유지보수 가능한 펌웨어 구조를 핵심으로
합니다. 이 문서는 그 기술적 기반을 투명하게 공개하고, 누구나 검증하고 확장할
수 있도록 돕습니다.

.. toctree::
   :maxdepth: 1
   :caption: Getting Started

   quickstart
   cli/cli_commands

.. toctree::
   :maxdepth: 1
   :caption: Architecture

   architecture/charger
   architecture/network
   architecture/system_quality
   architecture/adr

.. toctree::
   :maxdepth: 1
   :caption: Communication / OCPP

   comms/ocpp_auth
   comms/ocpp_boot

.. toctree::
   :maxdepth: 1
   :caption: Electrical Interfaces

   electrical/control_pilot
   electrical/flash
   electrical/input
   electrical/power

.. toctree::
   :maxdepth: 1
   :caption: Firmware Reference

   firmware/configuration
   firmware/device_serial_number
   firmware/metrics
   firmware/operating_mode
   firmware/uid_storage

.. toctree::
   :maxdepth: 1
   :caption: API Reference

   api-reference/index

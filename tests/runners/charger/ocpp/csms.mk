# This file is part of the Pazzk project <https://pazzk.net/>.
# Copyright (c) 2025 Pazzk <team@pazzk.net>.
#
# Community Version License (GPLv3):
# This software is open-source and licensed under the GNU General Public
# License v3.0 (GPLv3). You are free to use, modify, and distribute this code
# under the terms of the GPLv3. For more details, see
# <https://www.gnu.org/licenses/gpl-3.0.en.html>.
# Note: If you modify and distribute this software, you must make your
# modifications publicly available under the same license (GPLv3), including
# the source code.
#
# Commercial Version License:
# For commercial use, including redistribution or integration into proprietary
# systems, you must obtain a commercial license. This license includes
# additional benefits such as dedicated support and feature customization.
# Contact us for more details.
#
# Contact Information:
# Maintainer: 권경환 Kyunghwan Kwon (on behalf of the Pazzk Team)
# Email: k@pazzk.net
# Website: <https://pazzk.net/>
#
# Disclaimer:
# This software is provided "as-is", without any express or implied warranty,
# including, but not limited to, the implied warranties of merchantability or
# fitness for a particular purpose. In no event shall the authors or
# maintainers be held liable for any damages, whether direct, indirect,
# incidental, special, or consequential, arising from the use of this software.

COMPONENT_NAME = csms

SRC_FILES = \
	../src/charger/ocpp/csms.c \
	../external/libmcu/modules/metrics/src/metrics.c \
	../external/libmcu/modules/metrics/src/metrics_overrides.c \

TEST_SRC_FILES = \
	src/charger/ocpp/csms_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	stubs/logger.c \
	../external/libmcu/tests/mocks/assert.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	mocks/ \
	../include \
	../src/charger/ocpp \
	../include/driver \
	../external/ocpp/include \
	../external/cJSON \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/logging/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/modules/ratelim/include \
	../external/libmcu/modules/fsm/include \
	../external/libmcu/interfaces/pwm/include \
	../external/libmcu/interfaces/spi/include \
	../external/libmcu/interfaces/gpio/include \
	../external/libmcu/interfaces/i2c/include \
	../external/libmcu/interfaces/adc/include \
	../external/libmcu/interfaces/uart/include \
	../external/libmcu/interfaces/kvstore/include \
	../external/libmcu/interfaces/flash/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -include ../include/logger.h \
	-DMETRICS_USER_DEFINES=\"../include/metrics.def\" \
	-DOCPP_ERROR=error \
	-DOCPP_DEBUG=debug \
	-DOCPP_INFO=info \
	-D_GNU_SOURCE \

include runners/MakefileRunner

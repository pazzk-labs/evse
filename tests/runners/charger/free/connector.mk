# This file is part of the Pazzk project <https://pazzk.net/>.
# Copyright (c) 2024 Pazzk <team@pazzk.net>.
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

COMPONENT_NAME = FreeConnector

SRC_FILES = \
	../src/charger/free/free_connector.c \
	../src/charger/connector.c \
	../external/libmcu/modules/fsm/src/fsm.c \
	../external/libmcu/modules/ratelim/src/ratelim.c \

TEST_SRC_FILES = \
	src/charger/free/connector_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	stubs/logger.c \
	mocks/metering.cpp \
	mocks/iec61851.cpp \
	mocks/safety.cpp \
	../external/libmcu/tests/stubs/metrics.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	../include \
	../src/charger \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/fsm/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/modules/ratelim/include \
	../external/libmcu/modules/logging/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -include ../include/logger.h

include runners/MakefileRunner

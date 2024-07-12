# This file is part of PAZZK EVSE project <https://pazzk.net/>.
# Copyright (c) 2024 Kyunghwan Kwon <k@libmcu.org>.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

COMPONENT_NAME = ControlPilot

SRC_FILES = \
	../src/pilot.c \
	../external/libmcu/modules/metrics/src/metrics.c \
	../external/libmcu/modules/metrics/src/metrics_overrides.c \
	../external/libmcu/modules/ratelim/src/ratelim.c \

TEST_SRC_FILES = \
	src/pilot_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	stubs/logger.c \
	../external/libmcu/tests/mocks/timext.cpp \
	../external/libmcu/tests/mocks/pwm.cpp \
	../external/libmcu/tests/mocks/assert.cpp \
	../external/libmcu/tests/stubs/board.cpp \
	../external/libmcu/tests/stubs/apptmr.cpp \
	../external/libmcu/tests/stubs/wdt.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	../include \
	../include/driver \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/logging/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/modules/ratelim/include \
	../external/libmcu/interfaces/pwm/include \
	../external/libmcu/interfaces/spi/include \
	../external/libmcu/interfaces/apptmr/include \
	../external/libmcu/interfaces/wdt/include \
	../external/libmcu/interfaces/flash/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -DMETRICS_USER_DEFINES=\"../include/metrics.def\"

include runners/MakefileRunner

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

COMPONENT_NAME = NetworkManager

SRC_FILES = \
	../src/net/netmgr.c \
	../external/libmcu/modules/metrics/src/metrics.c \
	../external/libmcu/modules/metrics/src/metrics_overrides.c \
	../external/libmcu/modules/fsm/src/fsm.c \
	../external/libmcu/modules/retry/src/retry.c \
	../external/libmcu/modules/common/src/ringbuf.c \
	../external/libmcu/modules/common/src/msgq.c \
	../external/libmcu/modules/common/src/bitops.c \

TEST_SRC_FILES = \
	src/net/netmgr_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	mocks/netif.cpp \
	../external/libmcu/tests/mocks/assert.cpp \
	../external/libmcu/tests/stubs/apptmr.cpp \
	../external/libmcu/tests/stubs/wdt.cpp \
	../external/libmcu/tests/stubs/timext.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	mocks/ \
	../include \
	../include/driver \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/logging/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/modules/fsm/include \
	../external/libmcu/modules/retry/include \
	../external/libmcu/interfaces/apptmr/include \
	../external/libmcu/interfaces/wdt/include \
	../external/libmcu/interfaces/flash/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -DNETMGR_CONNECT_TIMEOUT_MS=1000 \
	-DMETRICS_USER_DEFINES=\"../include/metrics.def\" \
	-include ../include/logger.h \
	-DFSM_INFO=info -DRETRY_INFO=info

ifeq ($(shell uname), Darwin)
TEST_SRC_FILES += ../external/libmcu/tests/fakes/fake_semaphore_ios.c
INCLUDE_DIRS += ../external/libmcu/modules/common/include/libmcu/posix
endif

include runners/MakefileRunner

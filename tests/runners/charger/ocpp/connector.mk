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

COMPONENT_NAME = OcppConnector

SRC_FILES = \
	../src/charger/ocpp/ocpp_connector.c \
	../src/charger/ocpp/ocpp_connector_internal.c \
	../src/charger/connector.c \
	../external/libmcu/modules/fsm/src/fsm.c \
	../external/libmcu/modules/ratelim/src/ratelim.c \

TEST_SRC_FILES = \
	src/charger/ocpp/connector_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	stubs/logger.c \
	stubs/metering.cpp \
	mocks/iec61851.cpp \
	mocks/safety.cpp \
	mocks/csms.cpp \
	mocks/ocpp.cpp \
	../external/libmcu/tests/mocks/assert.cpp \
	../external/libmcu/tests/stubs/metrics.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	mocks/ \
	../include \
	../src/charger/ocpp \
	../external/ocpp/include \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/fsm/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/modules/ratelim/include \
	../external/libmcu/modules/logging/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -include ../include/logger.h

include runners/MakefileRunner

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

COMPONENT_NAME = OcppDecoder

SRC_FILES = \
	../src/charger/ocpp/decoder_json.c \
	../external/libmcu/modules/metrics/src/metrics.c \
	../external/libmcu/modules/metrics/src/metrics_overrides.c \

TEST_SRC_FILES = \
	src/charger/ocpp/decoder_test.cpp \
	src/test_all.cpp \
	stubs/logging.c \
	stubs/logger.c \
	../external/libmcu/tests/mocks/assert.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	mocks/ \
	../include \
	../src/charger/ocpp \
	../external/ocpp/include \
	../external/cJSON \
	../external/libmcu/modules/common/include \
	../external/libmcu/modules/logging/include \
	../external/libmcu/modules/metrics/include \
	../external/libmcu/interfaces/flash/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -include ../include/logger.h \
	-DMETRICS_USER_DEFINES=\"../include/metrics.def\" \
	-DOCPP_ERROR=error \
	-DOCPP_DEBUG=debug \
	-DOCPP_INFO=info \
	-D_GNU_SOURCE \

include runners/MakefileRunner

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

COMPONENT_NAME = NetUtil

SRC_FILES = \
	../src/net/util.c \

TEST_SRC_FILES = \
	src/net/util_test.cpp \
	src/test_all.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	mocks/ \
	../include \

MOCKS_SRC_DIRS =

include runners/MakefileRunner

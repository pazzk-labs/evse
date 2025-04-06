/*
 * This file is part of the Pazzk project <https://pazzk.net/>.
 * Copyright (c) 2025 Pazzk <team@pazzk.net>.
 *
 * Community Version License (GPLv3):
 * This software is open-source and licensed under the GNU General Public
 * License v3.0 (GPLv3). You are free to use, modify, and distribute this code
 * under the terms of the GPLv3. For more details, see
 * <https://www.gnu.org/licenses/gpl-3.0.en.html>.
 * Note: If you modify and distribute this software, you must make your
 * modifications publicly available under the same license (GPLv3), including
 * the source code.
 *
 * Commercial Version License:
 * For commercial use, including redistribution or integration into proprietary
 * systems, you must obtain a commercial license. This license includes
 * additional benefits such as dedicated support and feature customization.
 * Contact us for more details.
 *
 * Contact Information:
 * Maintainer: 권경환 Kyunghwan Kwon (on behalf of the Pazzk Team)
 * Email: k@pazzk.net
 * Website: <https://pazzk.net/>
 *
 * Disclaimer:
 * This software is provided "as-is", without any express or implied warranty,
 * including, but not limited to, the implied warranties of merchantability or
 * fitness for a particular purpose. In no event shall the authors or
 * maintainers be held liable for any damages, whether direct, indirect,
 * incidental, special, or consequential, arising from the use of this software.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "iec61851.h"

TEST_GROUP(iec61851) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(iec61851, ShouldReturnDuty100_WhenZeroMilliampereGiven) {
	LONGS_EQUAL(100, iec61851_milliampere_to_duty(0));
}
TEST(iec61851, ShouldReturnDuty96_WhenGreaterThan80MilliampereGiven) {
	LONGS_EQUAL(96, iec61851_milliampere_to_duty(80000));
	LONGS_EQUAL(96, iec61851_milliampere_to_duty(80001));
	LONGS_EQUAL(96, iec61851_milliampere_to_duty(90000));
	LONGS_EQUAL(96, iec61851_milliampere_to_duty(100000));
}
TEST(iec61851, ShouldReturnDuty_WhenBetween52And80MilliampereGiven) {
	LONGS_EQUAL(86, iec61851_milliampere_to_duty(52000));
	LONGS_EQUAL(86, iec61851_milliampere_to_duty(55000));
	LONGS_EQUAL(88, iec61851_milliampere_to_duty(60000));
	LONGS_EQUAL(90, iec61851_milliampere_to_duty(65000));
	LONGS_EQUAL(92, iec61851_milliampere_to_duty(70000));
	LONGS_EQUAL(94, iec61851_milliampere_to_duty(75000));
	LONGS_EQUAL(96, iec61851_milliampere_to_duty(80000));
}
TEST(iec61851, ShouldReturnDuty_WhenBetween6And51MilliampereGiven) {
	LONGS_EQUAL(10, iec61851_milliampere_to_duty(6000));
	LONGS_EQUAL(16, iec61851_milliampere_to_duty(9600));
	LONGS_EQUAL(17, iec61851_milliampere_to_duty(10200));
	LONGS_EQUAL(25, iec61851_milliampere_to_duty(15000));
	LONGS_EQUAL(30, iec61851_milliampere_to_duty(18000));
	LONGS_EQUAL(33, iec61851_milliampere_to_duty(19800));
	LONGS_EQUAL(40, iec61851_milliampere_to_duty(24000));
	LONGS_EQUAL(50, iec61851_milliampere_to_duty(30000));
	LONGS_EQUAL(53, iec61851_milliampere_to_duty(31800));
	LONGS_EQUAL(53, iec61851_milliampere_to_duty(31818));
	LONGS_EQUAL(66, iec61851_milliampere_to_duty(39600));
	LONGS_EQUAL(83, iec61851_milliampere_to_duty(49800));
	LONGS_EQUAL(85, iec61851_milliampere_to_duty(51000));
}
TEST(iec61851, ShouldReturnDuty100_WhenLessThan6MilliampereGiven) {
	LONGS_EQUAL(100, iec61851_milliampere_to_duty(5999));
	LONGS_EQUAL(100, iec61851_milliampere_to_duty(5000));
	LONGS_EQUAL(100, iec61851_milliampere_to_duty(1));
}

TEST(iec61851, ShouldReturnZeroMilliampere_WhenDutyIsGreaterThan97) {
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(98));
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(99));
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(100));
}
TEST(iec61851, ShouldReturn80KMilliampere_WhenDutyIs97) {
	LONGS_EQUAL(80000, iec61851_duty_to_milliampere(97));
}
TEST(iec61851, ShouldReturnMilliampere_WhenDutyIsBetween85And96) {
	LONGS_EQUAL(55000, iec61851_duty_to_milliampere(86));
	LONGS_EQUAL(60000, iec61851_duty_to_milliampere(88));
	LONGS_EQUAL(65000, iec61851_duty_to_milliampere(90));
	LONGS_EQUAL(70000, iec61851_duty_to_milliampere(92));
	LONGS_EQUAL(75000, iec61851_duty_to_milliampere(94));
	LONGS_EQUAL(80000, iec61851_duty_to_milliampere(96));
}
TEST(iec61851, ShouldReturnMilliampere_WhenDutyIsBetween10And85) {
	LONGS_EQUAL(6000, iec61851_duty_to_milliampere(10));
	LONGS_EQUAL(9600, iec61851_duty_to_milliampere(16));
	LONGS_EQUAL(10200, iec61851_duty_to_milliampere(17));
	LONGS_EQUAL(15000, iec61851_duty_to_milliampere(25));
	LONGS_EQUAL(18000, iec61851_duty_to_milliampere(30));
	LONGS_EQUAL(19800, iec61851_duty_to_milliampere(33));
	LONGS_EQUAL(24000, iec61851_duty_to_milliampere(40));
	LONGS_EQUAL(30000, iec61851_duty_to_milliampere(50));
	LONGS_EQUAL(31800, iec61851_duty_to_milliampere(53));
	LONGS_EQUAL(39600, iec61851_duty_to_milliampere(66));
	LONGS_EQUAL(49800, iec61851_duty_to_milliampere(83));
	LONGS_EQUAL(51000, iec61851_duty_to_milliampere(85));
}
TEST(iec61851, ShouldReturn6KMilliampere_WhenDutyIsBetween8And10) {
	LONGS_EQUAL(6000, iec61851_duty_to_milliampere(8));
	LONGS_EQUAL(6000, iec61851_duty_to_milliampere(9));
	LONGS_EQUAL(6000, iec61851_duty_to_milliampere(10));
}
TEST(iec61851, ShouldReturnZeroMilliampere_WhenDutyIsLessThan8) {
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(7));
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(6));
	LONGS_EQUAL(0, iec61851_duty_to_milliampere(5));
}

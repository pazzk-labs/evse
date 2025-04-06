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

extern "C" {
    #include "uptime.h"
    #include <time.h>
}

// Mock for time function
extern "C" {
    time_t mock_time = 0;
    
    time_t time(time_t *tloc) {
        if (tloc != NULL) {
            *tloc = mock_time;
        }
        return mock_time;
    }
}

TEST_GROUP(uptime) {
    void setup() {
        mock_time = 1000; // Start with a non-zero time
        uptime_init();
    }

    void teardown() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(uptime, ShouldReturnZeroUptimeInitially) {
    // After initialization, uptime should be 0
    time_t result = uptime_get();
    LONGS_EQUAL(0, result);
}

TEST(uptime, ShouldReturnCorrectUptimeAfterTimeChange) {
    // Advance the mock time by 60 seconds
    mock_time += 60;
    
    // Uptime should now be 60 seconds
    time_t result = uptime_get();
    LONGS_EQUAL(60, result);
}

TEST(uptime, ShouldHandleLargeTimeValues) {
    // Set a large time value
    mock_time += 86400; // 1 day
    
    // Uptime should be 1 day
    time_t result = uptime_get();
    LONGS_EQUAL(86400, result);
}

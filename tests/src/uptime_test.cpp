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
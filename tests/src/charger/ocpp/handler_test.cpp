#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "handler.h"

TEST_GROUP(OcppHandler) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppHandler, t) {
}

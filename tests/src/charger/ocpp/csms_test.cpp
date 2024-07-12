#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "csms.h"

TEST_GROUP(CSMS) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(CSMS, t) {
}

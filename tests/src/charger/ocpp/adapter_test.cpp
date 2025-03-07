#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "adapter.h"

TEST_GROUP(OcppMessage) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppMessage, t) {
}

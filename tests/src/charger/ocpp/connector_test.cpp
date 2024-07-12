#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "connector_private.h"

TEST_GROUP(OcppConnector) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppConnector, t) {
}

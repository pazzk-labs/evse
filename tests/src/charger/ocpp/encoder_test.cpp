#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "encoder_json.h"

TEST_GROUP(OcppEncoder) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppEncoder, t) {
}

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "decoder_json.h"

TEST_GROUP(OcppDecoder) {
	void setup(void) {
	}
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(OcppDecoder, t) {
}

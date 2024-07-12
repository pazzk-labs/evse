#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "pilot.h"
#include "libmcu/apptmr.h"
#include "libmcu/metrics.h"
#include "adc122s051.h"

#if !defined(ARR_SIZE)
#define ARR_SIZE(x)	(sizeof(x) / sizeof(x[0]))
#endif

static struct apptmr *apptmr;

static const uint16_t samples_5pct[] = {
554,554,554,554,554,554,554,557,554,554,554,554,554,557,557,557,557,554,554,557,
557,554,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,603,858,
1293,1828,2389,2818,3039,3108,3128,3131,3131,3135,3135,3135,3135,3135,3135,3135,3135,3135,3135,3135,
3135,3135,3135,2989,2603,2052,1452,1013,755,643,594,574,564,561,561,557,557,557,557,557,
557,554,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
};
static const uint16_t samples_2pct[] = {
554,554,554,554,554,554,554,554,554,557,557,557,557,557,554,557,554,557,557,554,
554,557,554,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,561,735,
1105,1613,2174,2676,2976,3092,3125,3131,3125,2910,2475,1897,1323,927,712,623,587,570,561,561,
561,557,557,557,557,557,557,554,557,557,557,554,557,557,557,554,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
};
static const uint16_t samples_100pct[] = {
3141,3141,3141,3141,3141,3141,3141,3141,3138,3138,3138,3138,3138,3138,3138,3138,3141,3141,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,3138,
};
static const uint16_t samples_0pct[] = {
554,554,554,554,554,554,554,554,554,554,554,554,554,554,557,554,554,554,554,554,
554,554,554,557,557,557,557,554,554,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,554,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,557,
};

void apptmr_create_hook(struct apptmr *self) {
	apptmr = self;
}

int adc122s051_measure(struct adc122s051 *self,
		uint16_t adc_samples[], const uint16_t nr_samples,
		struct adc122s051_callback *cb) {
	int err = mock().actualCall("adc122s051_measure")
		.withOutputParameter("adc_samples", adc_samples)
		.withParameter("nr_samples", nr_samples)
		.returnIntValue();

	if (cb && cb->on_sample) {
		for (uint16_t i = 0; i < nr_samples; i++) {
			adc_samples[i] = cb->on_sample(cb->on_sample_ctx, adc_samples[i]);
		}
	}

	return err;
}

uint16_t adc122s051_convert_adc_to_millivolt(uint16_t adc_raw) {
	return adc_raw;
}

TEST_GROUP(ControlPilot) {
	struct pilot_params params;
	struct pilot *pilot;
	uint16_t sample_buffer[500];

	void setup(void) {
		metrics_init(true);

		pilot_default_params(&params);
		pilot = pilot_create(&params, 0, 0, sample_buffer);

		mock().expectOneCall("pwm_enable").ignoreOtherParameters()
			.andReturnValue(0);
		mock().expectOneCall("pwm_start").ignoreOtherParameters()
			.andReturnValue(0);
		pilot_enable(pilot);
	}
	void teardown(void) {
		mock().expectOneCall("pwm_stop").ignoreOtherParameters()
			.andReturnValue(0);
		mock().expectOneCall("pwm_disable").ignoreOtherParameters()
			.andReturnValue(0);
		pilot_disable(pilot);
		pilot_delete(pilot);

		mock().checkExpectations();
		mock().clear();
	}
	void shift_phase(uint16_t *samples, size_t count) {
		uint16_t t = samples[0];
		for (size_t i = 0; i < count-1; i++) {
			samples[i] = samples[i+1];
		}
		samples[count-1] = t;
	}

	void expect_sampling_data(const uint16_t *samples, size_t count) {
		const size_t size = sizeof(samples[0]) * count;
		mock().expectOneCall("adc122s051_measure")
			.withOutputParameterReturning("adc_samples", samples, size)
			.withParameter("nr_samples", count)
			.ignoreOtherParameters()
			.andReturnValue(0);
	}
	void expect_duty(uint8_t duty) {
		mock().expectOneCall("pwm_update_duty")
			.ignoreOtherParameters()
			.andReturnValue(0);
		pilot_set_duty(pilot, duty);
	}
};

TEST(ControlPilot, ShouldReturnStatusString) {
	STRCMP_EQUAL("A(12V)", pilot_stringify_status(PILOT_STATUS_A));
	STRCMP_EQUAL("B(9V)", pilot_stringify_status(PILOT_STATUS_B));
	STRCMP_EQUAL("C(6V)", pilot_stringify_status(PILOT_STATUS_C));
	STRCMP_EQUAL("D(3V)", pilot_stringify_status(PILOT_STATUS_D));
	STRCMP_EQUAL("E(0V)", pilot_stringify_status(PILOT_STATUS_E));
	STRCMP_EQUAL("F(-12V)", pilot_stringify_status(PILOT_STATUS_F));
	STRCMP_EQUAL("Unknown", pilot_stringify_status((pilot_status_t)-1));
}

TEST(ControlPilot, ShouldReturnUnknownStatus_WhenCalledJustAfterBoot) {
	LONGS_EQUAL(PILOT_STATUS_UNKNOWN, pilot_status(pilot));
}

TEST(ControlPilot, ShouldReturnZeroDuty_WhenCalledJustAfterBoot) {
	LONGS_EQUAL(0, pilot_duty(pilot));
}

TEST(ControlPilot, ShouldReturnSanityOK_WhenCalledJustAfterBoot) {
	LONGS_EQUAL(true, pilot_ok(pilot));
}

TEST(ControlPilot, ShouldReturnZeroMillivolt_WhenCalledJustAfterBoot) {
	LONGS_EQUAL(0, pilot_millivolt(pilot, false));
}

TEST(ControlPilot, ShouldReturnDutyMismatchError_WhenDifferentSamplingDataGiven) {
	expect_sampling_data(samples_5pct, ARR_SIZE(samples_5pct));
	apptmr_trigger(apptmr);
	LONGS_EQUAL(PILOT_ERROR_DUTY_MISMATCH, pilot_error(pilot));
}

TEST(ControlPilot, ShouldReturnStatusA_WhenAStatusSamplingDataGiven) {
	/* to not print duty mismatch warning */
	expect_duty(5);
	expect_sampling_data(samples_5pct, ARR_SIZE(samples_5pct));
	apptmr_trigger(apptmr);
	LONGS_EQUAL(PILOT_STATUS_A, pilot_status(pilot));

	expect_duty(2);
	expect_sampling_data(samples_2pct, ARR_SIZE(samples_2pct));
	apptmr_trigger(apptmr);
	LONGS_EQUAL(PILOT_STATUS_A, pilot_status(pilot));

	expect_duty(100);
	expect_sampling_data(samples_100pct, ARR_SIZE(samples_100pct));
	apptmr_trigger(apptmr);
	LONGS_EQUAL(PILOT_STATUS_A, pilot_status(pilot));
}

TEST(ControlPilot, pct100_ShouldReturnTheSameVoltage_WhenTheSameSamplingDataWithDifferentPhaseGiven) {
	const size_t count = ARR_SIZE(samples_100pct);
	uint16_t buf[count];
	memcpy(buf, samples_100pct, sizeof(samples_100pct));

	expect_duty(100);

	for (size_t i = 0; i < count; i++) {
		expect_sampling_data(buf, count);
		apptmr_trigger(apptmr);

		LONGS_EQUAL(3141, pilot_millivolt(pilot, false));
		LONGS_EQUAL(0, pilot_millivolt(pilot, true));

		shift_phase(buf, count);
	}
}

TEST(ControlPilot, pct0_ShouldReturnTheSameVoltage_WhenTheSameSamplingDataWithDifferentPhaseGiven) {
	const size_t count = ARR_SIZE(samples_0pct);
	uint16_t buf[count];
	memcpy(buf, samples_0pct, sizeof(samples_0pct));

	expect_duty(0);

	for (size_t i = 0; i < count; i++) {
		expect_sampling_data(buf, count);
		apptmr_trigger(apptmr);

		LONGS_EQUAL(0, pilot_millivolt(pilot, false));
		LONGS_EQUAL(554, pilot_millivolt(pilot, true));

		shift_phase(buf, count);
	}
}

TEST(ControlPilot, pct5_ShouldReturnTheSameVoltage_WhenTheSameSamplingDataWithDifferentPhaseGiven) {
	const size_t count = ARR_SIZE(samples_5pct);
	uint16_t buf[count];
	memcpy(buf, samples_5pct, sizeof(samples_5pct));

	expect_duty(0);

	for (size_t i = 0; i < count; i++) {
		expect_sampling_data(buf, count);
		apptmr_trigger(apptmr);

		LONGS_EQUAL(3135, pilot_millivolt(pilot, false));
		LONGS_EQUAL(554, pilot_millivolt(pilot, true));

		shift_phase(buf, count);
	}
}

TEST(ControlPilot, pct2_ShouldReturnTheSameVoltage_WhenTheSameSamplingDataWithDifferentPhaseGiven) {
	const size_t count = ARR_SIZE(samples_2pct);
	uint16_t buf[count];
	memcpy(buf, samples_2pct, sizeof(samples_2pct));

	expect_duty(0);

	for (size_t i = 0; i < count; i++) {
		expect_sampling_data(buf, count);
		apptmr_trigger(apptmr);

		LONGS_EQUAL(3131, pilot_millivolt(pilot, false));
		LONGS_EQUAL(554, pilot_millivolt(pilot, true));

		shift_phase(buf, count);
	}
}

TEST(ControlPilot, ShouldReturnStatusB_WhenBStatusSamplingDataGiven) {
}
TEST(ControlPilot, ShouldReturnStatusC_WhenCStatusSamplingDataGiven) {
}
TEST(ControlPilot, ShouldReturnStatusD_WhenDStatusSamplingDataGiven) {
}
TEST(ControlPilot, ShouldReturnStatusE_WhenEStatusSamplingDataGiven) {
}
TEST(ControlPilot, ShouldReturnStatusF_WhenFStatusSamplingDataGiven) {
}
TEST(ControlPilot, ShouldReturnFluctuatingError_WhenFluctuatingSamplingDataGiven) {
}

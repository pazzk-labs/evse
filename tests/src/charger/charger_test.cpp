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

#include "charger_internal.h"
#include "charger/connector.h"

TEST_GROUP(Charger) {
	struct charger charger;
	struct charger_param param;

	void setup(void) {
		charger_default_param(&param);
		charger_init(&charger, &param, NULL);
	}
	void teardown(void) {
		charger_deinit(&charger);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(Charger, ShouldReturnDefaultParam) {
	charger_default_param(&param);

	LONGS_EQUAL(31818, param.max_input_current_mA);
	LONGS_EQUAL(220, param.input_voltage);
	LONGS_EQUAL(60, param.input_frequency);
	LONGS_EQUAL(31818, param.max_output_current_mA);
	LONGS_EQUAL(31818, param.min_output_current_mA);
}
TEST(Charger, ShouldReturnEINVAL_WhenInvalidParamGiven) {
	charger_default_param(&param);
	param.max_input_current_mA = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));

	charger_default_param(&param);
	param.input_voltage = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));

	charger_default_param(&param);
	param.input_frequency = 0;
	LONGS_EQUAL(-EINVAL, charger_init(&charger, &param, NULL));
}
TEST(Charger, ShouldReturnZeroConnectors_WhenJustCreated) {
	LONGS_EQUAL(0, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnNull_WhenNoConnectorAttached) {
	LONGS_EQUAL(NULL, charger_get_connector_by_id(&charger, 0));
	LONGS_EQUAL(NULL, charger_get_connector_available(&charger));
	LONGS_EQUAL(NULL, charger_get_connector(&charger, ""));
	LONGS_EQUAL(NULL, charger_get_connector(&charger, NULL));
}
TEST(Charger, ShouldAttachConnector) {
	struct connector c;
	int id;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(1, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c, &id));
	LONGS_EQUAL(1, id);
}
TEST(Charger, ShouldReturnConnector_WhenConnectorIdGiven) {
	struct connector c;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(&c, charger_get_connector_by_id(&charger, 1));
}
TEST(Charger, ShouldReturnAvailableConnector_WhenAvailable) {
	struct connector c;
	mock().expectOneCall("connector_is_enabled").andReturnValue(true);
	mock().expectOneCall("connector_is_occupied").andReturnValue(false);
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(&c, charger_get_connector_available(&charger));
}
TEST(Charger, ShouldDetachConnector) {
	struct connector c;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c));
	LONGS_EQUAL(1, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_detach_connector(&charger, &c));
	LONGS_EQUAL(0, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnCountOfConnectors_WhenConnectorAttached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(3, charger_count_connectors(&charger));
}
TEST(Charger, ShouldReturnUniqueConnectorID_WhenMultipleConnectorsAttached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
}
TEST(Charger, ShouldReturnUniqueConnectorID_WhenMultipleConnectorsAttachedAndDetached) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
	LONGS_EQUAL(0, charger_detach_connector(&charger, &c2));
	LONGS_EQUAL(2, charger_count_connectors(&charger));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(4, id2);
	LONGS_EQUAL(3, charger_count_connectors(&charger));
}
TEST(Charger, ShouldProcessAllConnectors) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));

	mock().expectOneCall("connector_process").withParameter("self", &c1);
	mock().expectOneCall("connector_process").withParameter("self", &c2);
	mock().expectOneCall("connector_process").withParameter("self", &c3);

	LONGS_EQUAL(0, charger_process(&charger));
}
TEST(Charger, ShouldReturnConnectorID_WhenConnectorGiven) {
	struct connector c1;
	struct connector c2;
	struct connector c3;
	int id1, id2, id3;
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c1));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c2));
	LONGS_EQUAL(0, charger_attach_connector(&charger, &c3));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c1, &id1));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c2, &id2));
	LONGS_EQUAL(0, charger_get_connector_id(&charger, &c3, &id3));
	LONGS_EQUAL(1, id1);
	LONGS_EQUAL(2, id2);
	LONGS_EQUAL(3, id3);
}

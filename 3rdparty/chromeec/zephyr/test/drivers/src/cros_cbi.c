/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <device.h>
#include <ztest.h>

#include "drivers/cros_cbi.h"

static void test_check_match(void)
{
	const struct device *dev = device_get_binding(CROS_CBI_LABEL);
	int value;

	zassert_not_null(dev, NULL);

	value = cros_cbi_ssfc_check_match(
		dev, CBI_SSFC_VALUE_ID(DT_NODELABEL(base_sensor_0)));
	zassert_true(value, "Expected cbi ssfc to match base_sensor_0");

	value = cros_cbi_ssfc_check_match(
		dev, CBI_SSFC_VALUE_ID(DT_NODELABEL(base_sensor_1)));
	zassert_false(value, "Expected cbi ssfc not to match base_sensor_1");

	value = cros_cbi_ssfc_check_match(dev, CBI_SSFC_VALUE_COUNT);
	zassert_false(value, "Expected cbi ssfc to fail on invalid enum");
}

static void test_fail_check_match(void)
{
	const struct device *dev = device_get_binding(CROS_CBI_LABEL);
	int value;

	zassert_not_null(dev, NULL);

	value = cros_cbi_ssfc_check_match(dev, CBI_SSFC_VALUE_COUNT);
	zassert_false(value,
		      "Expected cbi ssfc to never match CBI_SSFC_VALUE_COUNT");
}

static void test_fw_config(void)
{
	const struct device *dev = device_get_binding(CROS_CBI_LABEL);
	uint32_t value;
	int ret;

	zassert_not_null(dev, NULL);

	ret = cros_cbi_get_fw_config(dev, FW_CONFIG_FIELD_1, &value);
	zassert_true(ret == 0,
		     "Expected no error return from cros_cbi_get_fw_config");
	zassert_true(value == FW_FIELD_1_A,
		     "Expected field value to match FW_FIELD_1_A");

	ret = cros_cbi_get_fw_config(dev, FW_CONFIG_FIELD_2, &value);
	zassert_true(ret == 0,
		     "Expected no error return from cros_cbi_get_fw_config");
	zassert_false(value == FW_FIELD_2_X,
		      "Expected field value to not match FW_FIELD_2_X");
}

void test_suite_cros_cbi(void)
{
	ztest_test_suite(cros_cbi,
			 ztest_unit_test(test_check_match),
			 ztest_unit_test(test_fail_check_match),
			 ztest_unit_test(test_fw_config));
	ztest_run_test_suite(cros_cbi);
}

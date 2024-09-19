/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/gpio.h>
#include <drivers/gpio/gpio_emul.h>

#include "battery.h"

#define GPIO_BATT_PRES_ODL_PATH DT_PATH(named_gpios, ec_batt_pres_odl)
#define GPIO_BATT_PRES_ODL_PORT DT_GPIO_PIN(GPIO_BATT_PRES_ODL_PATH, gpios)

static void test_battery_is_present_gpio(void)
{
	const struct device *dev =
		DEVICE_DT_GET(DT_GPIO_CTLR(GPIO_BATT_PRES_ODL_PATH, gpios));

	zassert_not_null(dev, NULL);
	/* ec_batt_pres_odl = 0 means battery present. */
	zassert_ok(gpio_emul_input_set(dev, GPIO_BATT_PRES_ODL_PORT, 0), NULL);
	zassert_equal(BP_YES, battery_is_present(), NULL);
	/* ec_batt_pres_odl = 1 means battery missing. */
	zassert_ok(gpio_emul_input_set(dev, GPIO_BATT_PRES_ODL_PORT, 1), NULL);
	zassert_equal(BP_NO, battery_is_present(), NULL);
}

void test_suite_battery(void)
{
	ztest_test_suite(battery,
			 ztest_user_unit_test(test_battery_is_present_gpio));
	ztest_run_test_suite(battery);
}

/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/gpio/gpio_emul.h>

#include "battery_smart.h"
#include "ec_commands.h"
#include "ec_tasks.h"
#include "emul/emul_smart_battery.h"
#include "emul/tcpc/emul_tcpci.h"
#include "emul/tcpc/emul_tcpci_partner_src.h"
#include "host_command.h"
#include "tcpm/tcpci.h"

#define TCPCI_EMUL_LABEL DT_NODELABEL(tcpci_emul)
#define BATTERY_ORD DT_DEP_ORD(DT_NODELABEL(battery))

#define GPIO_AC_OK_PATH DT_PATH(named_gpios, acok_od)
#define GPIO_AC_OK_PIN DT_GPIO_PIN(GPIO_AC_OK_PATH, gpios)

static void init_tcpm(void)
{
	const struct emul *tcpci_emul =
		emul_get_binding(DT_LABEL(TCPCI_EMUL_LABEL));
	struct i2c_emul *i2c_emul;
	struct sbat_emul_bat_data *bat;
	const struct device *gpio_dev =
		DEVICE_DT_GET(DT_GPIO_CTLR(GPIO_AC_OK_PATH, gpios));

	set_test_runner_tid();
	zassert_ok(tcpci_tcpm_init(0), 0);
	pd_set_suspend(0, 0);
	/* Reset to disconnected state. */
	zassert_ok(tcpci_emul_disconnect_partner(tcpci_emul), NULL);

	/* Battery defaults to charging, so reset to not charging. */
	i2c_emul = sbat_emul_get_ptr(BATTERY_ORD);
	bat = sbat_emul_get_bat_data(i2c_emul);
	bat->cur = -5;

	zassert_ok(gpio_emul_input_set(gpio_dev, GPIO_AC_OK_PIN, 0), NULL);
}

static void remove_emulated_devices(void)
{
	const struct emul *tcpci_emul =
		emul_get_binding(DT_LABEL(TCPCI_EMUL_LABEL));
	/* TODO: This function should trigger gpios to signal there is nothing
	 * attached to the port.
	 */
	zassert_ok(tcpci_emul_disconnect_partner(tcpci_emul), NULL);
}

static void test_attach_compliant_charger(void)
{
	const struct emul *tcpci_emul =
		emul_get_binding(DT_LABEL(TCPCI_EMUL_LABEL));
	struct i2c_emul *i2c_emul;
	uint16_t battery_status;
	struct tcpci_src_emul_data my_charger;
	const struct device *gpio_dev =
		DEVICE_DT_GET(DT_GPIO_CTLR(GPIO_AC_OK_PATH, gpios));

	/* Verify battery not charging. */
	i2c_emul = sbat_emul_get_ptr(BATTERY_ORD);
	zassert_ok(sbat_emul_get_word_val(i2c_emul, SB_BATTERY_STATUS,
					  &battery_status),
		   NULL);
	zassert_not_equal(battery_status & STATUS_DISCHARGING, 0,
			  "Battery is not discharging: %d", battery_status);

	/* TODO? Send host command to verify PD_ROLE_DISCONNECTED. */

	/* Attach emulated charger. */
	zassert_ok(gpio_emul_input_set(gpio_dev, GPIO_AC_OK_PIN, 1), NULL);
	tcpci_src_emul_init(&my_charger);
	zassert_ok(tcpci_src_emul_connect_to_tcpci(&my_charger, tcpci_emul),
		   NULL);

	/* Wait for current ramp. */
	k_sleep(K_SECONDS(10));

	/* Verify battery charging. */
	zassert_ok(sbat_emul_get_word_val(i2c_emul, SB_BATTERY_STATUS,
					  &battery_status),
		   NULL);
	zassert_equal(battery_status & STATUS_DISCHARGING, 0,
		      "Battery is discharging: %d", battery_status);
	/* TODO: Also check voltage, current, etc. */
}

static void test_attach_pd_charger(void)
{
	const struct emul *tcpci_emul =
		emul_get_binding(DT_LABEL(TCPCI_EMUL_LABEL));
	struct i2c_emul *i2c_emul;
	uint16_t battery_status;
	struct tcpci_src_emul_data my_charger;
	const struct device *gpio_dev =
		DEVICE_DT_GET(DT_GPIO_CTLR(GPIO_AC_OK_PATH, gpios));
	struct ec_params_charge_state charge_params;
	struct ec_response_charge_state charge_response;
	struct host_cmd_handler_args args = BUILD_HOST_COMMAND(
			EC_CMD_CHARGE_STATE, 0, charge_response, charge_params);

	/*
	 * TODO(b/209907297): Implement the steps of the test beyond USB default
	 * charging.
	 */

	/* 1. Configure source PDOs of partner (probably fixed source 5V 3A
	 * and fixed source 20V 3A). Currently, the partner emulator only
	 * supports the default USB power PDO.
	 */

	/* Attach emulated charger. This will send Source Capabilities. */
	zassert_ok(gpio_emul_input_set(gpio_dev, GPIO_AC_OK_PIN, 1), NULL);
	tcpci_src_emul_init(&my_charger);
	zassert_ok(tcpci_src_emul_connect_to_tcpci(&my_charger, tcpci_emul),
		   NULL);

	/* Wait for current ramp. */
	k_sleep(K_SECONDS(10));

	/* Verify battery charging. */
	i2c_emul = sbat_emul_get_ptr(BATTERY_ORD);
	zassert_ok(sbat_emul_get_word_val(i2c_emul, SB_BATTERY_STATUS,
					  &battery_status),
		   NULL);
	zassert_equal(battery_status & STATUS_DISCHARGING, 0,
		      "Battery is discharging: %d", battery_status);

	/*
	 * 2. Check charging current and voltage (should be 5V, default USB
	 * current); make sure that reports from battery and PD host commands
	 * match; check that host command reports no active PDO.
	 */
	/*
	 * TODO(b/209907297): Also check the corresponding PD state and
	 * encapsulate this for use in other tests.
	 */
	charge_params.chgnum = 0;
	charge_params.cmd = CHARGE_STATE_CMD_GET_STATE;
	zassert_ok(host_command_process(&args), "Failed to get charge state");
	zassert_true(charge_response.get_state.ac, "USB default but AC absent");
	zassert_equal(charge_response.get_state.chg_voltage, 5000,
			"USB default voltage %dmV",
			charge_response.get_state.chg_voltage);
	zassert_true(charge_response.get_state.chg_current > 0,
			"USB default current %dmA",
			charge_response.get_state.chg_current);

	/*
	 * 3. Wait for SenderResponseTimeout. Expect TCPM to send Request.
	 * We could verify that the Request references the expected PDO, but
	 * the voltage/current/PDO checks at the end of the test should all be
	 * wrong if the requested PDO was wrong here.
	 */

	/*
	 * 4. Send Accept and PS_RDY from partner with appropriate delay between
	 * them. Emulate supplying VBUS at the requested voltage/current before
	 * PS_RDY.
	 */

	/*
	 * 5. Check the charging voltage and current. Cross-check the PD state,
	 * the battery/charger state, and the active PDO as reported by the PD
	 * state.
	 */
}

void test_suite_integration_usb(void)
{
	ztest_test_suite(integration_usb,
			 ztest_user_unit_test_setup_teardown(
				 test_attach_compliant_charger, init_tcpm,
				 remove_emulated_devices),
			 ztest_user_unit_test_setup_teardown(
				 test_attach_pd_charger, init_tcpm,
				 remove_emulated_devices));
	ztest_run_test_suite(integration_usb);
}

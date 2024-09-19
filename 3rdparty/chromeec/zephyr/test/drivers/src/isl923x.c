/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ztest.h>
#include <drivers/emul.h>
#include <fff.h>

#include "battery.h"
#include "battery_smart.h"
#include "charger_utils.h"
#include "driver/charger/isl923x.h"
#include "driver/charger/isl923x_public.h"
#include "emul/emul_common_i2c.h"
#include "emul/emul_isl923x.h"
#include "system.h"
#include "test_mocks.h"

BUILD_ASSERT(CONFIG_CHARGER_SENSE_RESISTOR == 10 ||
	     CONFIG_CHARGER_SENSE_RESISTOR == 5);

BUILD_ASSERT(CONFIG_CHARGER_SENSE_RESISTOR_AC == 20 ||
	     CONFIG_CHARGER_SENSE_RESISTOR_AC == 10);

BUILD_ASSERT(IS_ENABLED(CONFIG_CHARGER_ISL9238),
	     "Must test on ISL9238; ISL9237, ISL9238c, and RAA489000 are not"
	     " yet supported");

#if CONFIG_CHARGER_SENSE_RESISTOR == 10
#define EXPECTED_CURRENT_MA(n) (n)
#define EXPECTED_CURRENT_REG(n) (n)
#else
#define EXPECTED_CURRENT_MA(n) (n * 2)
#define EXPECTED_CURRENT_REG(n) (n / 2)
#endif

#if CONFIG_CHARGER_SENSE_RESISTOR_AC == 20
#define EXPECTED_INPUT_CURRENT_MA(n) (n)
#define EXPECTED_INPUT_CURRENT_REG(n) (n)
#else
#define EXPECTED_INPUT_CURRENT_MA(n) (n * 2)
#define EXPECTED_INPUT_CURRENT_REG(n) (n / 2)
#endif

#define CHARGER_NUM get_charger_num(&isl923x_drv)
#define ISL923X_EMUL emul_get_binding(DT_LABEL(DT_NODELABEL(isl923x_emul)))

static int mock_write_fn_always_fail(struct i2c_emul *emul, int reg,
				     uint8_t val, int bytes, void *data)
{
	ztest_test_fail();
	return 0;
}

static void test_isl923x_set_current(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int expected_current_milli_amps[] = {
		EXPECTED_CURRENT_MA(0),	   EXPECTED_CURRENT_MA(4),
		EXPECTED_CURRENT_MA(8),	   EXPECTED_CURRENT_MA(16),
		EXPECTED_CURRENT_MA(32),   EXPECTED_CURRENT_MA(64),
		EXPECTED_CURRENT_MA(128),  EXPECTED_CURRENT_MA(256),
		EXPECTED_CURRENT_MA(512),  EXPECTED_CURRENT_MA(1024),
		EXPECTED_CURRENT_MA(2048), EXPECTED_CURRENT_MA(4096)
	};
	int current_milli_amps;

	/* Test I2C failure when reading charge current */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CHG_CURRENT);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.get_current(CHARGER_NUM, &current_milli_amps),
		      NULL);

	/* Reset fail register */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	for (int i = 0; i < ARRAY_SIZE(expected_current_milli_amps); ++i) {
		zassert_ok(isl923x_drv.set_current(
				   CHARGER_NUM, expected_current_milli_amps[i]),
			   "Failed to set the current to %dmA",
			   expected_current_milli_amps[i]);
		zassert_ok(isl923x_drv.get_current(CHARGER_NUM,
						   &current_milli_amps),
			   "Failed to get current");
		zassert_equal(expected_current_milli_amps[i],
			      current_milli_amps,
			      "Expected current %dmA but got %dmA",
			      expected_current_milli_amps[i],
			      current_milli_amps);
	}
}

static void test_isl923x_set_voltage(void)
{
	int expected_voltage_milli_volts[] = { 8,    16,   32,	 64,
					       128,  256,  512,	 1024,
					       2048, 4096, 8192, 16384 };
	int voltage_milli_volts;

	/* Test 0mV first, it's a special case because of voltage_min */
	zassert_ok(isl923x_drv.set_voltage(CHARGER_NUM, 0),
		   "Failed to set the voltage to 0mV");
	zassert_ok(isl923x_drv.get_voltage(CHARGER_NUM, &voltage_milli_volts),
		   "Failed to get voltage");
	zassert_equal(battery_get_info()->voltage_min, voltage_milli_volts,
		      "Expected voltage %dmV but got %dmV",
		      battery_get_info()->voltage_min, voltage_milli_volts);

	for (int i = 0; i < ARRAY_SIZE(expected_voltage_milli_volts); ++i) {
		zassert_ok(isl923x_drv.set_voltage(
				   CHARGER_NUM,
				   expected_voltage_milli_volts[i]),
			   "Failed to set the voltage to %dmV",
			   expected_voltage_milli_volts[i]);
		zassert_ok(isl923x_drv.get_voltage(CHARGER_NUM,
						   &voltage_milli_volts),
			   "Failed to get voltage");
		zassert_equal(expected_voltage_milli_volts[i],
			      voltage_milli_volts,
			      "Expected voltage %dmV but got %dmV",
			      expected_voltage_milli_volts[i],
			      voltage_milli_volts);
	}
}

static void test_isl923x_set_input_current_limit(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int expected_current_milli_amps[] = {
		EXPECTED_INPUT_CURRENT_MA(0),
		EXPECTED_INPUT_CURRENT_MA(4),
		EXPECTED_INPUT_CURRENT_MA(8),
		EXPECTED_INPUT_CURRENT_MA(16),
		EXPECTED_INPUT_CURRENT_MA(32),
		EXPECTED_INPUT_CURRENT_MA(64),
		EXPECTED_INPUT_CURRENT_MA(128),
		EXPECTED_INPUT_CURRENT_MA(256),
		EXPECTED_INPUT_CURRENT_MA(512),
		EXPECTED_INPUT_CURRENT_MA(1024),
		EXPECTED_INPUT_CURRENT_MA(2048),
		EXPECTED_INPUT_CURRENT_MA(4096) };
	int current_milli_amps;

	/* Test failing to write to current limit 1 reg */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   ISL923X_REG_ADAPTER_CURRENT_LIMIT1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.set_input_current_limit(CHARGER_NUM, 0),
		      NULL);

	/* Test failing to write to current limit 2 reg */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   ISL923X_REG_ADAPTER_CURRENT_LIMIT2);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.set_input_current_limit(CHARGER_NUM, 0),
		      NULL);

	/* Reset fail register */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test failing to read current limit 1 reg */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  ISL923X_REG_ADAPTER_CURRENT_LIMIT1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.get_input_current_limit(CHARGER_NUM,
							  &current_milli_amps),
		      NULL);

	/* Reset fail register */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test normal code path */
	for (int i = 0; i < ARRAY_SIZE(expected_current_milli_amps); ++i) {
		zassert_ok(isl923x_drv.set_input_current_limit(
				   CHARGER_NUM, expected_current_milli_amps[i]),
			   "Failed to set input current limit to %dmV",
			   expected_current_milli_amps[i]);
		zassert_ok(isl923x_drv.get_input_current_limit(
				   CHARGER_NUM, &current_milli_amps),
			   "Failed to get input current limit");
		zassert_equal(expected_current_milli_amps[i],
			      current_milli_amps,
			      "Expected input current %dmA but got %dmA",
			      expected_current_milli_amps[i],
			      current_milli_amps);
	}
}

static void test_manufacturer_id(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int id;

	isl923x_emul_set_manufacturer_id(isl923x_emul, 0x1234);
	zassert_ok(isl923x_drv.manufacturer_id(CHARGER_NUM, &id), NULL);
	zassert_equal(0x1234, id, NULL);

	/* Test read error */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  ISL923X_REG_MANUFACTURER_ID);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.manufacturer_id(CHARGER_NUM, &id), NULL);

	/* Reset fail register */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
}

static void test_device_id(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int id;

	isl923x_emul_set_device_id(isl923x_emul, 0x5678);
	zassert_ok(isl923x_drv.device_id(CHARGER_NUM, &id), NULL);
	zassert_equal(0x5678, id, NULL);

	/* Test read error */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  ISL923X_REG_DEVICE_ID);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.device_id(CHARGER_NUM, &id), NULL);

	/* Reset fail register */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
}

static void test_options(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint32_t option;

	/* Test failed control 0 read */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL0);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.get_option(CHARGER_NUM, &option), NULL);

	/* Test failed control 1 read */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.get_option(CHARGER_NUM, &option), NULL);

	/* Reset failed read */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test failed control 0 write */
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL0);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.set_option(CHARGER_NUM, option), NULL);

	/* Test failed control 1 write */
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.set_option(CHARGER_NUM, option), NULL);

	/* Reset failed write */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test normal write/read, note that bits 23 and 0 are always 0 */
	zassert_ok(isl923x_drv.set_option(CHARGER_NUM, 0xffffffff), NULL);
	zassert_ok(isl923x_drv.get_option(CHARGER_NUM, &option), NULL);
	zassert_equal(0xff7ffffe, option,
		      "Expected options 0xff7ffffe but got 0x%x", option);
}

static void test_get_info(void)
{
	const struct charger_info *info = isl923x_drv.get_info(CHARGER_NUM);

	zassert_ok(strcmp("isl9238", info->name), NULL);
	zassert_equal(ISL9238_SYS_VOLTAGE_REG_MAX, info->voltage_max, NULL);
	zassert_equal(ISL923X_SYS_VOLTAGE_REG_MIN, info->voltage_min, NULL);
	zassert_equal(8, info->voltage_step, NULL);
	zassert_equal(EXPECTED_CURRENT_MA(6080), info->current_max, NULL);
	zassert_equal(EXPECTED_CURRENT_MA(4), info->current_min, NULL);
	zassert_equal(EXPECTED_CURRENT_MA(4), info->current_step, NULL);
	zassert_equal(EXPECTED_INPUT_CURRENT_MA(6080), info->input_current_max,
		      NULL);
	zassert_equal(EXPECTED_INPUT_CURRENT_MA(4), info->input_current_min,
		      NULL);
	zassert_equal(EXPECTED_INPUT_CURRENT_MA(4), info->input_current_step,
		      NULL);
}

static void test_status(void)
{
	int status;

	zassert_ok(isl923x_drv.get_status(CHARGER_NUM, &status), NULL);
	zassert_equal(CHARGER_LEVEL_2, status, NULL);
}

static void test_set_mode(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;

	/* Enable learn mode and set mode (actual value doesn't matter) */
	zassert_ok(isl923x_drv.discharge_on_ac(CHARGER_NUM, true), NULL);
	zassert_ok(isl923x_drv.set_mode(CHARGER_NUM, 0), NULL);
	/* Learn mode should still be set */
	zassert_true(isl923x_emul_is_learn_mode_enabled(isl923x_emul), NULL);

	/* Disable learn mode, but keep the bits */
	zassert_ok(isl923x_drv.discharge_on_ac(CHARGER_NUM, false), NULL);
	isl923x_emul_set_learn_mode_enabled(isl923x_emul, true);
	zassert_ok(isl923x_drv.set_mode(CHARGER_NUM, 0), NULL);
	/* Learn mode should still be off */
	zassert_true(!isl923x_emul_is_learn_mode_enabled(isl923x_emul), NULL);
}

static void test_post_init(void)
{
	zassert_ok(isl923x_drv.post_init(CHARGER_NUM), NULL);
}

static void test_set_ac_prochot(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	const struct device *i2c_dev = isl923x_emul_get_parent(isl923x_emul);
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t expected_current_milli_amps[] = {
		EXPECTED_INPUT_CURRENT_MA(0),
		EXPECTED_INPUT_CURRENT_MA(128),
		EXPECTED_INPUT_CURRENT_MA(256),
		EXPECTED_INPUT_CURRENT_MA(512),
		EXPECTED_INPUT_CURRENT_MA(1024),
		EXPECTED_INPUT_CURRENT_MA(2048),
		EXPECTED_INPUT_CURRENT_MA(4096)
	};
	uint16_t current_milli_amps;

	/* Test can't set current above max */
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_set_ac_prochot(
			      CHARGER_NUM, ISL923X_AC_PROCHOT_CURRENT_MAX + 1),
		      NULL);

	/* Test failed I2C write to prochot register */
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_PROCHOT_AC);
	zassert_equal(EC_ERROR_INVAL, isl923x_set_ac_prochot(CHARGER_NUM, 0),
		      NULL);

	/* Clear write fail reg */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	for (int i = 0; i < ARRAY_SIZE(expected_current_milli_amps); ++i) {
		uint8_t reg_addr = ISL923X_REG_PROCHOT_AC;

		/*
		 * Due to resistor multiplying the current, the upper end of the
		 * test data might be out of bounds (which is already tested
		 * above). Skip the test.
		 */
		if (expected_current_milli_amps[i] >
		    ISL923X_AC_PROCHOT_CURRENT_MAX) {
			continue;
		}

		zassert_ok(isl923x_set_ac_prochot(
				   CHARGER_NUM, expected_current_milli_amps[i]),
			   "Failed to set AC prochot to %dmA",
			   expected_current_milli_amps[i]);
		zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
					  sizeof(reg_addr), &current_milli_amps,
					  sizeof(current_milli_amps)),
			   "Failed to read AC prochot register");
		zassert_equal(EXPECTED_INPUT_CURRENT_REG(
				      expected_current_milli_amps[i]),
			      current_milli_amps,
			      "AC prochot expected %dmA but got %dmA",
			      EXPECTED_INPUT_CURRENT_REG(
				      expected_current_milli_amps[i]),
			      current_milli_amps);
	}
}
static void test_set_dc_prochot(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	const struct device *i2c_dev = isl923x_emul_get_parent(isl923x_emul);
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t expected_current_milli_amps[] = {
		EXPECTED_CURRENT_MA(256),  EXPECTED_CURRENT_MA(512),
		EXPECTED_CURRENT_MA(1024), EXPECTED_CURRENT_MA(2048),
		EXPECTED_CURRENT_MA(4096), EXPECTED_CURRENT_MA(8192)
	};
	uint16_t current_milli_amps;

	/* Test can't set current above max */
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_set_dc_prochot(
			      CHARGER_NUM, ISL923X_DC_PROCHOT_CURRENT_MAX + 1),
		      NULL);

	/* Test failed I2C write to prochot register */
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_PROCHOT_DC);
	zassert_equal(EC_ERROR_INVAL, isl923x_set_dc_prochot(CHARGER_NUM, 0),
		      NULL);

	/* Clear write fail reg */
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	for (int i = 0; i < ARRAY_SIZE(expected_current_milli_amps); ++i) {
		uint8_t reg_addr = ISL923X_REG_PROCHOT_DC;

		/*
		 * Due to resistor multiplying the current, the upper end of the
		 * test data might be out of bounds (which is already tested
		 * above). Skip the test.
		 */
		if (expected_current_milli_amps[i] >
		    ISL923X_DC_PROCHOT_CURRENT_MAX) {
			continue;
		}
		zassert_ok(isl923x_set_dc_prochot(
				   CHARGER_NUM, expected_current_milli_amps[i]),
			   "Failed to set DC prochot to %dmA",
			   expected_current_milli_amps[i]);
		zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
					  sizeof(reg_addr), &current_milli_amps,
					  sizeof(current_milli_amps)),
			   "Failed to read DC prochot register");
		zassert_equal(
			EXPECTED_CURRENT_REG(expected_current_milli_amps[i]),
			current_milli_amps,
			"AC prochot expected %dmA but got %dmA",
			EXPECTED_CURRENT_REG(expected_current_milli_amps[i]),
			current_milli_amps);
	}
}

static void test_comparator_inversion(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	const struct device *i2c_dev = isl923x_emul_get_parent(isl923x_emul);
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint8_t reg_addr = ISL923X_REG_CONTROL2;
	uint16_t reg_value;
	uint8_t tx_buf[] = { reg_addr, 0, 0 };

	/* Test failed read, should not write */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL2);
	i2c_common_emul_set_write_func(i2c_emul, mock_write_fn_always_fail,
				       NULL);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_set_comparator_inversion(CHARGER_NUM, false),
		      NULL);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	i2c_common_emul_set_write_func(i2c_emul, NULL, NULL);

	/* Test failed write */
	zassert_ok(i2c_write(i2c_dev, tx_buf, sizeof(tx_buf), i2c_emul->addr),
		   "Failed to clear CTRL2 register");
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL2);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_set_comparator_inversion(CHARGER_NUM, true),
		      NULL);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test enable comparator inversion */
	zassert_ok(isl923x_set_comparator_inversion(CHARGER_NUM, true), NULL);
	zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
				  sizeof(reg_addr), &reg_value,
				  sizeof(reg_value)),
		   "Failed to read CTRL 2 register");
	zassert_true((reg_value & ISL923X_C2_INVERT_CMOUT) != 0, NULL);

	/* Test disable comparator inversion */
	zassert_ok(isl923x_set_comparator_inversion(CHARGER_NUM, false), NULL);
	zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
				  sizeof(reg_addr), &reg_value,
				  sizeof(reg_value)),
		   "Failed to read CTRL 2 register");
	zassert_true((reg_value & ISL923X_C2_INVERT_CMOUT) == 0, NULL);
}

static void test_discharge_on_ac(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	const struct device *i2c_dev = isl923x_emul_get_parent(isl923x_emul);
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint8_t reg_addr = ISL923X_REG_CONTROL1;
	uint8_t tx_buf[] = { reg_addr, 0, 0 };
	uint16_t reg_value;

	/* Test failure to read CTRL1 register */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.discharge_on_ac(CHARGER_NUM, true), NULL);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Set CTRL1 register to 0 */
	zassert_ok(i2c_write(i2c_dev, tx_buf, sizeof(tx_buf), i2c_emul->addr),
		   NULL);

	/* Test failure to write CTRL1 register */
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL1);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.discharge_on_ac(CHARGER_NUM, true), NULL);
	zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
				  sizeof(reg_addr), &reg_value,
				  sizeof(reg_value)),
		   NULL);
	zassert_equal(0, reg_value, NULL);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Test enabling discharge on AC */
	zassert_ok(isl923x_drv.discharge_on_ac(CHARGER_NUM, true), NULL);

	zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
				  sizeof(reg_addr), &reg_value,
				  sizeof(reg_value)),
		   NULL);
	zassert_true((reg_value & ISL923X_C1_LEARN_MODE_ENABLE) != 0, NULL);

	/* Test disabling discharge on AC */
	zassert_ok(isl923x_drv.discharge_on_ac(CHARGER_NUM, false), NULL);

	zassert_ok(i2c_write_read(i2c_dev, i2c_emul->addr, &reg_addr,
				  sizeof(reg_addr), &reg_value,
				  sizeof(reg_value)),
		   NULL);
	zassert_true((reg_value & ISL923X_C1_LEARN_MODE_ENABLE) == 0, NULL);
}

static void test_get_vbus_voltage(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int reg_values[] = { BIT(6),  BIT(7),  BIT(8),	BIT(9),
			     BIT(10), BIT(11), BIT(12), BIT(13) };
	int voltage;

	/* Test fail to read the ADC vbus register */
	i2c_common_emul_set_read_fail_reg(i2c_emul, RAA489000_REG_ADC_VBUS);
	zassert_equal(EC_ERROR_INVAL,
		      isl923x_drv.get_vbus_voltage(CHARGER_NUM, 0, &voltage),
		      NULL);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	for (int i = 0; i < ARRAY_SIZE(reg_values); ++i) {
		int expected_voltage = (reg_values[i] >> 6) * 96;

		isl923x_emul_set_adc_vbus(isl923x_emul, reg_values[i]);
		zassert_ok(isl923x_drv.get_vbus_voltage(CHARGER_NUM, 0,
							&voltage),
			   NULL);
		zassert_equal(expected_voltage, voltage,
			      "Expected %dmV but got %dmV", expected_voltage,
			      voltage);
	}
}

static void test_init(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int input_current;

	/* Test failed CTRL2 register read (prochot debounce) */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL2);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed CTRL2 register write */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL2);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed CTRL 0 read */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL0);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed CTRL 0 write */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL923X_REG_CONTROL0);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed CTRL 3 read */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL9238_REG_CONTROL3);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed CTRL 3 write */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_write_fail_reg(i2c_emul, ISL9238_REG_CONTROL3);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/* Test failed write adapter current limit */
	isl923x_emul_reset(isl923x_emul);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   ISL923X_REG_ADAPTER_CURRENT_LIMIT1);
	isl923x_drv.init(CHARGER_NUM);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
	zassert_ok(isl923x_drv.get_input_current_limit(CHARGER_NUM,
						       &input_current),
		   NULL);
	zassert_equal(0, input_current,
		      "Expected input current 0mV but got %dmV", input_current);

	/*
	 * Test system_jumped_late being true (will not call
	 * set_input_current_limit)
	 */
	system_jumped_late_mock.ret_val = true;
	system_jumped_late_mock.call_count = 0;
	isl923x_emul_reset(isl923x_emul);
	isl923x_drv.init(CHARGER_NUM);
	zassert_equal(
		1, system_jumped_late_mock.call_count,
		"Expected to have called system_jumped_late() once, but got %d calls",
		system_jumped_late_mock.call_count);
	system_jumped_late_mock.ret_val = false;
}

static void test_isl923x_is_acok(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	enum ec_error_list rv;
	bool acok;

	/* Part 1: invalid charger number */
	rv = raa489000_is_acok(board_get_charger_chip_count() + 1, &acok);
	zassert_equal(EC_ERROR_INVAL, rv,
		      "Invalid charger num, but AC OK check succeeded");

	/* Part 2: error accessing register */
	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL9238_REG_INFO2);

	rv = raa489000_is_acok(CHARGER_NUM, &acok);
	zassert_equal(EC_ERROR_INVAL, rv,
		      "Register read failure, but AC OK check succeeded");

	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);

	/* Part 3: successful path - ACOK is true */
	raa489000_emul_set_acok_pin(isl923x_emul, 1);

	rv = raa489000_is_acok(CHARGER_NUM, &acok);
	zassert_equal(EC_SUCCESS, rv, "AC OK check did not return success");
	zassert_true(acok, "AC OK is false");

	/* Part 3: successful path - ACOK is false */
	raa489000_emul_set_acok_pin(isl923x_emul, 0);

	rv = raa489000_is_acok(CHARGER_NUM, &acok);
	zassert_equal(EC_SUCCESS, rv, "AC OK check did not return success");
	zassert_false(acok, "AC OK is true");
}

static void test_isl923x_enable_asgate(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	int rv;

	/* Part 1: Try enabling the ASGATE */
	rv = raa489000_enable_asgate(CHARGER_NUM, true);

	zassert_equal(EC_SUCCESS, rv, "Expected return code of %d but got %d",
		      EC_SUCCESS, rv);
	zassert_true(
		isl923x_emul_peek_reg(i2c_emul, RAA489000_REG_CONTROL8) &
			RAA489000_C8_ASGATE_ON_READY,
		"RAA489000_C8_ASGATE_ON_READY bit not set in Control Reg 8");

	/* Part 2: Turn it back off */
	rv = raa489000_enable_asgate(CHARGER_NUM, false);

	zassert_equal(EC_SUCCESS, rv, "Expected return code of %d but got %d",
		      EC_SUCCESS, rv);
	zassert_false(isl923x_emul_peek_reg(i2c_emul, RAA489000_REG_CONTROL8) &
			      RAA489000_C8_ASGATE_ON_READY,
		      "RAA489000_C8_ASGATE_ON_READY bit set in Control Reg 8");
}

/* Mock read and write functions to use in the hibernation test */
FAKE_VALUE_FUNC(int, hibernate_mock_read_fn, struct i2c_emul *, int, uint8_t *,
		int, void *);
FAKE_VALUE_FUNC(int, hibernate_mock_write_fn, struct i2c_emul *, int, uint8_t,
		int, void *);

/**
 * @brief Setup function for the hibernate tests.
 */
static void hibernate_test_setup(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	/* Reset mocks and make the read/write mocks pass all data through */
	RESET_FAKE(hibernate_mock_read_fn);
	RESET_FAKE(hibernate_mock_write_fn);
	hibernate_mock_read_fn_fake.return_val = 1;
	hibernate_mock_write_fn_fake.return_val = 1;

	i2c_common_emul_set_read_func(i2c_emul, hibernate_mock_read_fn, NULL);
	i2c_common_emul_set_write_func(i2c_emul, hibernate_mock_write_fn, NULL);

	/* Don't fail on any register access */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
}

/**
 * @brief Teardown function for the hibernate tests.
 */
static void hibernate_test_teardown(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	/* Clear the mock read/write functions */
	i2c_common_emul_set_read_func(i2c_emul, NULL, NULL);
	i2c_common_emul_set_write_func(i2c_emul, NULL, NULL);

	/* Don't fail on any register access */
	i2c_common_emul_set_read_fail_reg(i2c_emul,
					  I2C_COMMON_EMUL_NO_FAIL_REG);
	i2c_common_emul_set_write_fail_reg(i2c_emul,
					   I2C_COMMON_EMUL_NO_FAIL_REG);
}

static void test_isl923x_hibernate__happy_path(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t actual;

	raa489000_hibernate(CHARGER_NUM, false);

	/* Check ISL923X_REG_CONTROL0 */
	actual = isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL0);

	zassert_false(actual & RAA489000_C0_EN_CHG_PUMPS_TO_100PCT,
		      "RAA489000_C0_EN_CHG_PUMPS_TO_100PCT should not be set");
	zassert_false(actual & RAA489000_C0_BGATE_FORCE_ON,
		      "RAA489000_C0_BGATE_FORCE_ON should not be set");

	/* Check ISL923X_REG_CONTROL1 */
	actual = isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1);

	zassert_false(actual & RAA489000_C1_ENABLE_SUPP_SUPPORT_MODE,
		      "RAA489000_C1_ENABLE_SUPP_SUPPORT_MODE should not be set");
	zassert_false(actual & ISL923X_C1_ENABLE_PSYS,
		      "ISL923X_C1_ENABLE_PSYS should not be set");
	zassert_true(actual & RAA489000_C1_BGATE_FORCE_OFF,
		     "RAA489000_C1_BGATE_FORCE_OFF should be set");
	zassert_true(actual & ISL923X_C1_DISABLE_MON,
		     "ISL923X_C1_DISABLE_MON should be set");

	/* Check ISL9238_REG_CONTROL3 (disable_adc = false) */
	actual = isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3);

	zassert_true(actual & RAA489000_ENABLE_ADC,
		     "RAA489000_ENABLE_ADC should be set");

	/* Check ISL9238_REG_CONTROL4 */
	actual = isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL4);

	zassert_true(actual & RAA489000_C4_DISABLE_GP_CMP,
		     "RAA489000_C4_DISABLE_GP_CMP should be set");

	/* Ensure all expected register reads and writes happened */
	int registers[] = { ISL923X_REG_CONTROL0, ISL923X_REG_CONTROL1,
			    ISL9238_REG_CONTROL3, ISL9238_REG_CONTROL4 };

	for (int i = 0; i < ARRAY_SIZE(registers); i++) {
		/* Each reg has 2 reads and 2 writes because they are 16-bit */
		MOCK_ASSERT_I2C_READ(hibernate_mock_read_fn, i * 2,
				     registers[i]);
		MOCK_ASSERT_I2C_READ(hibernate_mock_read_fn, (i * 2) + 1,
				     registers[i]);
		MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, i * 2,
				      registers[i], MOCK_IGNORE_VALUE);
		MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, (i * 2) + 1,
				      registers[i], MOCK_IGNORE_VALUE);
	}
}

static void test_isl923x_hibernate__invalid_charger_number(void)
{
	/* Mocks should just be pass-through */
	RESET_FAKE(hibernate_mock_read_fn);
	RESET_FAKE(hibernate_mock_write_fn);
	hibernate_mock_read_fn_fake.return_val = 1;
	hibernate_mock_write_fn_fake.return_val = 1;

	raa489000_hibernate(board_get_charger_chip_count() + 1, false);

	/* Make sure no I2C activity happened */
	zassert_equal(hibernate_mock_read_fn_fake.call_count, 0,
		      "No I2C reads should have happened");
	zassert_equal(hibernate_mock_write_fn_fake.call_count, 0,
		      "No I2C writes should have happened");
}

static void test_isl923x_hibernate__fail_at_ISL923X_REG_CONTROL0(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL0);

	raa489000_hibernate(CHARGER_NUM, false);

	/*
	 * We have no return codes to check, so instead verify that the first
	 * successful I2C write is to CONTROL1 and not CONTROL0.
	 */

	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 0, ISL923X_REG_CONTROL1,
			      MOCK_IGNORE_VALUE);
}

static void test_isl923x_hibernate__fail_at_ISL923X_REG_CONTROL1(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL923X_REG_CONTROL1);

	raa489000_hibernate(CHARGER_NUM, false);

	/*
	 * Ensure we skipped CONTROL1. (NB: due to 16-bit regs, each write takes
	 * two calls to the mock_write_fn)
	 */

	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 0, ISL923X_REG_CONTROL0,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 1, ISL923X_REG_CONTROL0,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 2, ISL9238_REG_CONTROL3,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 3, ISL9238_REG_CONTROL3,
			      MOCK_IGNORE_VALUE);
}

static void test_isl923x_hibernate__fail_at_ISL9238_REG_CONTROL3(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL9238_REG_CONTROL3);

	raa489000_hibernate(CHARGER_NUM, false);

	/*
	 * Ensure we skipped CONTROL3. (NB: due to 16-bit regs, each write takes
	 * two calls to the mock_write_fn)
	 */

	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 2, ISL923X_REG_CONTROL1,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 3, ISL923X_REG_CONTROL1,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 4, ISL9238_REG_CONTROL4,
			      MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 5, ISL9238_REG_CONTROL4,
			      MOCK_IGNORE_VALUE);
}

static void test_isl923x_hibernate__fail_at_ISL9238_REG_CONTROL4(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);

	i2c_common_emul_set_read_fail_reg(i2c_emul, ISL9238_REG_CONTROL4);

	raa489000_hibernate(CHARGER_NUM, false);

	/*
	 * Ensure we skipped CONTROL4. (i.e. the last calls should be to write
	 * to CONTROL3)
	 */
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn,
			      hibernate_mock_write_fn_fake.call_count - 2,
			      ISL9238_REG_CONTROL3, MOCK_IGNORE_VALUE);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn,
			      hibernate_mock_write_fn_fake.call_count - 1,
			      ISL9238_REG_CONTROL3, MOCK_IGNORE_VALUE);
}

static void test_isl923x_hibernate__adc_disable(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t expected;

	raa489000_hibernate(CHARGER_NUM, true);

	/* Check ISL9238_REG_CONTROL3 (disable_adc = true) */
	expected = isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3);
	expected &= ~RAA489000_ENABLE_ADC;

	MOCK_ASSERT_I2C_READ(hibernate_mock_read_fn, 4, ISL9238_REG_CONTROL3);
	MOCK_ASSERT_I2C_READ(hibernate_mock_read_fn, 5, ISL9238_REG_CONTROL3);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 4, ISL9238_REG_CONTROL3,
			      expected & 0xff);
	MOCK_ASSERT_I2C_WRITE(hibernate_mock_write_fn, 5, ISL9238_REG_CONTROL3,
			      expected >> 8);
}

static void test_isl9238c_hibernate(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t control1_expected, control2_expected, control3_expected;
	int rv;

	/* Part 1: Happy path */
	control1_expected =
		(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1) &
			~ISL923X_C1_ENABLE_PSYS) |
		ISL923X_C1_DISABLE_MON;
	control2_expected =
		isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2) |
		ISL923X_C2_COMPARATOR;
	control3_expected =
		isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3) |
		ISL9238_C3_BGATE_OFF;

	rv = isl9238c_hibernate(CHARGER_NUM);

	zassert_equal(EC_SUCCESS, rv, "Expected return code %d but got %d",
		      EC_SUCCESS, rv);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1),
		      control1_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1),
		      control1_expected);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2),
		      control2_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2),
		      control2_expected);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3),
		      control3_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3),
		      control3_expected);

	/* Part 2: Fail reading each register and check for error code */
	int registers[] = { ISL923X_REG_CONTROL1, ISL923X_REG_CONTROL2,
			    ISL9238_REG_CONTROL3 };

	for (int i = 0; i < ARRAY_SIZE(registers); i++) {
		i2c_common_emul_set_read_fail_reg(i2c_emul, registers[i]);

		rv = isl9238c_hibernate(CHARGER_NUM);

		zassert_equal(EC_ERROR_INVAL, rv,
			      "Wrong return code. Expected %d but got %d",
			      EC_ERROR_INVAL, rv);
	}
}

static void test_isl9238c_resume(void)
{
	const struct emul *isl923x_emul = ISL923X_EMUL;
	struct i2c_emul *i2c_emul = isl923x_emul_get_i2c_emul(isl923x_emul);
	uint16_t control1_expected, control2_expected, control3_expected;
	int rv;

	/* Part 1: Happy path */
	control1_expected =
		(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1) &
			~ISL923X_C1_DISABLE_MON) | ISL923X_C1_ENABLE_PSYS
		;
	control2_expected =
		isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2) &
		~ISL923X_C2_COMPARATOR;
	control3_expected =
		isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3) &
		~ISL9238_C3_BGATE_OFF;

	rv = isl9238c_resume(CHARGER_NUM);

	zassert_equal(EC_SUCCESS, rv, "Expected return code %d but got %d",
		      EC_SUCCESS, rv);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1),
		      control1_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL1),
		      control1_expected);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2),
		      control2_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL923X_REG_CONTROL2),
		      control2_expected);
	zassert_equal(isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3),
		      control3_expected,
		      "Unexpected register value 0x%02x. Should be 0x%02x",
		      isl923x_emul_peek_reg(i2c_emul, ISL9238_REG_CONTROL3),
		      control3_expected);

	/* Part 2: Fail reading each register and check for error code */
	int registers[] = { ISL923X_REG_CONTROL1, ISL923X_REG_CONTROL2,
			    ISL9238_REG_CONTROL3 };

	for (int i = 0; i < ARRAY_SIZE(registers); i++) {
		i2c_common_emul_set_read_fail_reg(i2c_emul, registers[i]);

		rv = isl9238c_resume(CHARGER_NUM);

		zassert_equal(EC_ERROR_INVAL, rv,
			      "Wrong return code. Expected %d but got %d",
			      EC_ERROR_INVAL, rv);
	}
}

void test_suite_isl923x(void)
{
	ztest_test_suite(
		isl923x, ztest_unit_test(test_isl923x_set_current),
		ztest_unit_test(test_isl923x_set_voltage),
		ztest_unit_test(test_isl923x_set_input_current_limit),
		ztest_unit_test(test_manufacturer_id),
		ztest_unit_test(test_device_id), ztest_unit_test(test_options),
		ztest_unit_test(test_get_info), ztest_unit_test(test_status),
		ztest_unit_test(test_set_mode), ztest_unit_test(test_post_init),
		ztest_unit_test(test_set_ac_prochot),
		ztest_unit_test(test_set_dc_prochot),
		ztest_unit_test(test_comparator_inversion),
		ztest_unit_test(test_discharge_on_ac),
		ztest_unit_test(test_get_vbus_voltage),
		ztest_unit_test(test_init),
		ztest_unit_test(test_isl923x_is_acok),
		ztest_unit_test(test_isl923x_enable_asgate),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__happy_path,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__invalid_charger_number,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__fail_at_ISL923X_REG_CONTROL0,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__fail_at_ISL923X_REG_CONTROL1,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__fail_at_ISL9238_REG_CONTROL3,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__fail_at_ISL9238_REG_CONTROL4,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(
			test_isl923x_hibernate__adc_disable,
			hibernate_test_setup, hibernate_test_teardown),
		ztest_unit_test_setup_teardown(test_isl9238c_hibernate,
					       hibernate_test_teardown,
					       hibernate_test_teardown),
		ztest_unit_test_setup_teardown(test_isl9238c_resume,
					       hibernate_test_teardown,
					       hibernate_test_teardown));
	ztest_run_test_suite(isl923x);
}

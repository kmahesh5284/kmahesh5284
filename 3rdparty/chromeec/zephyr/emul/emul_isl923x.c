/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define DT_DRV_COMPAT cros_isl923x_emul

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/i2c_emul.h>
#include <emul.h>
#include <errno.h>
#include <sys/__assert.h>

#include "driver/charger/isl923x.h"
#include "driver/charger/isl923x_public.h"
#include "emul/emul_common_i2c.h"
#include "emul/emul_isl923x.h"
#include "emul/emul_smart_battery.h"
#include "i2c.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(isl923x_emul, CONFIG_ISL923X_EMUL_LOG_LEVEL);

#define ISL923X_DATA_FROM_I2C_EMUL(_emul)                                    \
	CONTAINER_OF(CONTAINER_OF(_emul, struct i2c_common_emul_data, emul), \
		     struct isl923x_emul_data, common)

/** Mask used for the charge current register */
#define REG_CHG_CURRENT_MASK GENMASK(12, 2)

/** Mask used for the system voltage min register */
#define REG_SYS_VOLTAGE_MIN_MASK GENMASK(13, 8)

/** Mask used for the system voltage max register */
#define REG_SYS_VOLTAGE_MAX_MASK GENMASK(14, 3)

/** Mask used for the adapter current limit 1 register */
#define REG_ADAPTER_CURRENT_LIMIT1_MASK GENMASK(12, 2)

/** Mask used for the adapter current limit 2 register */
#define REG_ADAPTER_CURRENT_LIMIT2_MASK GENMASK(12, 2)

/** Mask used for the control 0 register */
#define REG_CONTROL0_MASK GENMASK(15, 1)

/** Mask used for the control 1 register */
#define REG_CONTROL1_MASK (GENMASK(15, 8) | GENMASK(6, 0))

/** Mask used for the control 2 register */
#define REG_CONTROL2_MASK GENMASK(15, 0)

/** Mask used for the control 3 register */
#define REG_CONTROL3_MASK GENMASK(15, 0)

/** Mask used for the control 4 register */
#define REG_CONTROL4_MASK GENMASK(15, 0)

/** Mask used for the control 8 register */
#define REG_CONTROL8_MASK GENMASK(15, 0)

/** Mask used for the AC PROCHOT register */
#define REG_PROCHOT_AC_MASK GENMASK(12, 7)

/** Mask used for the DC PROCHOT register */
#define REG_PROCHOT_DC_MASK GENMASK(13, 8)

#define DEFAULT_R_SNS 10
#define R_SNS CONFIG_CHARGER_SENSE_RESISTOR
#define REG_TO_CURRENT(REG) ((REG) * DEFAULT_R_SNS / R_SNS)

struct isl923x_emul_data {
	/** Common I2C data */
	struct i2c_common_emul_data common;
	/** Emulated charge current limit register */
	uint16_t current_limit_reg;
	/** Emulated adapter current limit 1 register */
	uint16_t adapter_current_limit1_reg;
	/** Emulated adapter current limit 2 register */
	uint16_t adapter_current_limit2_reg;
	/** Emulated min voltage register */
	uint16_t min_volt_reg;
	/** Emulated max voltage register */
	uint16_t max_volt_reg;
	/** Emulated manufacturer ID register */
	uint16_t manufacturer_id_reg;
	/** Emulated device ID register */
	uint16_t device_id_reg;
	/** Emulated control 0 register */
	uint16_t control_0_reg;
	/** Emulated control 1 register */
	uint16_t control_1_reg;
	/** Emulated control 2 register */
	uint16_t control_2_reg;
	/** Emulated control 3 register */
	uint16_t control_3_reg;
	/** Emulated control 4 register */
	uint16_t control_4_reg;
	/** Emulated control 8 register (RAA489000-only) */
	uint16_t control_8_reg;
	/** Emulated info 2 reg */
	uint16_t info_2_reg;
	/** Emulated AC PROCHOT register */
	uint16_t ac_prochot_reg;
	/** Emulated DC PROCHOT register */
	uint16_t dc_prochot_reg;
	/** Emulated ADC vbus register */
	uint16_t adc_vbus_reg;
	/** Pointer to battery emulator. */
	int battery_ord;
};

struct isl923x_emul_cfg {
	/** Common I2C config */
	const struct i2c_common_emul_cfg common;
};

const struct device *isl923x_emul_get_parent(const struct emul *emulator)
{
	struct isl923x_emul_data *data = emulator->data;

	return data->common.i2c;
}

struct i2c_emul *isl923x_emul_get_i2c_emul(const struct emul *emulator)
{
	struct isl923x_emul_data *data = emulator->data;

	return &(data->common.emul);
}

void isl923x_emul_reset(const struct emul *emulator)
{
	struct isl923x_emul_data *data = emulator->data;
	struct i2c_common_emul_data common_backup = data->common;
	int battery_ord = data->battery_ord;

	memset(data, 0, sizeof(struct isl923x_emul_data));
	data->common = common_backup;
	data->battery_ord = battery_ord;
}

void isl923x_emul_set_manufacturer_id(const struct emul *emulator,
				      uint16_t manufacturer_id)
{
	struct isl923x_emul_data *data = emulator->data;

	data->manufacturer_id_reg = manufacturer_id;
}

void isl923x_emul_set_device_id(const struct emul *emulator,
				uint16_t device_id)
{
	struct isl923x_emul_data *data = emulator->data;

	data->device_id_reg = device_id;
}

bool isl923x_emul_is_learn_mode_enabled(const struct emul *emulator)
{
	struct isl923x_emul_data *data = emulator->data;

	return (data->control_1_reg & ISL923X_C1_LEARN_MODE_ENABLE) != 0;
}

void isl923x_emul_set_learn_mode_enabled(const struct emul *emulator,
					 bool enabled)
{
	struct isl923x_emul_data *data = emulator->data;

	if (enabled)
		data->control_1_reg |= ISL923X_C1_LEARN_MODE_ENABLE;
	else
		data->control_1_reg &= ~ISL923X_C1_LEARN_MODE_ENABLE;
}

void isl923x_emul_set_adc_vbus(const struct emul *emulator,
			       uint16_t value)
{
	struct isl923x_emul_data *data = emulator->data;

	/* The VBUS voltage is returned in bits 13:6. The LSB is 96mV. */
	data->adc_vbus_reg = value & GENMASK(13, 6);
}

void raa489000_emul_set_acok_pin(const struct emul *emulator, uint16_t value)
{
	struct isl923x_emul_data *data = emulator->data;

	if (value)
		data->info_2_reg |= RAA489000_INFO2_ACOK;
	else
		data->info_2_reg &= ~RAA489000_INFO2_ACOK;
}

/** Convenience macro for reading 16-bit registers */
#define READ_REG_16(REG, BYTES, OUT)                             \
	do {                                                     \
		__ASSERT_NO_MSG((BYTES) == 0 || (BYTES) == 1);   \
		if ((BYTES) == 0)                                \
			*(OUT) = (uint8_t)((REG)&0xff);          \
		else                                             \
			*(OUT) = (uint8_t)(((REG) >> 8) & 0xff); \
		break;                                           \
	} while (0)

static int isl923x_emul_read_byte(struct i2c_emul *emul, int reg, uint8_t *val,
				  int bytes)
{
	struct isl923x_emul_data *data = ISL923X_DATA_FROM_I2C_EMUL(emul);

	switch (reg) {
	case ISL923X_REG_CHG_CURRENT:
		READ_REG_16(data->current_limit_reg, bytes, val);
		break;
	case ISL923X_REG_SYS_VOLTAGE_MIN:
		READ_REG_16(data->min_volt_reg, bytes, val);
		break;
	case ISL923X_REG_SYS_VOLTAGE_MAX:
		READ_REG_16(data->max_volt_reg, bytes, val);
		break;
	case ISL923X_REG_ADAPTER_CURRENT_LIMIT1:
		READ_REG_16(data->adapter_current_limit1_reg, bytes, val);
		break;
	case ISL923X_REG_ADAPTER_CURRENT_LIMIT2:
		READ_REG_16(data->adapter_current_limit2_reg, bytes, val);
		break;
	case ISL923X_REG_MANUFACTURER_ID:
		READ_REG_16(data->manufacturer_id_reg, bytes, val);
		break;
	case ISL923X_REG_DEVICE_ID:
		READ_REG_16(data->device_id_reg, bytes, val);
		break;
	case ISL923X_REG_CONTROL0:
		READ_REG_16(data->control_0_reg, bytes, val);
		break;
	case ISL923X_REG_CONTROL1:
		READ_REG_16(data->control_1_reg, bytes, val);
		break;
	case ISL923X_REG_CONTROL2:
		READ_REG_16(data->control_2_reg, bytes, val);
		break;
	case ISL9238_REG_CONTROL3:
		READ_REG_16(data->control_3_reg, bytes, val);
		break;
	case ISL9238_REG_CONTROL4:
		READ_REG_16(data->control_4_reg, bytes, val);
		break;
	case RAA489000_REG_CONTROL8:
		READ_REG_16(data->control_8_reg, bytes, val);
		break;
	case ISL9238_REG_INFO2:
		READ_REG_16(data->info_2_reg, bytes, val);
		break;
	case ISL923X_REG_PROCHOT_AC:
		READ_REG_16(data->ac_prochot_reg, bytes, val);
		break;
	case ISL923X_REG_PROCHOT_DC:
		READ_REG_16(data->dc_prochot_reg, bytes, val);
		break;
	case RAA489000_REG_ADC_VBUS:
		READ_REG_16(data->adc_vbus_reg, bytes, val);
		break;
	default:
		__ASSERT(false, "Attempt to read unimplemented reg 0x%02x",
			 reg);
		return -EINVAL;
	}
	return 0;
}

uint16_t isl923x_emul_peek_reg(struct i2c_emul *i2c_emul, int reg)
{
	uint8_t bytes[2];

	isl923x_emul_read_byte(i2c_emul, reg, &bytes[0], 0);
	isl923x_emul_read_byte(i2c_emul, reg, &bytes[1], 1);

	return bytes[1] << 8 | bytes[0];
}

/** Convenience macro for writing 16-bit registers */
#define WRITE_REG_16(REG, BYTES, VAL, MASK)                    \
	do {                                                   \
		__ASSERT_NO_MSG((BYTES) == 1 || (BYTES) == 2); \
		if ((BYTES) == 1)                              \
			(REG) = (VAL) & (MASK);                \
		else                                           \
			(REG) |= ((VAL) << 8) & (MASK);        \
	} while (0)

static int isl923x_emul_write_byte(struct i2c_emul *emul, int reg, uint8_t val,
				   int bytes)
{
	struct isl923x_emul_data *data = ISL923X_DATA_FROM_I2C_EMUL(emul);

	switch (reg) {
	case ISL923X_REG_CHG_CURRENT:
		WRITE_REG_16(data->current_limit_reg, bytes, val,
			     REG_CHG_CURRENT_MASK);
		break;
	case ISL923X_REG_SYS_VOLTAGE_MIN:
		WRITE_REG_16(data->min_volt_reg, bytes, val,
			     REG_SYS_VOLTAGE_MIN_MASK);
		break;
	case ISL923X_REG_SYS_VOLTAGE_MAX:
		WRITE_REG_16(data->max_volt_reg, bytes, val,
			     REG_SYS_VOLTAGE_MAX_MASK);
		break;
	case ISL923X_REG_ADAPTER_CURRENT_LIMIT1:
		WRITE_REG_16(data->adapter_current_limit1_reg, bytes, val,
			     REG_ADAPTER_CURRENT_LIMIT1_MASK);
		break;
	case ISL923X_REG_ADAPTER_CURRENT_LIMIT2:
		WRITE_REG_16(data->adapter_current_limit2_reg, bytes, val,
			     REG_ADAPTER_CURRENT_LIMIT2_MASK);
		break;
	case ISL923X_REG_CONTROL0:
		WRITE_REG_16(data->control_0_reg, bytes, val,
			     REG_CONTROL0_MASK);
		break;
	case ISL923X_REG_CONTROL1:
		WRITE_REG_16(data->control_1_reg, bytes, val,
			     REG_CONTROL1_MASK);
		break;
	case ISL923X_REG_CONTROL2:
		WRITE_REG_16(data->control_2_reg, bytes, val,
			     REG_CONTROL2_MASK);
		break;
	case ISL9238_REG_CONTROL3:
		WRITE_REG_16(data->control_3_reg, bytes, val,
			     REG_CONTROL3_MASK);
		break;
	case ISL9238_REG_CONTROL4:
		WRITE_REG_16(data->control_4_reg, bytes, val,
			     REG_CONTROL4_MASK);
		break;
	case RAA489000_REG_CONTROL8:
		WRITE_REG_16(data->control_8_reg, bytes, val,
			     REG_CONTROL8_MASK);
		break;
	case ISL9238_REG_INFO2:
		__ASSERT(false, "Write to read-only reg ISL9238_REG_INFO2");
		break;
	case ISL923X_REG_PROCHOT_AC:
		WRITE_REG_16(data->ac_prochot_reg, bytes, val,
			     REG_PROCHOT_AC_MASK);
		break;
	case ISL923X_REG_PROCHOT_DC:
		WRITE_REG_16(data->dc_prochot_reg, bytes, val,
			     REG_PROCHOT_DC_MASK);
		break;
	default:
		__ASSERT(false, "Attempt to write unimplemented reg 0x%02x",
			 reg);
		return -EINVAL;
	}
	return 0;
}

static int isl923x_emul_finish_write(struct i2c_emul *emul, int reg, int bytes)
{
	struct isl923x_emul_data *data = ISL923X_DATA_FROM_I2C_EMUL(emul);
	struct i2c_emul *battery_i2c_emul;
	struct sbat_emul_bat_data *bat;
	int16_t current;

	switch (reg) {
	case ISL923X_REG_CHG_CURRENT:
		/* Write current to battery. */
		if (data->battery_ord >= 0) {
			battery_i2c_emul = sbat_emul_get_ptr(data->battery_ord);
			if (battery_i2c_emul != NULL) {
				bat = sbat_emul_get_bat_data(battery_i2c_emul);
				if (bat != NULL) {
					current = REG_TO_CURRENT(
						data->current_limit_reg);
					if (current > 0)
						bat->cur = current;
					else
						bat->cur = -5;
				}
			}
		}
		break;
	}
	return 0;
}

static int emul_isl923x_init(const struct emul *emul,
			     const struct device *parent)
{
	const struct isl923x_emul_cfg *cfg = emul->cfg;
	struct isl923x_emul_data *data = emul->data;

	data->common.emul.api = &i2c_common_emul_api;
	data->common.emul.addr = cfg->common.addr;
	data->common.emul.parent = emul;
	data->common.i2c = parent;
	data->common.cfg = &cfg->common;
	i2c_common_emul_init(&data->common);

	return i2c_emul_register(parent, emul->dev_label, &data->common.emul);
}

#define INIT_ISL923X(n)                                                        \
	static struct isl923x_emul_data isl923x_emul_data_##n = {              \
		.common = {                                                    \
			.write_byte = isl923x_emul_write_byte,                 \
			.read_byte = isl923x_emul_read_byte,                   \
			.finish_write = isl923x_emul_finish_write,             \
		},                                                             \
		.battery_ord = COND_CODE_1(                                    \
			DT_INST_NODE_HAS_PROP(n, battery),                     \
			(DT_DEP_ORD(DT_INST_PROP(n, battery))),                \
			(-1)),                                                 \
	};                                                                     \
	static struct isl923x_emul_cfg isl923x_emul_cfg_##n = {                \
	.common = {                                                            \
		.i2c_label = DT_INST_BUS_LABEL(n),                             \
		.dev_label = DT_INST_LABEL(n),                                 \
		.addr = DT_INST_REG_ADDR(n),                                   \
		},                                                             \
	};                                                                     \
	EMUL_DEFINE(emul_isl923x_init, DT_DRV_INST(n), &isl923x_emul_cfg_##n,  \
		    &isl923x_emul_data_##n)

DT_INST_FOREACH_STATUS_OKAY(INIT_ISL923X)

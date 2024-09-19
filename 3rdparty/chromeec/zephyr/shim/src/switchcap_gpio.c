/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <devicetree.h>
#include "common.h"
#include "gpio.h"

#if DT_NODE_EXISTS(DT_PATH(switchcap))

#if !DT_NODE_HAS_COMPAT(DT_PATH(switchcap), switchcap_gpio)
#error "Invalid /switchcap node in device tree"
#endif

#define SC_PIN_ENABLE_PHANDLE \
	DT_PHANDLE_BY_IDX(DT_PATH(switchcap), enable_pin, 0)
#define SC_PIN_ENABLE \
	GPIO_SIGNAL(SC_PIN_ENABLE_PHANDLE)

#define SC_PIN_POWER_GOOD_PHANDLE \
	DT_PHANDLE_BY_IDX(DT_PATH(switchcap), power_good_pin, 0)
#define SC_PIN_POWER_GOOD_EXISTS \
	DT_NODE_EXISTS(SC_PIN_POWER_GOOD_PHANDLE)
#define SC_PIN_POWER_GOOD \
	GPIO_SIGNAL(SC_PIN_POWER_GOOD_PHANDLE)

void board_set_switchcap_power(int enable)
{
	gpio_set_level(SC_PIN_ENABLE, enable);
}

int board_is_switchcap_enabled(void)
{
	return gpio_get_level(SC_PIN_ENABLE);
}

int board_is_switchcap_power_good(void)
{
#if SC_PIN_POWER_GOOD_EXISTS
	return gpio_get_level(SC_PIN_POWER_GOOD);
#else
	return 1;
#endif
}

#endif

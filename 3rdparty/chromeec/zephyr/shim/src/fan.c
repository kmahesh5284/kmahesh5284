/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "fan.h"
#include "pwm.h"
#include "pwm/pwm.h"
#include "system.h"
#include "math_util.h"
#include "hooks.h"
#include "gpio_signal.h"
#include "gpio.h"
#include <logging/log.h>
#include <sys/util_macro.h>
#include <drivers/sensor.h>

LOG_MODULE_REGISTER(fan_shim, LOG_LEVEL_ERR);

#define FAN_CONFIGS(node_id)                                                   \
	const struct fan_conf node_id##_conf = {                               \
		.flags = (COND_CODE_1(DT_PROP(node_id, not_use_rpm_mode),      \
				      (0), (FAN_USE_RPM_MODE))) |              \
			 (COND_CODE_1(DT_PROP(node_id, use_fast_start),        \
				      (FAN_USE_FAST_START), (0))),             \
		.ch = node_id,                                                 \
		.pgood_gpio = COND_CODE_1(                                     \
			DT_NODE_HAS_PROP(node_id, pgood_gpio),                 \
			(GPIO_SIGNAL(DT_PHANDLE(node_id, pgood_gpio))),        \
			(GPIO_UNIMPLEMENTED)),                                 \
		.enable_gpio = COND_CODE_1(                                    \
			DT_NODE_HAS_PROP(node_id, enable_gpio),                \
			(GPIO_SIGNAL(DT_PHANDLE(node_id, enable_gpio))),       \
			(GPIO_UNIMPLEMENTED)),                                 \
	};                                                                     \
	const struct fan_rpm node_id##_rpm = {                                 \
		.rpm_min = DT_PROP(node_id, rpm_min),                          \
		.rpm_start = DT_PROP(node_id, rpm_start),                      \
		.rpm_max = DT_PROP(node_id, rpm_max),                          \
	};

#define FAN_INST(node_id)              \
	[node_id] = {                  \
		.conf = &node_id##_conf, \
		.rpm = &node_id##_rpm,   \
	},

#define FAN_CONTROL_INST(node_id)                                \
	[node_id] = {                                            \
		.pwm_id = PWM_CHANNEL(DT_PHANDLE(node_id, pwm)), \
	},

#if DT_NODE_EXISTS(DT_INST(0, named_fans))
DT_FOREACH_CHILD(DT_INST(0, named_fans), FAN_CONFIGS)
#endif /* named_fan */

const struct fan_t fans[] = {
#if DT_NODE_EXISTS(DT_INST(0, named_fans))
	DT_FOREACH_CHILD(DT_INST(0, named_fans), FAN_INST)
#endif /* named_fan */
};

#define TACHO_DEV_INIT(node_id) {                         \
	fan_control[node_id].tach =                       \
		DEVICE_DT_GET(DT_PHANDLE(node_id, tach)); \
	}

/* Rpm deviation (Unit:percent) */
#ifndef RPM_DEVIATION
#define RPM_DEVIATION 7
#endif

/* Margin of target rpm */
#define RPM_MARGIN(rpm_target) (((rpm_target)*RPM_DEVIATION) / 100)

/* Fan mode */
enum fan_mode {
	/* FAN rpm mode */
	FAN_RPM = 0,
	/* FAN duty mode */
	FAN_DUTY,
};

/* Fan status data structure */
struct fan_status_t {
	/* Fan mode */
	enum fan_mode current_fan_mode;
	/* Actual rpm */
	int rpm_actual;
	/* Target rpm */
	int rpm_target;
	/* Fan config flags */
	unsigned int flags;
	/* Automatic fan status */
	enum fan_status auto_status;
};

/* Data structure to define tachometer. */
struct fan_control_t {
	const struct device *tach;
	enum pwm_channel pwm_id;
};

static struct fan_status_t fan_status[FAN_CH_COUNT];
static int rpm_pre[FAN_CH_COUNT];
static struct fan_control_t fan_control[] = {
#if DT_NODE_EXISTS(DT_INST(0, named_fans))
	DT_FOREACH_CHILD(DT_INST(0, named_fans), FAN_CONTROL_INST)
#endif /* named_fan */
};

/**
 * Get fan rpm value
 *
 * @param   ch      operation channel
 * @return          Actual rpm
 */
static int fan_rpm(int ch)
{
	struct sensor_value val = { 0 };

	sensor_sample_fetch_chan(fan_control[ch].tach, SENSOR_CHAN_RPM);
	sensor_channel_get(fan_control[ch].tach, SENSOR_CHAN_RPM, &val);
	return (int)val.val1;
}

/**
 * Check all fans are stopped
 *
 * @return   1: all fans are stopped. 0: else.
 */
static int fan_all_disabled(void)
{
	int ch;

	for (ch = 0; ch < fan_get_count(); ch++)
		if (fan_status[ch].auto_status != FAN_STATUS_STOPPED)
			return 0;
	return 1;
}

/**
 * Adjust fan duty by difference between target and actual rpm
 *
 * @param   ch        operation channel
 * @param   rpm_diff  difference between target and actual rpm
 * @param   duty      current fan duty
 */
static void fan_adjust_duty(int ch, int rpm_diff, int duty)
{
	int duty_step = 0;

	/* Find suitable duty step */
	if (ABS(rpm_diff) >= 2000)
		duty_step = 20;
	else if (ABS(rpm_diff) >= 1000)
		duty_step = 10;
	else if (ABS(rpm_diff) >= 500)
		duty_step = 5;
	else if (ABS(rpm_diff) >= 250)
		duty_step = 3;
	else
		duty_step = 1;

	/* Adjust fan duty step by step */
	if (rpm_diff > 0)
		duty = MIN(duty + duty_step, 100);
	else
		duty = MAX(duty - duty_step, 1);

	fan_set_duty(ch, duty);

	LOG_DBG("fan%d: duty %d, rpm_diff %d", ch, duty, rpm_diff);
}

/**
 * Smart fan control function.
 *
 * The function sets the pwm duty to reach the target rpm
 *
 * @param   ch         operation channel
 * @param   rpm_actual actual operation rpm value
 * @param   rpm_target target operation rpm value
 * @return  current    fan control status
 */
enum fan_status fan_smart_control(int ch, int rpm_actual, int rpm_target)
{
	int duty, rpm_diff;

	/* wait rpm is stable */
	if (ABS(rpm_actual - rpm_pre[ch]) > RPM_MARGIN(rpm_actual)) {
		rpm_pre[ch] = rpm_actual;
		return FAN_STATUS_CHANGING;
	}

	/* Record previous rpm */
	rpm_pre[ch] = rpm_actual;

	/* Adjust PWM duty */
	rpm_diff = rpm_target - rpm_actual;
	duty = fan_get_duty(ch);
	if (duty == 0 && rpm_target == 0)
		return FAN_STATUS_STOPPED;

	/* Increase PWM duty */
	if (rpm_diff > RPM_MARGIN(rpm_target)) {
		if (duty == 100)
			return FAN_STATUS_FRUSTRATED;

		fan_adjust_duty(ch, rpm_diff, duty);
		return FAN_STATUS_CHANGING;
		/* Decrease PWM duty */
	} else if (rpm_diff < -RPM_MARGIN(rpm_target)) {
		if (duty == 1 && rpm_target != 0)
			return FAN_STATUS_FRUSTRATED;

		fan_adjust_duty(ch, rpm_diff, duty);
		return FAN_STATUS_CHANGING;
	}

	return FAN_STATUS_LOCKED;
}

void fan_tick_func(void)
{
	int ch;

	for (ch = 0; ch < FAN_CH_COUNT; ch++) {
		volatile struct fan_status_t *p_status = fan_status + ch;
		/* Make sure rpm mode is enabled */
		if (p_status->current_fan_mode != FAN_RPM) {
			/* Fan in duty mode still want rpm_actual being updated.
			 */
			if (p_status->flags & FAN_USE_RPM_MODE) {
				p_status->rpm_actual = fan_rpm(ch);
				if (p_status->rpm_actual > 0)
					p_status->auto_status =
						FAN_STATUS_LOCKED;
				else
					p_status->auto_status =
						FAN_STATUS_STOPPED;
				continue;
			} else {
				if (fan_get_duty(ch) > 0)
					p_status->auto_status =
						FAN_STATUS_LOCKED;
				else
					p_status->auto_status =
						FAN_STATUS_STOPPED;
			}
			continue;
		}
		if (!fan_get_enabled(ch))
			continue;
		/* Get actual rpm */
		p_status->rpm_actual = fan_rpm(ch);
		/* Do smart fan stuff */
		p_status->auto_status = fan_smart_control(
			ch, p_status->rpm_actual, p_status->rpm_target);
	}
}
DECLARE_HOOK(HOOK_TICK, fan_tick_func, HOOK_PRIO_DEFAULT);

int fan_get_duty(int ch)
{
	enum pwm_channel pwm_id = fan_control[ch].pwm_id;

	/* Return percent */
	return pwm_get_duty(pwm_id);
}

int fan_get_rpm_mode(int ch)
{
	return fan_status[ch].current_fan_mode == FAN_RPM ? 1 : 0;
}

void fan_set_rpm_mode(int ch, int rpm_mode)
{
	if (rpm_mode && (fan_status[ch].flags & FAN_USE_RPM_MODE))
		fan_status[ch].current_fan_mode = FAN_RPM;
	else
		fan_status[ch].current_fan_mode = FAN_DUTY;
}

int fan_get_rpm_actual(int ch)
{
	/* Check PWM is enabled first */
	if (fan_get_duty(ch) == 0)
		return 0;

	LOG_DBG("fan %d: get actual rpm = %d", ch, fan_status[ch].rpm_actual);
	return fan_status[ch].rpm_actual;
}

int fan_get_enabled(int ch)
{
	enum pwm_channel pwm_id = fan_control[ch].pwm_id;

	return pwm_get_enabled(pwm_id);
}

void fan_set_enabled(int ch, int enabled)
{
	enum pwm_channel pwm_id = fan_control[ch].pwm_id;

	if (!enabled)
		fan_status[ch].auto_status = FAN_STATUS_STOPPED;
	pwm_enable(pwm_id, enabled);
}

void fan_channel_setup(int ch, unsigned int flags)
{
	volatile struct fan_status_t *p_status = fan_status + ch;

	if (flags & FAN_USE_RPM_MODE) {
		DT_FOREACH_CHILD(DT_INST(0, named_fans), TACHO_DEV_INIT)
	}

	p_status->flags = flags;
	/* Set default fan states */
	p_status->current_fan_mode = FAN_DUTY;
	p_status->auto_status = FAN_STATUS_STOPPED;
}

void fan_set_duty(int ch, int percent)
{
	enum pwm_channel pwm_id = fan_control[ch].pwm_id;

	/* duty is zero */
	if (!percent) {
		fan_status[ch].auto_status = FAN_STATUS_STOPPED;
		if (fan_all_disabled())
			enable_sleep(SLEEP_MASK_FAN);
	} else
		disable_sleep(SLEEP_MASK_FAN);

	/* Set the duty cycle of PWM */
	pwm_set_duty(pwm_id, percent);
}

int fan_get_rpm_target(int ch)
{
	return fan_status[ch].rpm_target;
}

enum fan_status fan_get_status(int ch)
{
	return fan_status[ch].auto_status;
}

void fan_set_rpm_target(int ch, int rpm)
{
	if (rpm == 0) {
		/* If rpm = 0, disable PWM immediately. Why?*/
		fan_set_duty(ch, 0);
	} else {
		/* This is the counterpart of disabling PWM above. */
		if (!fan_get_enabled(ch))
			fan_set_enabled(ch, 1);
		if (rpm > fans[ch].rpm->rpm_max)
			rpm = fans[ch].rpm->rpm_max;
		else if (rpm < fans[ch].rpm->rpm_min)
			rpm = fans[ch].rpm->rpm_min;
	}

	/* Set target rpm */
	fan_status[ch].rpm_target = rpm;
	LOG_DBG("fan %d: set target rpm = %d", ch, fan_status[ch].rpm_target);
}

int fan_is_stalled(int ch)
{
	int is_pgood = 1;

	if (gpio_is_implemented(fans[ch].conf->enable_gpio))
		is_pgood = gpio_get_level(fans[ch].conf->enable_gpio);

	return fan_get_enabled(ch) && fan_get_duty(ch) &&
	       !fan_get_rpm_actual(ch) && is_pgood;
}

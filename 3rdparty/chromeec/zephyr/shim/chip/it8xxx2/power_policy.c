/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pm/pm.h>
#include <pm/policy.h>
#include <soc.h>
#include <zephyr.h>

#include "system.h"

static const struct pm_state_info pm_states[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));

/* CROS PM policy handler */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);

	/* Deep sleep is allowed */
	if (DEEP_SLEEP_ALLOWED) {
		/*
		 * If there are multiple power states, iterating backward
		 * is needed to take priority into account.
		 */
		for (int i = 0; i < ARRAY_SIZE(pm_states); i++) {
			/*
			 * To check if given power state is enabled and
			 * could be used.
			 */
			if (!pm_constraint_get(pm_states[i].state)) {
				continue;
			}

			return &pm_states[i];
		}
	}

	return NULL;
}

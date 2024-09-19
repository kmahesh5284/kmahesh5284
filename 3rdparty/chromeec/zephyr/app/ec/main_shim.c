/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <kernel.h>
#include "ec_app_main.h"

/** A stub main to call the real ec app main function. LCOV_EXCL_START */
void main(void)
{
	ec_app_main();

#ifdef CONFIG_THREAD_MONITOR
	/*
	 * Avoid returning so that the main stack is displayed by the
	 * "kernel stacks" shell command.
	 */
	k_sleep(K_FOREVER);
#endif
}
/* LCOV_EXCL_STOP */

# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

register_npcx_project(
    project_name="herobrine_npcx9",
    zephyr_board="herobrine_npcx9",
    dts_overlays=[
        "gpio.dts",
        "battery.dts",
        "i2c.dts",
        "motionsense.dts",
        "switchcap.dts",
        "usbc.dts",
    ],
)

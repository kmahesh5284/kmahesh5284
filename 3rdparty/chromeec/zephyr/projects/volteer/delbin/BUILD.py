# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

register_npcx_project(
    project_name="delbin",
    zephyr_board="volteer",
    dts_overlays=["gpio.dts", "motionsense.dts"],
)

# -*- makefile -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Baseboard specific files build
#

#Intel RVP common files
baseboard-y=baseboard.o
baseboard-$(CONFIG_LED_COMMON)+=led.o led_states.o

ifneq ($(CONFIG_USB_POWER_DELIVERY),)
baseboard-$(CONFIG_USB_POWER_DELIVERY)+=chg_usb_pd.o
baseboard-$(CONFIG_INTEL_RVP_MECC_VERSION_0_9)+=chg_usb_pd_mecc_0_9.o
baseboard-$(CONFIG_INTEL_RVP_MECC_VERSION_0_9)+=usb_pd_policy_mecc_0_9.o
baseboard-$(CONFIG_INTEL_RVP_MECC_VERSION_1_0)+=chg_usb_pd_mecc_1_0.o
baseboard-$(CONFIG_INTEL_RVP_MECC_VERSION_1_0)+=usb_pd_policy_mecc_1_0.o
endif

#EC specific files
baseboard-$(VARIANT_INTELRVP_EC_IT8320)+=ite_ec.o
baseboard-$(VARIANT_INTELRVP_EC_MCHP)+=mchp_ec.o
baseboard-$(VARIANT_INTELRVP_EC_NPCX)+=npcx_ec.o

#BC1.2 specific files
baseboard-$(CONFIG_BC12_DETECT_MAX14637)+=bc12.o

#Common board specific files
ifneq ($(filter y,$(BOARD_ADLRVPP_ITE) $(BOARD_ADLRVPM_ITE) \
		$(BOARD_ADLRVPP_MCHP1521) $(BOARD_ADLRVPP_NPCX) \
		$(BOARD_ADLRVPP_MCHP1727)),)
baseboard-y+=adlrvp.o
baseboard-$(CONFIG_BATTERY_SMART)+=adlrvp_battery.o
endif

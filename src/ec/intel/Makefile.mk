## SPDX-License-Identifier: GPL-2.0-or-later

## ifeq ($(CONFIG_EC_INTEL_ADLP),y)

bootblock-y += board_id.c
romstage-y += board_id.c
ramstage-y += board_id.c

CPPFLAGS_common += -I$(src)/ec/intel

## endif

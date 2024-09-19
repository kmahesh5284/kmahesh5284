/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <baseboard/ec.h>
#include <baseboard/gpio.h>

DefinitionBlock(
	"dsdt.aml",
	"DSDT",
	ACPI_DSDT_REV_2,	// DSDT revision: ACPI v2.0 and up
	OEM_ID,
	ACPI_TABLE_CREATOR,
	0x20110725		// OEM Version
)
{
	#include <acpi/dsdt_top.asl>
	// platform ACPI tables 
	#include <soc/intel/common/block/acpi/acpi/platform.asl>	   
	// global NVS and variables
	#include <soc/intel/common/block/acpi/acpi/globalnvs.asl>
	#include <cpu/intel/common/acpi/cpu.asl>

	Device (\_SB.PCI0) {
		#include <soc/intel/common/block/acpi/acpi/northbridge.asl>
		#include <soc/intel/alderlake/acpi/southbridge.asl>
		#include <soc/intel/alderlake/acpi/tcss.asl>
	}

#if CONFIG(EC_GOOGLE_CHROMEEC)
	/* ChromeOS Embedded Controller */
	Scope (\_SB.PCI0.LPCB)
	{
		/* ACPI code for EC SuperIO functions */
		#include <ec/google/chromeec/acpi/superio.asl>
		/* ACPI code for EC functions */
		#include <ec/google/chromeec/acpi/ec.asl>
	}
#endif
	// Chipset specific sleep states
	#include <southbridge/intel/common/acpi/sleepstates.asl>
	//#include "acpi/mainboard.asl" 
}

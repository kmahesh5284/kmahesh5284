/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/x86/smm.h>

/* default implementation of the !HAVE_CONFIGURABLE_APMC_SMI_PORT case */
uint16_t pm_acpi_smi_cmd_port(void)
{	
	printk(BIOS_DEBUG, "USER_INFO: Entered into loop to get the APM_CNT value\n");
	printk(BIOS_DEBUG, "USER_INFO: APM_CNT value is:%x\n",APM_CNT);

	return APM_CNT;
}

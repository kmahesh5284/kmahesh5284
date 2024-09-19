/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/x86/smm.h>
#include <console/console.h>
#include <ec/google/chromeec/smm.h>
#include <intelblocks/smihandler.h>
#include <baseboard/ec.h>

void mainboard_smi_espi_handler(void)
{
	if (!CONFIG(EC_GOOGLE_CHROMEEC))
		return;
	printk(BIOS_DEBUG,"Uer_info entered into SMI ESPI handler\n");
	chromeec_smi_process_events();
}

void mainboard_smi_sleep(u8 slp_typ)
{
	if (!CONFIG(EC_GOOGLE_CHROMEEC))
		return;
	printk(BIOS_DEBUG,"Uer_info entered into SMI sleep\n");

	chromeec_smi_sleep(slp_typ, MAINBOARD_EC_S3_WAKE_EVENTS, MAINBOARD_EC_S5_WAKE_EVENTS);
}

int mainboard_smi_apmc(u8 apmc)
{
	int tmp= apmc;
	printk(BIOS_DEBUG,"USER_INFO: Entered into SMI APMC outer loop\n");
	printk(BIOS_DEBUG,"USER_INFO: The apmc value is %d\n",tmp);

	if (CONFIG(EC_GOOGLE_CHROMEEC))
		chromeec_smi_apmc(apmc, MAINBOARD_EC_SCI_EVENTS, MAINBOARD_EC_SMI_EVENTS);

	return 0;
}

/* SPDX-License-Identifier: GPL-2.0-only */
/* The format of the AZALIA_PIN_CFG macros is AZALIA_PIN_CFG(codec, pin, val)
   where codec is the index of the codec,
   pin is the pin widget id, 
   and val is a 32 bit value corresponding to the "Configuration Default" register */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* coreboot specific header */
	0x10ec0256,	// Codec Vendor / Device ID: Realtek ALC256
	0x10ec12f8,	// Subsystem ID
	0x00000025,	// Number of jacks (NID entries)

	/* Reset Codec First */
	AZALIA_RESET(0x1),

	/* HDA Codec Subsystem ID Verb table */
	AZALIA_SUBVENDOR(0, 0x10ec12f8),
	0x001720F8,
	0x00172112,
	0x001722EC,
	0x00172310,

	/* Pin Widget Verb Table */
	
	AZALIA_PIN_CFG(0, 0x12, 0x40000000),	//
	AZALIA_PIN_CFG(0, 0x13, 0x411111F0),
	AZALIA_PIN_CFG(0, 0x14, 0x90170110),	// Front jack, lineout for jack device (Port-D) 
	AZALIA_PIN_CFG(0, 0x18, 0x411111F0),	// NPC
	AZALIA_PIN_CFG(0, 0x19, 0x04A11030),	// MIC2, mic input for MIC device (Port-F) 
	AZALIA_PIN_CFG(0, 0x1a, 0x411111F0),	// LINE1 jack, Linein for jack device (Port-C)
	AZALIA_PIN_CFG(0, 0x1b, 0x411111F0),	// LINE2 jack (Port-E)
	AZALIA_PIN_CFG(0, 0x1d, 0x40400001),	// BEEP-IN input for codec beep
	AZALIA_PIN_CFG(0, 0x1e, 0x411111F0),	// S/PDIF-OUT jack detection
	AZALIA_PIN_CFG(0, 0x21, 0x04211020),	// Line2 jack, headphone out for jack devices (HP-OUT) (Port-I)
						//
	//===== Pin Widget Verb-table =====

	//Widget node 0x01 : Widget Reset
	0x0017FF00,
	0x0017FF00,
	0x0017FF00,
	0x0017FF00,
	
	//Pin widget 0x12 - DMIC
	0x01271C00,
	0x01271D00,
	0x01271E00,
	0x01271F40,

	//Pin widget 0x13 - DMIC
	0x01371CF0,
	0x01371D11,
	0x01371E11,
	0x01371F41,

	//Pin widget 0x14 - Front (Port-D)
	0x01471C10,
	0x01471D01,
	0x01471E17,
	0x01471F90,

	//Pin widget 0x18 - NPC
	0x01871CF0,
	0x01871D11,
	0x01871E11,
	0x01871F41,

	//Pin widget 0x19 - MIC2 (Port-F)
	0x01971C30,
	0x01971D10,
	0x01971EA1,
	0x01971F04,

	// Pin widget 0x1A - LINE1 (Port-C)
	0x01A71CF0,
	0x01A71D11,
	0x01A71E11,
	0x01A71F41,

	// Pin widget 0x1B - LINE2 (Port-E)
	0x01B71CF0,
	0x01B71D11,
	0x01B71E11,
	0x01B71F41,

	// Pin widget 0x1D - BEEP-IN
	0x01D71C01,
	0x01D71D00,
	0x01D71E40,
	0x01D71F40,

	// Pin widget 0x1E - S/PDIF-OUT
	0x01E71CF0,
	0x01E71D11,
	0x01E71E11,
	0x01E71F41,

	// Pin widget 0x21 - HP-OUT (Port-I) 
	0x02171C20,
	0x02171D10,
	0x02171E21,
	0x02171F04,

	// Hidden Reset Register
	0x0205001A,
	0x0204C003,
	0x0205001A,
	0x02048003,
	
	// Pin widget 0x20- 1 Set combo jack auto detect when HP-JD=1 and Class-D AMP output for 4R/2W
	0x0205001B,
	0x02040A4B,
	0x02050038,
	0x02047901,
	
	//Pin widget 0x20- 2 HW EQ set 200Hz HPF
	0x05350000,
	0x0534201A,
	0x05350000,
	0x0534201A,
	
	//Pin widget 0x20- 3 HW EQ
	0x0535001D,
	0x05340800,
	0x0535001E,
	0x05340800,
	
	//Pin widget 0x20- 4 HW EQ
	0x05350003,
	0x05341F2C,
	0x05350004,
	0x05340000,
	
	//Pin widget 0x20- 5 HW EQ
	0x05450000,
	0x05442000,
	0x0545001D,
	0x05440800,
	
	//Pin widget 0x20- 6 HW EQ
	0x0545001E,
	0x05440800,
	0x05450003,
	0x05441F2C,
	
	//Pin widget 0x20- 7 HW EQ
	0x05450004,
	0x05440000,
	0x05350000,
	0x0534E01A,
	
	//Pin widget 0x20- 8 AGC compression is 1 and set AGC limit to -1.5dB
	0x02050016,
	0x02040C50,
	0x02050012,
	0x0204EBC1,
	
	//Pin widget 0x20- 9 Set AGC Post gain 0dB then Enable AGC
	0x02050013,
	0x0204401F,
	0x02050016,
	0x02040E50,
	
	//Pin widget 0x20- 10 Set AGC
	0x02050020,
	0x020451FF,
	0x02050020,
	0x020451FF,
	
	//Pin widget 0x20- 10 MIC_SEL_L is enable and Enable MIC SW then Set RING2 not pull low
	0x02050045,
	0x02047489,
	0x02050046,
	0x02040004,
	
	//Pin widget 0x20- 11 Pull down I2C and enable Gating Silence Detector
	0x02050034,
	0x0204A23C,
	0x02050037,
	0x0204FE15,
	
	//Pin widget 0x20- 12 Zero data mode Threshold (-78dB) and Power down JD2 and JD3
	0x02050030,
	0x02048000,
	0x02050008,
	0x02046A6C,
};

const u32 pc_beep_verbs[] = {
};

AZALIA_ARRAY_SIZES;

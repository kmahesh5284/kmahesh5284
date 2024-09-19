/* SPDX-License-Identifier: GPL-2.0-only */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <types.h>
#include <vendorcode/google/chromeos/chromeos.h>

/* Pad configuration in ramstage */
static const struct pad_config gpio_table[] = {

	/* --------Ethernet GPIO's................*/

        /* 19: GPIO pin for PCIE LAN SRCCLKREQ6 */
        PAD_CFG_NF(GPP_F19, NONE, DEEP, NF1),
        /* 2: GPD_2_LAN_WAKE_N */
        PAD_CFG_NF(GPD2, NONE, PWROK, NF1),
//        PAD_CFG_GPI_SCI_LOW(GPD2, NONE, DEEP, EDGE_SINGLE),
//	PAD_CFG_NF(GPD2, NATIVE, PWROK, NF1),
	/* 21 : LAN_ISOLATE# */
        PAD_CFG_GPO(GPP_A21, 1, PLTRST),

	/*-------Hall Sensor GPIO----------------*/

	/* 3: HALL_SENSOR_INIT */
        PAD_CFG_NF(GPP_D3, NONE, DEEP, NF1),
//      PAD_CFG_GPI_IRQ_WAKE(GPP_D3, NONE, DEEP, LEVEL, INVERT),
//      PAD_CFG_GPI_INT(GPP_D3, NONE, PLTRST, LEVEL),
//      PAD_CFG_GPI_SCI_HIGH(GPP_D3, NONE, PLTRST, LEVEL),

        /* GPIO A0-A5, A9-A10 default function is NF1 for eSPI interface when
	  eSPI is enabled */
	//A Series
	/* 0: ESPI_IO0_EC_R / ESPI_IO0_HDR */
        PAD_CFG_NF(GPP_A0, NONE, DEEP, NF1),
        /* 1: ESPI_IO1_EC_R / ESPI_IO1_HDR */
        PAD_CFG_NF(GPP_A1, NONE, DEEP, NF1),
        /* 2: ESPI_IO2_EC_R / ESPI_IO2_HDR */
        PAD_CFG_NF(GPP_A2, NONE, DEEP, NF1),
        /* 3: ESPI_IO3_EC_R / ESPI_IO3_HDR */
        PAD_CFG_NF(GPP_A3, NONE, DEEP, NF1),
        /* 4: ESPI_CS0_EC_R_N / ESPI_CS0_HDR_N */
        PAD_CFG_NF(GPP_A4, NONE, DEEP, NF1),
        /* 5: ESPI_ALERT0_EC_R_N / ESPI_ALERT0_HDR_N */
        PAD_CFG_NF(GPP_A5, NONE, DEEP, NF1),
	/* 6: NC */
	PAD_NC(GPP_A6, NONE),
	/* 7: NC */
	PAD_NC(GPP_A7, NONE),
	/* 8: NC */
	PAD_NC(GPP_A8, NONE),
        /* 9: ESPI_CLK_EC_R / ESPI_CLK_HDR */
        PAD_CFG_NF(GPP_A9, NONE, DEEP, NF1),
        /* 10: ESPI_RST_EC_R_N / ESPI_RST_HDR_N */
        PAD_CFG_NF(GPP_A10, NONE, DEEP, NF1),
	/* 11: NC */
	PAD_NC(GPP_A11, NONE),
        /* 13: BT_RF_KILL_N */
        PAD_CFG_GPO(GPP_A13, 1, PLTRST),
	/* 14 : USB_3.0_OC#_P1  */
        PAD_CFG_NF(GPP_A14, NONE, DEEP, NF1),
        /* 15 : USB_3.0_OC#_P2 */
        PAD_CFG_NF(GPP_A15, NONE, DEEP, NF1),
	/* 16: USB2 OC3 pins */
        PAD_CFG_NF(GPP_A16, NONE, DEEP, NF1),
	/* 17: SSD_PEWAKE(SSD Module) */
	PAD_CFG_GPO(GPP_A17, 1, PLTRST),
	/* 18: HPD_2 pins */
	PAD_CFG_NF(GPP_A18, NONE, DEEP, NF1),
	/* 19 : TCP0_RD_HPDIN */
        PAD_CFG_NF(GPP_A19, NONE, DEEP, NF1),
	/* 20: NC */
        PAD_NC(GPP_A20, NONE),
	/* 22: NC */
	PAD_NC(GPP_A22, NONE),
	/* 23: NC */
	PAD_NC(GPP_A23, NONE),

        //B Series
	/* 0: GPPC_B0_CORE_VID_0 / VCCIN AUX VR*/
        PAD_CFG_NF(GPP_B0, NONE, DEEP, NF1),
        /* 1: GPPC_B1_CORE_VID_1 / VCCIN AUX VR*/
        PAD_CFG_NF(GPP_B1, NONE, DEEP, NF1),
        /* 2: PEG Slot RST# & Bttry chrgr PCH PROCHOT*/
        PAD_CFG_GPO(GPP_B2, 1, PLTRST),
	/* 3: NC */
        PAD_NC(GPP_B3, NONE),
	/* 4: NC */
	PAD_NC(GPP_B4, NONE),
	/* 5: NC */
	PAD_NC(GPP_B5, NONE),
	/* 6: NC */
	PAD_NC(GPP_B6, NONE),
	/* 7: NC */
	PAD_NC(GPP_B7, NONE),
	/* 8: NC */
	PAD_NC(GPP_B8, NONE),
        /* 11: I2C_PMC_PD_INT_N*/
        PAD_CFG_NF(GPP_B11, NONE, DEEP, NF1),
        /* 12: PM_SLP_S0_N*/
        PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
        /* 13: PLT_RST_N*/
        PAD_CFG_NF(GPP_B13, NONE, PLTRST, NF1),
	/* 14: CRD_PRIVACY_LED */
        PAD_CFG_GPO(GPP_B14, 0, PLTRST),
	/* 15: SSD_ALERT (SSD) */
	PAD_CFG_GPO(GPP_B15, 1, PLTRST),
	/* 16: NC */
	PAD_NC(GPP_B16, NONE),
        /* 17: NC */
	PAD_NC(GPP_B17, NONE),
	/* 18: CAM_STROBE */
        //PAD_CFG_GPO(GPP_B18, 0, PLTRST),
	PAD_NC(GPP_B18, NONE), 
	/* 23: CRD1_PWREN_CVF_IRQ */
        //PAD_CFG_GPO(GPP_B23, 0, PLTRST),
	PAD_NC(GPP_B23, NONE), 
   
        //C Series
	/* 0: SMB_CLK */
        PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
        /* 1: SMB_DATA */
        PAD_CFG_NF(GPP_C1, NONE, DEEP, NF1),
	/* 2: M.2_CPU SSD2 PWREN */
        //PAD_CFG_GPO(GPP_C2, 1, PLTRST),
	PAD_NC(GPP_C2, NONE), 
	/* 3: NC */
        PAD_NC(GPP_C3, NONE),
	/* 4: NC */
	PAD_NC(GPP_C4, NONE), 
	/* 6: SML1_CLK_USBC_PD */
        PAD_CFG_NF(GPP_C6, NONE, DEEP, NF1),
	/* SML1_DATA_USBC_PD */
        PAD_CFG_NF(GPP_C7, NONE, DEEP, NF1),

	//D Series
	/* 0: NC */
        PAD_NC(GPP_D0, NONE),
	/* 1: NC */
	PAD_NC(GPP_D1, NONE),
	/* 2: NC */
        PAD_NC(GPP_D2, NONE),
	/* 4: NC */
        PAD_NC(GPP_D4, NONE),
	/* 5: NC */
        PAD_NC(GPP_D5, NONE),
	/* 6: NC */
        PAD_NC(GPP_D6, NONE),
        /* 7: GPIO pin for PCIE WIFI CLKREQ */
        PAD_CFG_NF(GPP_D7, NONE, DEEP, NF1),
	/* 8: NC */
        PAD_NC(GPP_D8, NONE),
	/* 9: NC */
	PAD_NC(GPP_D9, NONE),
	/* 10: TBT_LSX2_RXD */
        PAD_CFG_GPO(GPP_D10, 0, DEEP),
	/* 11: NC */
	PAD_NC(GPP_D11, NONE),
	/* 12: TBT_LSX3_RXD */
        PAD_CFG_GPO(GPP_D12, 0, DEEP),
        /* 13: WIFI_WAKE_N */
        PAD_CFG_GPI_IRQ_WAKE(GPP_D13, NONE, DEEP, LEVEL, INVERT),
        /* 14: SSD1_PWREN  CPU SSD1 */
        PAD_CFG_GPO(GPP_D14, 1, DEEP),
	/* 15: NC */
        PAD_NC(GPP_D15, NONE),
	/* 16: M.2_PCH_SSD_PWREN  */
	//PAD_CFG_GPO(GPP_D16, 1, PLTRST), 
	PAD_NC(GPP_D16, NONE), 
	/* 17: NC */
	PAD_NC(GPP_D17, NONE),
	/* 18: NC */
	PAD_NC(GPP_D18, NONE),
	/* 19: CAM_LS_EN */
	PAD_CFG_GPO(GPP_D19, 1, DEEP),


        //E Series
	/* 0: UART_BT_WAKE_N */
        PAD_CFG_GPI_IRQ_WAKE(GPP_E0, NONE, DEEP, LEVEL, INVERT),
	/* 1: NC */
        PAD_NC(GPP_E1, NONE),
	/* 2: NC */
	PAD_NC(GPP_E2, NONE),
	/* 3 : SPI_TPM_INT_N */
  //  PAD_CFG_GPI_APIC(GPP_E3, NONE, PLTRST, LEVEL, INVERT),
	PAD_CFG_GPI_INT(GPP_E3, NONE, DEEP, LEVEL),
	/* 4: NC */
	PAD_NC(GPP_E4, NONE),
	/* 5: NC */
	PAD_NC(GPP_E5, NONE),
	/* 6: THC0_SPI1_RST_N_TCH_PNL1 */
        PAD_CFG_GPO(GPP_E6, 1, PLTRST),
	/* 7: EC_SMI_N */
	PAD_CFG_GPI_SMI(GPP_E7, NONE, DEEP, EDGE_SINGLE, INVERT),
//	PAD_CFG_GPI_SMI(GPP_E7, NONE, PLTRST, EDGE_SINGLE, NONE),
	/* 8: NC */
	PAD_NC(GPP_E8, NONE),
	/* 9: NC */
	PAD_NC(GPP_E9, NONE),
	/* 10: NC */
	PAD_NC(GPP_E10, NONE),
	/* 11: NC */
	PAD_NC(GPP_E11, NONE),
	/* 12: TRACKPAD_INT*/
        PAD_CFG_GPI_INT(GPP_E12, NONE, PLTRST, LEVEL),
	/* 13: NC */
	PAD_NC(GPP_E13, NONE),
        /* 14: HPD_1 (E14) */
        PAD_CFG_NF(GPP_E14, NONE, DEEP, NF1),
	/* 15: NC */
        PAD_NC(GPP_E15, NONE),
	/* 16: NC */
	PAD_NC(GPP_E16, NONE),
	/* 17: NC */
	PAD_NC(GPP_E17, NONE),
	/* 18 : USB_3.0_PWR_EN_P1  */
        PAD_CFG_GPO(GPP_E18, 1, DEEP),
	/* 19: TBT_LSX0_RXD */
        //PAD_CFG_GPO(GPP_E19, 0, DEEP),
	PAD_NC(GPP_E19, NONE), 
	/* 20 : USB_3.0_PWR_EN_P2  */
        PAD_CFG_GPO(GPP_E20, 1, DEEP),
	/* 21: TBT_LSX1_RXD */
        //PAD_CFG_GPO(GPP_E21, 0, DEEP),
	PAD_NC(GPP_E21, NONE), 
	/* 22 : USB_2.0_PWR_EN  */
        PAD_CFG_GPO(GPP_E22, 1, DEEP),
        /* 23: SOC_eDP_CONN_CLR_EN */
        PAD_CFG_GPO(GPP_E23, 1, PLTRST),

        //F Series
        /* 0: CNV_BRI_DT_BT_UART2_RTS_N */
        PAD_CFG_NF(GPP_F0, NONE, DEEP, NF1),
        /* 1: CNV_BRI_RSP_BT_UART2_RXD */
        PAD_CFG_NF(GPP_F1, NONE, DEEP, NF1),
        /* 2: CNV_RGI_DT_BT_UART2_TXD */
        PAD_CFG_NF(GPP_F2, NONE, DEEP, NF1),
        /* 3: CNV_RGI_RSP_BT_UART2_CTS_N */
        PAD_CFG_NF(GPP_F3, NONE, DEEP, NF1),
        /* 4: CNV_RF_RESET_R_N */
        PAD_CFG_NF(GPP_F4, NONE, DEEP, NF1),
        /* 5: MODEM_CLKREQ_R */
        PAD_CFG_NF(GPP_F5, NONE, DEEP, NF1),
        /* 6: WLAN_COEX3 */
        PAD_CFG_NF(GPP_F6, NONE, DEEP, NF1),
        /* 7: NC */	
	PAD_NC(GPP_F7, NONE),
        /* 9: EC_SLP_S0_CS_N */
        PAD_CFG_GPO(GPP_F9, 1, PLTRST),
	/* 10: x4_SLOT_RESET (PCIE) */
        PAD_CFG_GPO(GPP_F10, 0, PLTRST),
	/* 11: NC */
        PAD_NC(GPP_F11, NONE),
	/* 12: NC */
	PAD_NC(GPP_F12, NONE),
	/* 13: NC */
	PAD_NC(GPP_F13, NONE),
	/* 14 : WiFi_RF_KILL */
        PAD_CFG_GPO(GPP_F14, 1, PLTRST),
	/* 15: NC */
	PAD_NC(GPP_F15, NONE),
	/* 16: NC */
	PAD_NC(GPP_F16, NONE),
	/* 17 : SD_CTRL_RST# */
        PAD_CFG_GPO(GPP_F17, 1, PLTRST),
	/* 18: NC */
	PAD_NC(GPP_F18, NONE),
        /* 20: SSD1_RESET  CPU SSD1 */
        PAD_CFG_GPO(GPP_F20, 1, DEEP),
	/* 21: NC */
	PAD_NC(GPP_F21, NONE),
        /* 22: F22 : VNN CTRL */
        PAD_CFG_NF(GPP_F22, NONE, DEEP, NF1),
        /* 23: F23 :  V1P05 CTRL */
        PAD_CFG_NF(GPP_F23, NONE, DEEP, NF1),

	//H Series
	/* 0: M2_SSD_RST_N */
        PAD_CFG_GPO(GPP_H0, 1, PLTRST),
        /* 1: M2_CPU_SSD2_RESET_N */
        PAD_CFG_GPO(GPP_H1, 1, PLTRST),
        /* 2: WLAN RST# */
        PAD_CFG_GPO(GPP_H2, 1, PLTRST),
	/* 3: NC */
	PAD_NC(GPP_H3, NONE),
        /* 4: I2C_SDA(0) */
        PAD_CFG_NF(GPP_H4, NONE, DEEP, NF1),
	/* 5: I2C_SCL(0) */
        PAD_CFG_NF(GPP_H5, NONE, DEEP, NF1),
	/* 6: NC */
	PAD_NC(GPP_H6, NONE),
	/* 7: NC */
	PAD_NC(GPP_H7, NONE),
        /* 8: WLAN_COEX1 */
        PAD_CFG_NF(GPP_H8, NONE, DEEP, NF2),
        /* 9: WLAN_COEX2 */
        PAD_CFG_NF(GPP_H9, NONE, DEEP, NF2),
	/* 12: NC */
	PAD_NC(GPP_H12, NONE),
	/* 13: NC */
	//PAD_CFG_NF(GPP_H13, NONE, DEEP, NF5),
	PAD_NC(GPP_H13, NONE), 
        /* 15: DDPB_CTRLCLK ==> DDIB_HDMI_CTRLCLK */
        PAD_CFG_NF(GPP_H15, NONE, DEEP, NF1),
	/* 17: DDPB_CTRLDATA ==> DDIB_HDMI_CTRLDATA */
        PAD_CFG_NF(GPP_H17, NONE, DEEP, NF1),
	/* 18: CPU_C10_GATE_N_R */
        PAD_CFG_NF(GPP_H18, NONE, DEEP, NF1),
	/* 19: CPU SSD CLKREQ# */
        PAD_CFG_NF(GPP_H19, NONE, DEEP, NF1),
	/* 20: NC */
        PAD_NC(GPP_H20, NONE),
	/* 21: NC */
	PAD_NC(GPP_H21, NONE),
	/* 22: NC */
	PAD_NC(GPP_H22, NONE),
	/* 23: NC */
	PAD_NC(GPP_H23, NONE),

        //R Series
	/* 0: I2S0_SCLK */
        PAD_CFG_NF(GPP_R0, NONE, DEEP, NF1),
        /* 1: I2S0_SFRM */
        PAD_CFG_NF(GPP_R1, NONE, DEEP, NF1), //vvdn
        /* 2: I2S0_TXD */
        PAD_CFG_NF(GPP_R2, NONE, DEEP, NF1), //vvdn
        /* 3: I2S0_RXD */
        PAD_CFG_NF(GPP_R3, NONE, DEEP, NF1), //vvdn
	/* 4: I2S2_SCLK*/
        PAD_CFG_NF(GPP_R4, NONE, DEEP, NF1),
	/* 5: NC */
        PAD_NC(GPP_R5, NONE),
	/* 6: NC */
	PAD_NC(GPP_R6, NONE),
	/* 7: NC */
	PAD_NC(GPP_R7, NONE),

        //S Series
	/* 0 : WLAN_BT_PCMCLK */
        PAD_CFG_NF(GPP_S0, NONE, DEEP, NF2),
        /* 1 :  WLAN_PCM_SYNC  */
        PAD_CFG_NF(GPP_S1, NATIVE, DEEP, NF2),
        /* 2 : WLAN_BT_PCM_OUT  */
        PAD_CFG_NF(GPP_S2, NATIVE, DEEP, NF2),
        /* 3 : WLAN_BT_PCM_IN */
        PAD_CFG_NF(GPP_S3, NATIVE, DEEP, NF2),
	/* 4: NC */
	PAD_NC(GPP_S4, NONE),
	/* 5: NC */
	PAD_NC(GPP_S5, NONE),
        /* 6: SOC_DMIC0_SNDW3_CLK */
        PAD_CFG_NF(GPP_S6, NONE, DEEP, NF1), //vvdn
        /* 7: SOC_DMIC0_SNDW3_DATA */
        PAD_CFG_NF(GPP_S7, NONE, DEEP, NF1), //vvdn 

	//GPD Series
	/* 0: GPD_0_BATLOW_N  */
        PAD_CFG_NF(GPD0, NONE, PWROK, NF1),
        /* 1: BC_ACOK_DSW  */
        PAD_CFG_NF(GPD1, NONE, PWROK, NF1),
        /* 3: GPD_3_PWRBTN_N  */
        PAD_CFG_NF(GPD3, NONE, PWROK, NF1),
        /* 4: GPD_4_SLP_S3_N  */
        PAD_CFG_NF(GPD4, NONE, PWROK, NF1),
        /* 5: GPD_5_SLP_S4_N */
        PAD_CFG_NF(GPD5, NONE, PWROK, NF1),
        /* 6: GPD_6_SLP_A_N */
        PAD_CFG_NF(GPD6, NONE, PWROK, NF1),
        /* 8: M.2_BTWIFI_SUS_CLK */
        PAD_CFG_NF(GPD8, NONE, PWROK, NF1),
        /* 9: GPD_9_SLP_WLAN_N */
        PAD_CFG_NF(GPD9, NONE, PWROK, NF1),
        /* 10: GPD_10_SLP_S5_N */
        PAD_CFG_NF(GPD10, NONE, PWROK, NF1),
	/* 11: NC */
	PAD_NC(GPD11, NONE),

};
void variant_configure_gpio_pads(void)
{
        gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}

static const struct cros_gpio cros_gpios[] = {
        CROS_GPIO_REC_AL(CROS_GPIO_VIRTUAL, CROS_GPIO_DEVICE_NAME),
};
DECLARE_CROS_GPIOS(cros_gpios);


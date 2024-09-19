/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef EC_INTEL_ADLP_SMM_H
#define EC_INTEL_ADLP_SMM_H

/* Board/FAB ID Command */
#define EC_FAB_ID_CMD	0x0d
/* Bit 5:0 for Board ID */
#define BOARD_ID_MASK	0x3f

/*
 * Returns board information (board id[15:8] and
 * Fab info[7:0]) on success and < 0 on error
 */
int get_rvp_board_id(void);

#endif /* EC_INTEL_ADLP_SMM_H */

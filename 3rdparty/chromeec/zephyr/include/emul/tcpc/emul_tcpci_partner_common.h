/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * @file
 *
 * @brief Common code used by TCPCI partner device emulators
 */

#ifndef __EMUL_TCPCI_PARTNER_COMMON_H
#define __EMUL_TCPCI_PARTNER_COMMON_H

#include <emul.h>
#include "emul/tcpc/emul_tcpci.h"

#include "ec_commands.h"
#include "usb_pd.h"

/**
 * @brief Common code used by TCPCI partner device emulators
 * @defgroup tcpci_partner Common code for TCPCI partner device emulators
 * @{
 *
 * Common code for TCPCI partner device emulators allows to send SOP messages
 * in generic way using optional delay.
 */

/** Common data for TCPCI partner device emulators */
struct tcpci_partner_data {
	/** Work used to send message with delay */
	struct k_work_delayable delayed_send;
	/** Pointer to connected TCPCI emulator */
	const struct emul *tcpci_emul;
	/** Queue for delayed messages */
	sys_slist_t to_send;
	/** Mutex for to_send queue */
	struct k_mutex to_send_mutex;
	/** Next SOP message id */
	int msg_id;
	/** Power role (used in message header) */
	enum pd_power_role power_role;
	/** Data role (used in message header) */
	enum pd_data_role data_role;
	/** Revision (used in message header) */
	enum pd_rev_type rev;
};

/** Structure of message used by TCPCI partner emulator */
struct tcpci_partner_msg {
	/** Reserved for sys_slist_* usage */
	sys_snode_t node;
	/** TCPCI emulator message */
	struct tcpci_emul_msg msg;
	/** Time when message should be sent if message is delayed */
	uint64_t time;
	/** Type of the message */
	int type;
	/** Number of data objects */
	int data_objects;
};

/**
 * @brief Initialise common TCPCI partner emulator. Need to be called before
 *        any other function.
 *
 * @param data Pointer to USB-C charger emulator
 */
void tcpci_partner_init(struct tcpci_partner_data *data);

/**
 * @brief Allocate message with space for header and given number of data
 *        objects. Type of message is set to TCPCI_MSG_SOP by default.
 *
 * @param data_objects Number of data objects in message
 *
 * @return Pointer to new message on success
 * @return NULL on error
 */
struct tcpci_partner_msg *tcpci_partner_alloc_msg(int data_objects);

/**
 * @brief Free message's memory
 *
 * @param msg Pointer to message
 */
void tcpci_partner_free_msg(struct tcpci_partner_msg *msg);

/**
 * @brief Set header of the message
 *
 * @param data Pointer to TCPCI partner emulator
 * @param msg Pointer to message
 */
void tcpci_partner_set_header(struct tcpci_partner_data *data,
			      struct tcpci_partner_msg *msg);

/**
 * @brief Send message to TCPCI emulator or schedule message. On error message
 *        is freed.
 *
 * @param data Pointer to TCPCI partner emulator
 * @param msg Pointer to message to send
 * @param delay Optional delay
 *
 * @return 0 on success
 * @return negative on failure
 */
int tcpci_partner_send_msg(struct tcpci_partner_data *data,
			   struct tcpci_partner_msg *msg, uint64_t delay);

/**
 * @brief Send control message with optional delay
 *
 * @param data Pointer to TCPCI partner emulator
 * @param type Type of message
 * @param delay Optional delay
 *
 * @return 0 on success
 * @return -ENOMEM when there is no free memory for message
 * @return negative on failure
 */
int tcpci_partner_send_control_msg(struct tcpci_partner_data *data,
				   enum pd_ctrl_msg_type type,
				   uint64_t delay);

/**
 * @brief Send data message with optional delay. Data objects are copied to
 *        message.
 *
 * @param data Pointer to TCPCI partner emulator
 * @param type Type of message
 * @param data_obj Pointer to array of data objects
 * @param data_obj_num Number of data objects
 * @param delay Optional delay
 *
 * @return 0 on success
 * @return -ENOMEM when there is no free memory for message
 * @return negative on failure
 */
int tcpci_partner_send_data_msg(struct tcpci_partner_data *data,
				enum pd_data_msg_type type,
				uint32_t *data_obj, int data_obj_num,
				uint64_t delay);

/**
 * @brief Remove all messages that are in delayed message queue
 *
 * @param data Pointer to TCPCI partner emulator
 *
 * @return 0 on success
 * @return negative on failure
 */
int tcpci_partner_clear_msg_queue(struct tcpci_partner_data *data);

/**
 * @}
 */

#endif /* __EMUL_TCPCI_PARTNER_COMMON_H */

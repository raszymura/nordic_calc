/*
 * Rafal Szymura
 * June 2024
 * BLE Calculator Application
 */

/** @file
 *  @brief Calculator Data Service (CDS)
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>  // GATT (Generic Attribute Profile) header
#include "my_cds.h"

// MODES:
#define FLOAT_MODE 0  // 32-bit floating-point mode
#define FIXED_MODE 1  // Q31 fixed-point mode

LOG_MODULE_DECLARE(BLE_Calculator_App);

// -----------------------------------------------------------------------------------------------
static bool notify_result_enabled;
static struct my_cds_cb  cds_cb;

#pragma pack(push, 1) // Preserve current packing settings and set packing to 1 byte
struct calculator_task {		// Define a structure for calculator tasks
    uint8_t operation;			// Operation to be performed (e.g., add, subtract)
    union {						// Union for operands, allowing either float or fixed-point integer.
        float f_operand;		// 32-bit floating-point operand
        int32_t q31_operand;	// Fixed-point (Q31) operand
    };
    bool mode;				// Mode: floating-point (0) or fixed-point (1)
};
#pragma pack(pop) // Restore original packing settings after defining the structure
// -----------------------------------------------------------------------------------------------

// Define the configuration change callback function for the result characteristic
static void mycdsbc_ccc_result_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_result_enabled = (value == BT_GATT_CCC_NOTIFY);  // Check if notifications are enabled
}

static ssize_t write_operation(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{	
    LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (len != sizeof(struct calculator_task)) { 
		LOG_DBG("Write operation: Incorrect data length");
		printk("Write operation: Incorrect data length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write operation: Incorrect data offset");
		printk("Write operation: Incorrect data offset\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	// Cast the buffer to the calculator_task struct
    struct calculator_task *task = (struct calculator_task *)buf;

    //memcpy(&task, buf, sizeof(task));  // Copy the received data into the task variable
	// Display the contents of the struct
    printk("Operation: %u\n", task->operation);
    if (task->mode) {
        printk("Q31 Operand: %d\n", task->q31_operand);
    } else {
        printk("Float Operand: %f\n", task->f_operand);
    }
    printk("Mode: %u\n", task->mode);
    //k_msgq_put(&calculator_msgq, &task, K_NO_WAIT);  // Put the task into the message queue

	// if (cds_cb.operation_cb) {
	// 	uint8_t val = *((uint8_t *)buf);  // Read the received value
	// 	// uint8_t val = *((uint8_t *)task.mode);  // Read the received value

	// 	if (val == 0x00 || val == 0x01) {
	// 		// Call the application callback function to update the operation state
	// 		cds_cb.operation_cb(val ? true : false);
	// 	} else {
	// 		LOG_DBG("Write operation: Incorrect value");
	// 		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	// 	}
	// }

	return len;  // Return the length of the received data
}

// GATT Calculator Data Service (CDS) Declaration --------------------------------------------------
BT_GATT_SERVICE_DEFINE(  // Statically add the service to the attributes table of our board (the GATT server) 
	my_cds_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_CDS),  // Primary service with a custom UUID
	BT_GATT_CHARACTERISTIC(BT_UUID_CDS_OPERATION, BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_WRITE, NULL, write_operation, NULL), // Writing operations Characteristic
	BT_GATT_CHARACTERISTIC(BT_UUID_CDS_RESULT, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_NONE, NULL, NULL, NULL),  // Notify result Characteristic
	BT_GATT_CCC(mycdsbc_ccc_result_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

// Register application callbacks for the CDS characteristics -----------------------------------------
int my_cds_init(struct my_cds_cb *callbacks)
{
	if (callbacks) {
		cds_cb.operation_cb = callbacks->operation_cb;
		// calc_cb.result_cb = callbacks->result_cb;
	}

	return 0;
}

// Function to send notifications for the result characteristic
int my_cds_send_result_notify(uint32_t result_value)
{
	if (!notify_result_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_cds_svc.attrs[4], &result_value, sizeof(result_value));
}
// ---------------------------------------------------------------------------------------------------
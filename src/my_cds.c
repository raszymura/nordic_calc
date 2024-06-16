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

LOG_MODULE_DECLARE(BLE_Calculator_App);

// -------------------------------------------------------------------------------------------------
static bool notify_result_enabled;
static struct my_cds_cb  cds_cb;
// -------------------------------------------------------------------------------------------------
extern struct k_sem result_sem;  // Semaphor
extern struct k_msgq calculator_msgq;  // Message queue
// -------------------------------------------------------------------------------------------------

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
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write operation: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	// Cast the buffer to the calculator_task struct
    struct calculator_task *task = (struct calculator_task *)buf;  // Read the received value

	// struct calculator_task *task;
    // memcpy(task, buf, sizeof(task));  // Copy the received data into the task variable

/*	// Display the contents of the struct
	printk("Operation: %u\n", task->operation);
	if (task->mode) {
		printk("Float Operand 1: %f\n", task.f_operand_1);
		printk("Float Operand 2: %f\n", task.f_operand_2);
	} else {
		printk("Float Operand: %f\n", task->f_operand);
	}
	printk("Mode: %u\n", task->mode);
	printk("----------------\n");
*/
    k_msgq_put(&calculator_msgq, task, K_NO_WAIT);  // Put the task into the message queue

	// LED mode indicator: LED on: FIXED_MODE, LED off: FLOAT_MODE
	if (cds_cb.mode_cb) {
		if (task->mode == FLOAT_MODE || task->mode == FIXED_MODE) {
			// Call the application callback function to update the mode state
			cds_cb.mode_cb(task->mode ? true : false);  // LED on when FIXED_MODE
		} else {
			LOG_DBG("Write mode: Incorrect value");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

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

// Register application callbacks for the CDS characteristics --------------------------------------
int my_cds_init(struct my_cds_cb *callbacks)
{
	if (callbacks) {
		cds_cb.mode_cb = callbacks->mode_cb;
	}

	return 0;
}


// Thread functions --------------------------------------------------------------------------------
// Function to send notifications for the result characteristic (send_data_thread) -----------------
int my_cds_send_result_notify(ReturnValue result_value)
{
	if (!notify_result_enabled) {
		return -EACCES;
	}
	printk("...notifying...");
	if (result_value.type == FLOAT_TYPE) {
		float result_f = result_value.value.f;
		printk("Result = %f\n\n", result_value.value.f);
		return bt_gatt_notify(NULL, &my_cds_svc.attrs[4], &result_f, sizeof(result_f));
	} else if (result_value.type == INT32_TYPE) {
		int32_t result_u = result_value.value.u;
		printk("Result = %d\n\n", result_value.value.u);
		return bt_gatt_notify(NULL, &my_cds_svc.attrs[4], &result_u, sizeof(result_u));
	}
	return -1;
}
// -------------------------------------------------------------------------------------------------

// Function to calculate the equation result (calculator_engine_thread) ----------------------------
ReturnValue my_cds_calculate_result(struct calculator_task task)
{  
	// Display the contents of the struct
    printk("Operation in calculator thread: %u\n", task.operation);
	if (task.mode == FLOAT_MODE) {
		printk("Float Operand 1: %f\n", task.f_operand_1);
		printk("Float Operand 2: %f\n", task.f_operand_2);
		printk("FLOAT_MODE\n");
    } else { // FIXED_MODE
		printk("Q31 Operand 1: %d\n", task.q31_operand_1);
		printk("Q31 Operand 2: %d\n", task.q31_operand_2);
		printk("FIXED_MODE\n");
	}
	printk("----------------\n");

	float result_f = 0.0f;	// Initialize the floating-point result to 0
	int32_t result_q31 = 0;	// Initialize the fixed-point result to 0

	ReturnValue result;

// TODO: Add FPU usage: 
// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fhardware_driver_fpu.html
// https://devzone.nordicsemi.com/search?q=FPU
// https://docs.nordicsemi.com/search?q=fpu
	if (task.mode == FLOAT_MODE) { 
		switch (task.operation) {
			case 0: // Reset
				result_f = 0.0f;
				break;
			case 1: // Add
				result_f = task.f_operand_1 + task.f_operand_2;
				break;
			case 2: // Subtract
				result_f = task.f_operand_1 - task.f_operand_2;
				break;
			case 3: // Multiply
				result_f = task.f_operand_1 * task.f_operand_2;
				break;
			case 4: // Divide
				if (task.f_operand_2 > EPSILON || task.f_operand_2 < -EPSILON) { // Division by zero, also checked in TEST TOOL python app
					result_f = task.f_operand_1 / task.f_operand_2;
				} else {
					printk("Error: Division by zero.");
					result_f = 0.0f;
				}
				break;
			default:
				break;

			//  if (isinf(result_f) || isnan(result_f)){
			// 	printk("Float Overflow!");
			// 	result_q31 = 0.0f;
			//  }
		}
	} else { // FIXED_MODE - https://en.wikipedia.org/wiki/Q_(number_format)
		switch (task.operation) {
			case 0: // Reset
				result_q31 = 0;
				break;
			case 1: // Add
				result_q31 = task.q31_operand_1 + task.q31_operand_2;
				if (((task.q31_operand_2 > 0) && (task.q31_operand_1 > INT_MAX - task.q31_operand_2)) ||
					((task.q31_operand_2 < 0) && (task.q31_operand_1 < INT_MIN - task.q31_operand_2))) {
					printk("Integer Overflow in addition!");
					result_q31 = 0;
				}
				break;
			case 2: // Subtract
				result_q31 = task.q31_operand_1 - task.q31_operand_2;
				if (((task.q31_operand_2 > 0) && (task.q31_operand_1 < INT_MIN + task.q31_operand_2)) ||
					((task.q31_operand_2 < 0) && (task.q31_operand_1 > INT_MAX + task.q31_operand_2))) {
					printk("Integer Overflow in subtraction!");
					result_q31 = 0;
				}
				break;
			case 3: // Multiply
				int64_t result_q62 = (int64_t)task.q31_operand_1 * (int64_t)task.q31_operand_2;  // Convert 'q31_operand' to Q62 format (prevent overflow)
				result_q62 = (result_q62 >> 31);

				if ((result_q62 > INT32_MAX) || (result_q62 < INT32_MIN)) {
					printk("Integer Overflow in multiplication!");
				 	result_q62 = 0;
				}
				result_q31 = (int32_t )result_q62;  // Convert result back to Q31 format
				break;
			case 4: // Divide
				if (task.q31_operand_2 > EPSILON || task.q31_operand_2 < -EPSILON) {  // Division by zero, also checked in TEST TOOL python app
					result_q31 = q_div(task.q31_operand_1, task.q31_operand_2);
				} else {
					printk("Error: Division by zero.");
					result_q62 = 0;
				}
				break;
			default:
				break;
		}
	}

    // Notify result
    if (task.mode == FLOAT_MODE) {
		result.value.f = result_f;
		result.type = FLOAT_TYPE;
    } else { // FIXED_MODE
		result.value.u = result_q31;
		result.type = INT32_TYPE;
    }

	return result;
}
// -------------------------------------------------------------------------------------------------

int32_t q_div(int32_t a, int32_t b)  // TODO: Not working properly yet
{
    int64_t temp = (int64_t)a << 31;  // pre-multiply by the base 
    // Rounding: mid values are rounded up (down for negative values).
    // OR compare most significant bits i.e. if (((temp >> 31) & 1) == ((b >> 15) & 1))
    if ((temp >= 0 && b >= 0) || (temp < 0 && b < 0)) {   
        temp += b / 2;    // OR shift 1 bit i.e. temp += (b >> 1);
    } else {
        temp -= b / 2;    // OR shift 1 bit i.e. temp -= (b >> 1);
    }
	temp = (temp / b);
	if ((temp > INT32_MAX) || (temp < INT32_MIN)) {
		printk("Integer Overflow in division!");
		temp = 0;
	}
    return (int32_t)temp;
}

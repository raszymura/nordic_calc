/*
 * Rafal Szymura
 * June 2024
 * BLE Calculator Application
 */

/*
 * Two custom characteristics for the service handling, binary data exchange:
 * (1) Write: For sending data (arguments and operations) to the board 
 * (2) Notify: For receiving data (result of the operation) from the board
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h> // Header file of the Bluetooth LE stack
#include <zephyr/bluetooth/gap.h>       // Header file of the Bluetooth LE stack
#include <zephyr/bluetooth/uuid.h>  	// Header file of the UUID helper macros and definitions
#include <zephyr/bluetooth/conn.h>  	// Header file for managing Bluetooth LE Connections
#include <dk_buttons_and_leds.h>    	// Header file for buttons and LEDs on a Nordic devkit
#include "my_cds.h"    					// Header file of custom Calculator Data Service

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	BT_LE_ADV_OPT_USE_IDENTITY), // Connectable advertising and use identity address
	800, // Min Advertising Interval 500ms (800*0.625ms)
	801, // Max Advertising Interval 500.625ms (801*0.625ms)
	NULL); // Set to NULL for undirected advertising

LOG_MODULE_REGISTER(BLE_Calculator_App, LOG_LEVEL_INF);

// -------------------------------------------------------------------------------------------------
K_SEM_DEFINE(result_sem, 0, 1);  // Semaphor
extern struct k_sem result_sem;
// -------------------------------------------------------------------------------------------------
#define CALC_MSGQ_MAX_MSGS 10  // Message queue
#define CALC_MSGQ_MSG_SIZE sizeof(struct calculator_task)
K_MSGQ_DEFINE(calculator_msgq, CALC_MSGQ_MSG_SIZE, CALC_MSGQ_MAX_MSGS, 4);
extern struct k_msgq calculator_msgq;
// -------------------------------------------------------------------------------------------------

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define USER_LED DK_LED3

#define STACKSIZE 1024
#define SEND_DATA_PRIORITY 7
#define CALCULATOR_ENGINE_PRIORITY 7
// #define RECEIVE_DATA_PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000

//  Manufacturer Specific Data (Additional advertisement data) -------------------------------------
#define COMPANY_ID_CODE 0x0059  // Company identifier (Company ID)
// Declare the structure for custom data
typedef struct adv_mfg_data {
	uint16_t company_code;  		// Company Identifier Code
	uint16_t seconds_since_reset;	// Number of seconds since reset
} adv_mfg_data_type;
// Define and initialize a variable of type adv_mfg_data_type
static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, 0x00 };
// -------------------------------------------------------------------------------------------------

static uint32_t app_result_value = 0;  // Data to stream over BLE

// Create the advertising parameter for connectable advertising ------------------------------------
static const struct bt_data ad[] = {
	// Set the flags and populate the device name in the advertising packet
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	// Manufacturer Specific Data in the advertising packet
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

static const struct bt_data sd[] = {
	// Include the 16-bytes (128-Bits) UUID of the CDS service in the scan response packet
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CDS_VAL),
};
// -------------------------------------------------------------------------------------------------

static void app_mode_cb(bool mode_state) // Mode (float or fixed) LED indicator
{
	dk_set_led(USER_LED, mode_state);
}

static struct my_cds_cb app_callbacks = {
	.mode_cb = app_mode_cb,
};


// ----------- Connection Callback functions -------------------------------------------------------
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}
	printk("** Connected! **\n");
	dk_set_led_on(CON_STATUS_LED);  // Turn the connection status LED on
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("** Disconnected (reason %u) **\n", reason);
	dk_set_led_off(CON_STATUS_LED);  // Turn the connection status LED off
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};
// ----------- END: Connection Callback functions --------------------------------------------------


// Thread functions --------------------------------------------------------------------------------
void send_data_thread(void)
{
    while (1) {
        k_sem_take(&result_sem, K_FOREVER);  // Wait for the semaphore
        printk("...sending...\n\n");
        int err = my_cds_send_result_notify(app_result_value);
        if (err) {
            LOG_ERR("Failed to send notification (err %d)\n", err);
        }
    }
}

void calculator_engine_thread(void)
{
    struct calculator_task task;

    while (1) {
        k_msgq_get(&calculator_msgq, &task, K_FOREVER);  // Get the task from the message queue

        // Perform calculations based on the task
        app_result_value = my_cds_calculate_result(task);  // TODO: Zamień na rzeczywiste obliczenie
        // if (task.mode == FLOAT_MODE) {
        //     // Example calculation for float mode
        //     app_result_value = my_cds_calculate_result(task);  // TODO: Zamień na rzeczywiste obliczenie
        // } else if (task.mode == FIXED_MODE) {
        //     // Example calculation for fixed-point mode
        //     app_result_value = my_cds_calculate_result(task);  // Replace with actual calculation
        // }
        k_sem_give(&result_sem);  // Set semaphore to notify the send_data_thread that the result is ready
    }
}
// -------------------------------------------------------------------------------------------------



// -------------------------------------------------------------------------------------------------
int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Nordic Calculator\n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks);  // Register connection callbacks
	// Pass application callback functions stored in app_callbacks to the Calculator Service
	err = my_cds_init(&app_callbacks);
	if (err) {
		printk("Failed to init CDS (err:%d)\n", err);
		return -1;
	}
	
	LOG_INF("Bluetooth initialized\n");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return -1;
	}
	LOG_INF("Advertising successfully started\n");

	while (1) {
		// Update the advertising data dynamically
		adv_mfg_data.seconds_since_reset = k_uptime_get() / 1000;  // Update number of seconds since reset
		bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));  // Update adv

		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
// ------- END: int main(void) ---------------------------------------------------------------------


// ------- THREADS ---------------------------------------------------------------------------------
// Define and initialize the 'Send Data' thread
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL,
					NULL, SEND_DATA_PRIORITY, 0, 0);

// Define and initialize the 'Calculator Engine' thread
K_THREAD_DEFINE(calculator_engine_thread_id, STACKSIZE, calculator_engine_thread, NULL, NULL,
                NULL, CALCULATOR_ENGINE_PRIORITY, 0, 0);

// Define and initialize the 'Receive Data' thread
// K_THREAD_DEFINE(receive_data_thread_id, STACKSIZE, receive_data_thread, NULL, NULL,
//                 NULL, RECEIVE_DATA_PRIORITY, 0, 0);
// -------END: THREADS -----------------------------------------------------------------------------
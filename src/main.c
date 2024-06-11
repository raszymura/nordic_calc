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
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	800, /* Min Advertising Interval 500ms (800*0.625ms) */
	801, /* Max Advertising Interval 500.625ms (801*0.625ms) */
	NULL); /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(BLE_Calculator_App, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define USER_LED DK_LED3

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000
#define NOTIFY_INTERVAL 500	/* STEP 17 - Define the interval at which you want to send data at */

//  Manufacturer Specific Data (Additional advertisement data)------------------------
#define COMPANY_ID_CODE 0x0059  // Company identifier (Company ID)
// Declare the structure for custom data
typedef struct adv_mfg_data {
	uint16_t company_code;  		// Company Identifier Code
	uint16_t seconds_since_reset;	// Number of seconds since reset
} adv_mfg_data_type;
// Define and initialize a variable of type adv_mfg_data_type
static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, 0x00 };
// -----------------------------------------------------------------------------------------------

static uint32_t app_sensor_value = 100; /* STEP 15 - Define the data you want to stream over Bluetooth LE */
//static uint32_t app_result = 0;         // Data to stream over Bluetooth LE

// Create the advertising parameter for connectable advertising
static const struct bt_data ad[] = {
	// Set the flags and populate the device name in the advertising packet
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	// Manufacturer Specific Data in the advertising packet
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

static const struct bt_data sd[] = {
	// Include the 16-bytes (128-Bits) UUID of the LBS service in the scan response packet
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
	// BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
};
// -----------------------------------------------------------------------------------------------

/* STEP 16 - Define a function to simulate the data */
static void simulate_data(void)
{
	app_sensor_value++;
	if (app_sensor_value == 200) {
		app_sensor_value = 100;
	}
}

// // Define a function to calculate the data
// static void calculate_data(void)
// {
// 	app_result = ;
// }

static void app_led_cb(bool led_state)
{
	dk_set_led(USER_LED, led_state);
}

/* STEP 18.1 - Define the thread function  */
void send_data_thread(void)
{
	while (1) {
		/* Simulate data */
		simulate_data();
		/* Send notification, the function sends notifications only if a client is subscribed */
		my_lbs_send_sensor_notify(app_sensor_value);

		k_sleep(K_MSEC(NOTIFY_INTERVAL));
	}
}

// // Define the thread function
// void send_data_thread(void)
// {
// 	while (1) {
// 		calculate_data();  // Calculate data
// 		/* Send notification, the function sends notifications only if a client is subscribed */
// 		my_cds_send_notify(app_result);

// 		k_sleep(K_MSEC(NOTIFY_INTERVAL));
// 	}
// }

static struct my_lbs_cb app_callbacks = {
	.led_cb = app_led_cb,
};


// // ----------- MY Callback functions --------------------------------------------------------------
// // Define the application callback function for pass ?operands? or ?operations?
// static void app_operation_cb(bool operand)
// {
//         dk_set_operation(USER_operation, 1);
// }

// // Define the application callback function for reading the operation result
// static bool app_result_cb(void)
// {
// 	return result;
// }

// // Declare a variable app_callbacks of type ?? and initiate its members
// static struct my_result_cb app_callbacks = {
// 	.operation_cb    = app_operation_cb,
// 	.result_cb = app_result_cb,
// };
// // ----------- END: MY Callback functions --------------------------------------------------------------


// ----------- Connection Callback functions --------------------------------------------------------------
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}
	printk("Connected\n");
	dk_set_led_on(CON_STATUS_LED);  // Turn the connection status LED on
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	dk_set_led_off(CON_STATUS_LED);  // Turn the connection status LED off
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};
// ----------- END: Connection Callback functions ---------------------------------------------------------


// ---------------------------------------------------------------------------------------------------
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
	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
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
		// printk("%ds\n", adv_mfg_data.seconds_since_reset);
		bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
/* STEP 18.2 - Define and initialize a thread to send data periodically */
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

// // Define and initialize the calculator thread
// K_THREAD_DEFINE(calculator_thread_id, STACKSIZE, calculator_thread, NULL, NULL,
//                 NULL, CALC_PRIORITY, 0, 0);
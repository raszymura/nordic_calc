/*
 * Rafal Szymura
 * June 2024
 * BLE Calculator Application
 */

#ifndef BT_CDS_H_
#define BT_CDS_H_

/**@file
 * @defgroup bt_cds Calculator Data Service (CDS) API
 * @{
 * @brief API for the Calculator Data Service (CDS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

// -----------------------------------------------------------------------------------------------
/** @brief CDS Service UUID. */
#define BT_UUID_CDS_VAL BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Arguments and operations Characteristic UUID. */
#define BT_UUID_CDS_OPERATION_VAL BT_UUID_128_ENCODE(0x00001525, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Calculated equation result Characteristic UUID. */
#define BT_UUID_CDS_RESULT_VAL BT_UUID_128_ENCODE(0x00001526, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

// Convert the array to a generic UUID
#define BT_UUID_CDS 			BT_UUID_DECLARE_128(BT_UUID_CDS_VAL)
#define BT_UUID_CDS_OPERATION	BT_UUID_DECLARE_128(BT_UUID_CDS_OPERATION_VAL)
#define BT_UUID_CDS_RESULT 		BT_UUID_DECLARE_128(BT_UUID_CDS_RESULT_VAL)


/** @brief Callback type for when a operation is received. */
typedef void (*operation_cb_t)(const bool led_state); // TODO: ????char????


/** @brief Callback struct used by the CDS Service. */
struct my_cds_cb {
	operation_cb_t operation_cb;  // LED state change callback
};


/** @brief Initialize the CDS Service.
 *
 * This function registers application callback functions with the My CDS Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_cds_init(struct my_cds_cb *callbacks);

/** @brief Send the result value as notification.
 *
 * This function sends an uint32_t equation result value. 
 *
 * @param[in] result_value The equation result value.
 *
 * @retval 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int my_cds_send_result_notify(uint32_t result_value);  // TODO: ????uint32_t???? & ????retval????32-bit float,Q31 fixed-point mode


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CDS_H_ */
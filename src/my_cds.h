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

// MODES:
#define FLOAT_MODE 0  // 32-bit floating-point mode
#define FIXED_MODE 1  // Q31 fixed-point mode
// -------------------------------------------------------------------------------------------------
#pragma pack(push, 1) // Preserve current packing settings and set packing to 1 byte
struct calculator_task {		// Define a structure for calculator tasks
	uint8_t operation;			// Operation to be performed (e.g., add, subtract)
	union {						// Union for 1 argument, allowing either float or fixed-point integer.
		float f_operand_1;		// 32-bit floating-point operand
        int32_t q31_operand_1;	// Fixed-point (Q31) operand
	};
	union {						// Union for 2 argument, allowing either float or fixed-point integer.
		float f_operand_2;		// 32-bit floating-point operand
		int32_t q31_operand_2;	// Fixed-point (Q31) operand
	};
	bool mode;					// Mode: floating-point (0) or fixed-point (1)
};
#pragma pack(pop) // Restore original packing
// -------------------------------------------------------------------------------------------------
typedef union {
    int32_t u;
    float f;
} int32_float_union;

typedef enum {
    INT32_TYPE,
    FLOAT_TYPE
} ReturnType;

typedef struct {
    int32_float_union value;
    ReturnType type;
} ReturnValue;
// -------------------------------------------------------------------------------------------------

#define EPSILON 1e-10  // Division by zero

// -------------------------------------------------------------------------------------------------
// UUID generated with: https://www.uuidgenerator.net/
/** @brief CDS Service UUID. */
#define BT_UUID_CDS_VAL BT_UUID_128_ENCODE(0x6e7e652f,0x0b5d,0x4de6,0xbcd9,0xa071d34c3e9f)

/** @brief Arguments and operations Characteristic UUID. */
#define BT_UUID_CDS_OPERATION_VAL BT_UUID_128_ENCODE(0x448e4b02,0xb99a,0x4f57,0xa76d,0xd283933c2fd5)

/** @brief Calculated equation result Characteristic UUID. */
#define BT_UUID_CDS_RESULT_VAL BT_UUID_128_ENCODE(0x4d19fe91,0x2164,0x49a8,0x9022,0x55ba662ce6fc)

// Convert the array to a generic UUID
#define BT_UUID_CDS 			BT_UUID_DECLARE_128(BT_UUID_CDS_VAL)
#define BT_UUID_CDS_OPERATION	BT_UUID_DECLARE_128(BT_UUID_CDS_OPERATION_VAL)
#define BT_UUID_CDS_RESULT 		BT_UUID_DECLARE_128(BT_UUID_CDS_RESULT_VAL)


/** @brief Callback type for when a operation is received. */
typedef void (*mode_cb_t)(const bool mode_state); // Mode (float or fixed) LED indicator


/** @brief Callback struct used by the CDS Service. */
struct my_cds_cb {
	mode_cb_t mode_cb;  // LED state change callback
};
// -------------------------------------------------------------------------------------------------

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
 * This function sends an int32_t or float equation result value. 
 *
 * @param[in] result_value The equation result value.
 *
 * @retval 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int my_cds_send_result_notify(ReturnValue result_value);

/** @brief Calculate the result value.
 *
 * This function calculates an int32_t or float equation result value. 
 *
 * @param[in] task The equation struct.
 *
 * @retval 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
ReturnValue my_cds_calculate_result(struct calculator_task task);

/** @brief Calculate the result of q31 division.
 *
 * This function calculates an int32_t division result value. 
 *
 * @param[in] a dividend
* @param[in] b divider
 *
 * @retval Result value.
 */
int32_t q_div(int32_t a, int32_t b);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CDS_H_ */

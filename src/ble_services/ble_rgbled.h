/*
 * Copyright (c) 2025 Robert Wessels
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RGBLED_H_
#define RGBLED_H_

#include <stdint.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief RGBLED Service UUID */
#define BT_UUID_RGBLED_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

/** @brief RGBLED Pattern Characteristic UUID */
#define BT_UUID_RGBLED_PATTERN_CHAR_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

/** @brief RGBLED Indicator Characteristic UUID */
#define BT_UUID_RGBLED_INDICATOR_CHAR_VAL BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

#define BT_UUID_RGBLED_SERVICE        BT_UUID_DECLARE_128(BT_UUID_RGBLED_SERVICE_VAL)
#define BT_UUID_RGBLED_PATTERN_CHAR   BT_UUID_DECLARE_128(BT_UUID_RGBLED_PATTERN_CHAR_VAL)
#define BT_UUID_RGBLED_INDICATOR_CHAR BT_UUID_DECLARE_128(BT_UUID_RGBLED_INDICATOR_CHAR_VAL)

#define INDICATOR_LEFT   0U
#define INDICATOR_RIGHT  1U
#define INDICATOR_OFF    2U
#define INDICATOR_HAZARD 3U

    void bt_rgbled_service_set_num_patterns(uint8_t num_patterns);
    uint8_t bt_rgbled_service_get_pattern(void);
    uint8_t bt_rgbled_service_get_indicator();
    int bt_rgbled_set_pattern(uint8_t level);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RGBLED_H_ */

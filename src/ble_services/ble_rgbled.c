
/*
 * Copyright (c) 2025 Robert Wessels
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>

#include "ble_rgbled.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rgb_led, LOG_LEVEL_DBG);

static uint8_t rgb_pattern_curr = 0U;
static uint8_t rgb_pattern = 0U;

// TODO: convert to atomic bitfield
static uint8_t indicator_state = INDICATOR_OFF;

static uint8_t _num_patterns = 0;

static void rgbpat_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    ARG_UNUSED(attr);

    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

    LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t write_pattern(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    const void* buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    if (offset + len > sizeof(rgb_pattern))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(&rgb_pattern + offset, buf, len);

    if (rgb_pattern >= _num_patterns)
    {
        return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
    }

    rgb_pattern_curr = rgb_pattern;
    LOG_INF("RGB Pattern set to %d", rgb_pattern_curr);

    // Here you would add code to update the RGB LED strip with the new pattern

    return len;
}

static ssize_t write_indicator(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    const void* buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    if (offset + len > sizeof(indicator_state))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(&indicator_state + offset, buf, len);

    if (indicator_state >= INDICATOR_OFF)
    {
        return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
    }

    LOG_INF("RGB indicator set to %d", indicator_state);

    return len;
}
BT_GATT_SERVICE_DEFINE(
    rgbled_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_RGBLED_SERVICE),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_RGBLED_PATTERN_CHAR,
        BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
        BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
        NULL,
        write_pattern,
        &rgb_pattern),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_RGBLED_INDICATOR_CHAR,
        BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
        BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
        NULL,
        write_indicator,
        &indicator_state),
    BT_GATT_CCC(rgbpat_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static int rgbled_init(void)
{

    return 0;
}

uint8_t bt_rgbled_service_get_indicator(void)
{
    return indicator_state;
}

uint8_t bt_rgbled_service_get_pattern(void)
{
    return rgb_pattern_curr;
}

void bt_rgbled_service_set_num_patterns(uint8_t num_patterns)
{
    _num_patterns = num_patterns;
}

SYS_INIT(rgbled_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE      2048
#define THREAD_PRIORITY 2

/*
 * Copyright (c) 2025 Robert Wessels
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_services/ble_rgbled.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define SMP_BT_SVC_UUID_VAL BT_UUID_128_ENCODE(0x8d53dc1d, 0x1db7, 0x4cd3, 0x868b, 0x8a527460aa84)
/** SMP service UUID. */
#define SMP_BT_SVC_UUID BT_UUID_DECLARE_128(SMP_BT_SVC_UUID_VAL)
/** SMP characteristic UUID value. */
#define SMP_BT_CHR_UUID_VAL BT_UUID_128_ENCODE(0xda2e7828, 0xfbce, 0x4e01, 0xae9e, 0x261174997c48)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_RGBLED_SERVICE_VAL),
};

#if !defined(CONFIG_BT_EXT_ADV)
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_RGBLED_SERVICE_VAL),
    // BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};
#endif /* !CONFIG_BT_EXT_ADV */

/* Use atomic variable, 2 bits for connection and disconnection state */
static ATOMIC_DEFINE(state, 2U);

#define STATE_CONNECTED    1U
#define STATE_DISCONNECTED 2U

static void connected(struct bt_conn* conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err 0x%02x)\n", err);
    }
    else
    {
        printk("Connected\n");

        (void)atomic_set_bit(state, STATE_CONNECTED);
    }
}

static void disconnected(struct bt_conn* conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);

    (void)atomic_set_bit(state, STATE_DISCONNECTED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

void ble_thread(void)
{
    int err;
    char addr_s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_t addr = { 0 };
    size_t count = 1;

    printk("Sample - Bluetooth Peripheral NUS\n");

    err = bt_enable(NULL);
    if (err)
    {
        printk("Failed to enable bluetooth: %d\n", err);
        return;
    }

    printk("Starting Legacy Advertising (connectable and scannable)\n");
    err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    // bt_id_get(&addr, &count);
    // bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

    // printk("Beacon started, advertising as %s\n", addr_s);

    printk("Initialization complete\n");

    while (true)
    {
        k_sleep(K_SECONDS(3));

        if (atomic_test_and_clear_bit(state, STATE_CONNECTED))
        {
            /* Connected callback executed */
        }
        else if (atomic_test_and_clear_bit(state, STATE_DISCONNECTED))
        {
            printk("Starting Legacy Advertising (connectable and scannable)\n");
            err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
            if (err)
            {
                printk("Advertising failed to start (err %d)\n", err);
                return;
            }
        }
    }

    return;
}

K_THREAD_DEFINE(ble_thread_id, STACK_SIZE, ble_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

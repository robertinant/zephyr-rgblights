/*
 * Copyright (c) 2019 Robert Wessels
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/stats/stats.h>
#include <zephyr/usb/usb_device.h>

#ifdef CONFIG_MCUMGR_GRP_FS
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_STAT
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif

LOG_MODULE_REGISTER(bike_light, LOG_LEVEL_INF);

#define STORAGE_PARTITION_LABEL storage_partition
#define STORAGE_PARTITION_ID    FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

#ifdef CONFIG_MCUMGR_GRP_FS
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = { .type = FS_LITTLEFS,
                                          .fs_data = &cstorage,
                                          .storage_dev = (void*)STORAGE_PARTITION_ID,
                                          .mnt_point = "/lfs1" };
#endif

int main()
{
    int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32, "smp_svr_stats");

    if (rc < 0)
    {
        LOG_ERR("Error initializing stats system [%d]", rc);
    }

#ifdef CONFIG_MCUMGR_GRP_FS
    rc = fs_mount(&littlefs_mnt);
    if (rc < 0)
    {
        LOG_ERR("Error mounting littlefs [%d]", rc);
    }
#endif
    /* Register the built-in mcumgr command handlers. */
    /* using __TIME__ ensure that a new binary will be built on every
     * compile which is convenient when testing firmware upgrade.
     */
    LOG_INF("build time: " __DATE__ " " __TIME__);

    /* The system work queue handles all incoming mcumgr requests.  Let the
     * main thread idle while the mcumgr server runs.
     */
    while (1)
    {
        k_sleep(K_MSEC(1000));
        STATS_INC(smp_svr_stats, ticks);
    }
    return 0;
}
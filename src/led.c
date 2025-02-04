#include "ble_services/ble_rgbled.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(led_thread, LOG_LEVEL_DBG);

#define STACK_SIZE 1024
#define PRIORITY   7

#define STRIP_NODE DT_ALIAS(led_strip)

/* Strip Layout
 * [RIGH RUNNER 44] [RIGHT INDICATOR 14] [LEFT INDICATOR 14] [LEFT RUNNER 44]
 */

static uint8_t indicator_state = INDICATOR_OFF;

// Indicators are 23 cm long
#define INDICATOR_RIGHT_NUM_PIXELS 14
#define INDICATOR_LEFT_NUM_PIXELS  14
// Runners are 74 cm long
#define RUNNER_RIGHT_NUM_PIXELS 44
#define RUNNER_LEFT_NUM_PIXELS  44

// #define STRIP_NUM_PIXELS (INDICATOR_RIGHT_NUM_PIXELS + INDICATOR_LEFT_NUM_PIXELS + RUNNER_RIGHT_NUM_PIXELS +
// RUNNER_LEFT_NUM_PIXELS)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define LED_UPDATE_DELAY 10
#define DELAY_TIME       K_MSEC(LED_UPDATE_DELAY)
#define LED_BLINK_DELAY  500
#define BLINK_DELAY_TIME K_MSEC(LED_BLINK_DELAY)

#define RGB(_r, _g, _b)                 \
    {                                   \
        .r = (_b), .g = (_g), .b = (_r) \
    }

#define HUE_MAX 256

enum
{
    green,
    red,
    blue,
    off,
};

static const struct led_rgb colors[] = {
    RGB(0xff, 0x00, 0x00), /* green */
    RGB(0x00, 0xff, 0x00), /* red */
    RGB(0x00, 0x00, 0xff), /* blue */
    RGB(0x00, 0x00, 0x00), /* off */
};

void rainbow_effect(void);
void off_pattern(void);
void mode_breath(void);
typedef void (*pattern_func_t)(void);

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
static const pattern_func_t pattern_table[] = {
    off_pattern,
    rainbow_effect,
    mode_breath,
};

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device* const strip = DEVICE_DT_GET(STRIP_NODE);

void update_turn_indicators(void)
{
    static bool flash_on = true;
    static int64_t last_toggle_time = 0;
    int64_t current_time = k_uptime_get();

    if (current_time - last_toggle_time >= LED_BLINK_DELAY)
    {
        flash_on = !flash_on;
        last_toggle_time = current_time;
    }

    if (indicator_state == INDICATOR_LEFT)
    {
        for (size_t i = RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS;
             i < RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS + INDICATOR_LEFT_NUM_PIXELS;
             i++)
        {
            pixels[i] = flash_on ? colors[red] : colors[off]; // Flashing red for left indicator
        }
    }
    else if (indicator_state == INDICATOR_RIGHT)
    {
        for (size_t i = RUNNER_RIGHT_NUM_PIXELS; i < RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS; i++)
        {
            pixels[i] = flash_on ? colors[red] : colors[off]; // Flashing red for right indicator
        }
    }
    else if (indicator_state == INDICATOR_HAZARD)
    {
        for (size_t i = RUNNER_RIGHT_NUM_PIXELS; i < RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS; i++)
        {
            pixels[i] = flash_on ? colors[red] : colors[off]; // Flashing red for right indicator
        }
        for (size_t i = RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS;
             i < RUNNER_RIGHT_NUM_PIXELS + INDICATOR_RIGHT_NUM_PIXELS + INDICATOR_LEFT_NUM_PIXELS;
             i++)
        {
            pixels[i] = flash_on ? colors[red] : colors[off]; // Flashing red for left indicator
        }
    }
}

void off_pattern(void)
{
    memset(pixels, 0, sizeof(pixels));
    update_turn_indicators();
    led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
    k_sleep(DELAY_TIME);
}

void rainbow_effect(void)
{
    static uint8_t hue = 0;

    for (size_t i = 0; i < STRIP_NUM_PIXELS; i++)
    {
        uint8_t h = (hue + i * HUE_MAX / STRIP_NUM_PIXELS) % 256;
        uint8_t r, g, b;

        if (h < 85)
        {
            r = h * 3;
            g = 255 - h * 3;
            b = 0;
        }
        else if (h < 170)
        {
            h -= 85;
            r = 255 - h * 3;
            g = 0;
            b = h * 3;
        }
        else
        {
            h -= 170;
            r = 0;
            g = h * 3;
            b = 255 - h * 3;
        }

        pixels[i] = (struct led_rgb){ .r = r, .g = g, .b = b };
    }

    update_turn_indicators();
    led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
    hue += 1;
    k_sleep(DELAY_TIME);
}

void mode_breath(void)
{
    static uint8_t brightness = 0;
    static bool increasing = true;

    if (increasing)
    {
        brightness++;
        if (brightness == 255)
        {
            increasing = false;
        }
    }
    else
    {
        brightness--;
        if (brightness == 0)
        {
            increasing = true;
        }
    }

    for (size_t i = 0; i < STRIP_NUM_PIXELS; i++)
    {
        pixels[i].r = (colors[red].r * brightness) / 255;
        pixels[i].g = (colors[red].g * brightness) / 255;
        pixels[i].b = (colors[red].b * brightness) / 255;
    }

    update_turn_indicators();
    led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
    k_sleep(DELAY_TIME);
}

void led_thread(void)
{
    uint8_t rgb_pattern = 0, rgb_pattern_prev = 0;

    if (!device_is_ready(strip))
    {
        LOG_ERR("LED strip device %s is not ready", strip->name);
        return;
    }

    LOG_INF("Found LED strip device %s", strip->name);

    /*
     * This will ensure that BLE returns an error if
     * the user sets a pattern num that is beyond the number of available patterns.
     */
    bt_rgbled_service_set_num_patterns(NELEMS(pattern_table));

    while (1)
    {

        indicator_state = bt_rgbled_service_get_indicator();
        rgb_pattern = bt_rgbled_service_get_pattern();
        if (rgb_pattern != rgb_pattern_prev)
        {
            LOG_DBG("RGB pattern %d", rgb_pattern);
            if (rgb_pattern <= ARRAY_SIZE(pattern_table))
            {
                rgb_pattern_prev = rgb_pattern;
            }
        }
        pattern_table[rgb_pattern_prev]();
    }
}

K_THREAD_DEFINE(led_thread_id, STACK_SIZE, led_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
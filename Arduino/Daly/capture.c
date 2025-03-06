/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "capture.h"

typedef struct capture_t {
    uint32_t value;
    uint8_t event;
} capture_t;

const static char *TAG = "capture";

#define CAPTURE_GPIO 13
#define TIMEOUT_BITS 15

static QueueHandle_t capture_queue = NULL;

static bool IRAM_ATTR reference_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata,
                                         void *user_data) {
    capture_t event = {.value = edata->cap_value, .event = CAPTURE_NO_EVENT};
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR(capture_queue, &event, &pxHigherPriorityTaskWoken);
    return pxHigherPriorityTaskWoken == pdTRUE;
}

static bool IRAM_ATTR capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata,
                                       void *user_data) {
    capture_t event = {.value = edata->cap_value,
                       .event =
                           (edata->cap_edge == MCPWM_CAP_EDGE_POS) ? CAPTURE_EDGE_POS : CAPTURE_EDGE_NEG};
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR(capture_queue, &event, &pxHigherPriorityTaskWoken);
    return pxHigherPriorityTaskWoken == pdTRUE;
}

static mcpwm_cap_channel_handle_t ref_chan = NULL;

bool capture_init(void) {

    if (capture_queue == NULL) {
        capture_queue = xQueueCreate(32, sizeof(capture_t));
        if (capture_queue == NULL) {
            return false;
        }
    }

    ESP_LOGI(TAG, "Install capture timer");
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    // mcpwm_cap_timer_handle_t cap_ref = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));
    // ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_ref));

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    // mcpwm_cap_channel_handle_t cap_ref = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = CAPTURE_GPIO,
        .prescale = 1,
        // capture on both edge
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        // pull up internally
        .flags.pull_up = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = capture_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, NULL));

    ESP_LOGI(TAG, "Enable capture channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    mcpwm_capture_channel_config_t ref_ch_conf = {
        .gpio_num = CAPTURE_GPIO,
        .prescale = 1,
        // capture on both edge
        .flags.neg_edge = false,
        .flags.pos_edge = false,
        // pull up internally
        .flags.pull_up = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &ref_ch_conf, &ref_chan));

    ESP_LOGI(TAG, "Register reference callback");
    cbs.on_cap = reference_callback;

    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(ref_chan, &cbs, NULL));

    ESP_LOGI(TAG, "Enable reference channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(ref_chan));

    ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));

    return true;
}



// int32_t capture_get_event_count(void) {
//     capture_t event;
//     if (xQueueReceive(capture_queue, &event, 0)) {
//         uint32_t delta = (((event.value - last_count) + (8333 / 2)) / 8333);
//         last_count = event.value;
//         if (event.edge_pos) {
//             return (int32_t)delta;
//         } else {
//             return -(int32_t)delta;
//         }
//     }
//     return 0;
// }

// int32_t capture_get_timer_count(void) {
//     capture_t time;
//     mcpwm_capture_channel_trigger_soft_catch(ref_chan);

//     if (xQueueReceive(reference_queue, &time, 1)) {
//         uint32_t delta = (((time.value - last_count) + (8333 / 2)) / 8333);
//         return (int32_t)delta;
//     }
//     return 0;
// }

capture_event_t capture_get_event(void) {
    static uint32_t last_count = 0;
    capture_event_t event;
    capture_t capture;

    mcpwm_capture_channel_trigger_soft_catch(ref_chan);
    while (xQueueReceive(capture_queue, &capture, 0)) {
        uint32_t delta = (((capture.value - last_count) + (8333 / 2)) / 8333);
        if (delta > 255) {
            delta = 255;
        }
        event.value = delta;
        event.event = capture.event;
        if (event.event != CAPTURE_NO_EVENT) {
            last_count = capture.value;
            break;
        }
        if(delta > TIMEOUT_BITS){
            event.event = CAPTURE_TIMEOUT;
        }
    }
    return event;
}

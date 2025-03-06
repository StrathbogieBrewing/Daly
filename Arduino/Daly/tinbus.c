#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinbus.h"

#define GPIO_RX 13

static QueueHandle_t rx_queue = NULL;

static bool IRAM_ATTR capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata,
                                       void *user_data) {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR(rx_queue, &edata->cap_value, &pxHigherPriorityTaskWoken);
    return pxHigherPriorityTaskWoken == pdTRUE;
}


bool tinbus_init(void){
    if (rx_queue == NULL) {
        rx_queue = xQueueCreate(32, sizeof(uint32_t));
        if (rx_queue == NULL) {
            return false;
        }
    }

    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));

    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = GPIO_RX,
        .prescale = 1,
        .flags.neg_edge = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));

    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = capture_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, NULL));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));

    return true;
}


bool tinbus(uint8_t rx_data[], uint8_t rx_len){

    
}


// tinbus_event_t tinbus_get_event(void){

// }
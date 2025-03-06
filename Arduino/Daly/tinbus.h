#ifndef TINBUS_H
#define TINBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

enum {
    TINBUS_NO_EVENT = 0,
    TINBUS_RX_READY,
    // TINBUS_RX_PULSE = 1,
    // TINBUS_TIMEOUT = 2,
};

typedef struct tinbus_event_t {
    uint8_t event;
    uint8_t value;
} tinbus_event_t;

bool tinbus_init(void);

bool tinbus(uint8_t rx_data[], uint8_t rx_len);
// tinbus_event_t tinbus_get_event(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TINBUS_H








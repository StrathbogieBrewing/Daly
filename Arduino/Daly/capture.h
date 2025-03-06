#ifndef CAPTURE_H
#define CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CAPTURE_NO_EVENT = 0,
    CAPTURE_EDGE_NEG = 1,
    CAPTURE_EDGE_POS = 2,
    CAPTURE_TIMEOUT = 3,
};

typedef struct capture_event_t {
    unsigned event;
    unsigned value;
} capture_event_t;

bool capture_init(void);
capture_event_t capture_get_event(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CAPTURE_H








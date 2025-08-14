#ifndef HELPERS_INPUT_HELPERS_H_
#define HELPERS_INPUT_HELPERS_H_

#include <linux/input-event-codes.h>
#include <stdbool.h>

typedef int (*key_event_cb)(void);
typedef void (*mouse_event_cb)(int dx, int dy, int left, int right, int middle);

int init_input_handler(key_event_cb key_cb, mouse_event_cb mouse_cb);

bool is_key_pressed(int key);

void free_input_handler();

#endif

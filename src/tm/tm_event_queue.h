#pragma once
#include <stdbool.h>
typedef void (*event_callback)(unsigned data);

#define RING_BUF_SIZE 1024

typedef struct {
	event_callback callback;
	unsigned data;
} event_entry;

typedef struct {
	unsigned read_pos;
	unsigned write_pos;
	event_entry events[RING_BUF_SIZE];
} event_ringbuf;

void enqueue_event(volatile event_ringbuf* queue, event_callback callback, unsigned data);
bool pop_event(volatile event_ringbuf* queue);

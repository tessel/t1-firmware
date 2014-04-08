#include <stdint.h>
#include "tm_event_queue.h"

void check_failed(char *file, uint32_t line);

void enqueue_event(volatile event_ringbuf* queue, event_callback callback, unsigned data) {
	unsigned pos = queue->write_pos;
	unsigned next_pos = (queue->write_pos + 1) % RING_BUF_SIZE;

	if (next_pos != queue->read_pos) {
		queue->events[pos].callback = callback;
		queue->events[pos].data = data;
		queue->write_pos = next_pos;
	} else {
		// Queue is full
		check_failed(__FILE__, __LINE__);
	}
}

bool pop_event(volatile event_ringbuf* queue) {
	unsigned pos = queue->read_pos;
	if (pos != queue->write_pos) {
		queue->events[pos].callback(queue->events[pos].data);
		queue->events[pos].callback = 0;
		queue->events[pos].data = 0;
		queue->read_pos = (pos + 1) % RING_BUF_SIZE;
		return true;
	} else {
		return false;
	}
}


#include "common.h"


/**
 * @brief Creates a circular buffer.
 * @param size Size of the buffer.
 * @return Pointer to the created circular buffer structure.
 */
circbuf_t* create_buffer(uint16_t size) {
    if (size == 0) return NULL;

    circbuf_t *cb = (circbuf_t*) malloc(sizeof(circbuf_t));
    if (cb == NULL) return NULL;

    cb->buffer = (float*) calloc(size, sizeof(float));
    if (cb->buffer == NULL) {
        free(cb);
        return NULL;
    }

    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;

    return cb;
}

/**
 * @brief Destroys the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 */
void destroy_buffer(circbuf_t *cb) {
    if (cb == NULL) return;
    free(cb->buffer);
    free(cb);
}

/**
 * @brief Adds an element to the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 * @param value Value to be added.
 */
void buffer_push(circbuf_t *cb, float value) {
    if (cb == NULL) return;

    //place the new value at the tail index
    cb->buffer[cb->tail] = value;

    // Move the tail index forward in a circular manner
    cb->tail = (cb->tail + 1) % cb->size;

    // If the buffer was full, also advance the head
    // to "discard" the oldest element.
    if (cb->count == cb->size) {
        cb->head = (cb->head + 1) % cb->size;
    } else {
        // If it wasn't full, just increment the count
        cb->count++;
    }
}

/**
 * @brief Removes and returns the oldest element from the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 * @return The oldest element, or 0.0f if the buffer is empty.
 */
float buffer_pop(circbuf_t *cb) {
    if (cb == NULL || cb->count == 0) {
        return 0.0f; // empty buffer or invalid pointer
    }

    float value = cb->buffer[cb->head];
    cb->head = (cb->head + 1) % cb->size;
    cb->count--;

    return value;
}

/**
 * @brief Resets the circular buffer to empty state.
 * @param cb Pointer to the circular buffer structure.
 */
void buffer_reset(circbuf_t *cb) {
    if (cb == NULL) return;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    memset(cb->buffer, 0, cb->size * sizeof(float));
}
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define FS   100.0f  // Sample frequency in Hz
#define DT   (1.0f/FS) // Sample period in seconds

typedef struct {
    float *buffer; // pointer to buffer array
    uint16_t size;      // maximum number of elements in the buffer
    uint16_t head;      // index of the head element
    uint16_t tail;      // index of the tail element
    uint16_t count;     // current number of elements in the buffer
} circbuf_t;


/**
 * @brief Creates a circular buffer.
 * @param size Size of the buffer.
 * @return Pointer to the created circular buffer structure.
 */
circbuf_t* create_buffer(uint16_t size);

/**
 * @brief Destroys the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 */
void destroy_buffer(circbuf_t *cb);

/**
 * @brief Adds an element to the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 * @param value Value to be added.
 */
void buffer_push(circbuf_t *cb, float value);

/**
 * @brief Removes and returns the oldest element from the circular buffer.
 * @param cb Pointer to the circular buffer structure.
 * @return The oldest element, or 0.0f if the buffer is empty.
 */
float buffer_pop(circbuf_t *cb);

/**
 * @brief Resets the circular buffer to empty state.
 * @param cb Pointer to the circular buffer structure.
 */
void buffer_reset(circbuf_t *cb);

#endif // COMMON_H
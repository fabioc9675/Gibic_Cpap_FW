#ifndef FILTER_H
#define FILTER_H


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern const float butn3[3];
extern const float butd3[3];
// extern const float butn2[2];
// extern const float butd2[2];


/**
 * @brief Filter structure.
 */
typedef struct {
    uint8_t fcof; // filter order
    float *input;
    float *output;
    float *num;
    float *den;
} filter_t;

// prototype functions
/**
 * @brief Creates a filter structure.
 * @param fcof Filter order.
 * @return Pointer to the created filter structure.
 */
filter_t *filter_create(uint8_t fcof);

/**
 * @brief Destroys the filter structure.
 * @param filter Pointer to the filter structure.
 */
void filter_destroy(filter_t *filter);

/**
 * @brief Initializes the filter structure.
 * @param filter Pointer to the filter structure.
 * @param num Coefficients of the numerator.
 * @param den Coefficients of the denominator.
 */
void filter_init(filter_t *filter, const float *num, const float *den);

/**
 * Low-pass filter
 * @brief Applies a low-pass filter to the input data.
 * @param filter Pointer to the filter structure.
 * @param datain Input data to be filtered.
 * @param dataout Pointer to the output filtered data.
 */
void lp_filter(filter_t *filter, float datain, float *dataout);

/**
 * @brief Calculates the median of an array of floats.
 * @param data Pointer to the array of floats.
 * @param new_value The new value to be added to the array.
 * @param size Size of the array.
 * @return The median value.
 */
float median(float *data, float new_value, uint8_t size);

/**
 * @brief Calculates the mean of an array of floats.
 * @param data Pointer to the array of floats.
 * @param size Size of the array.
 * @return The mean value.
 */
float mean(float *data, uint16_t size);

#endif // FILTER_H
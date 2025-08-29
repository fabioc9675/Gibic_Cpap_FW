#include "filter.h"

/**
 * Butterworth filter coefficients
 * For a 1st order Butterworth filter 
 * with a cutoff frequency of 2 Hz normalized
 */
//const float butn2[2] = { 5.919070381840547e-02,  5.919070381840547e-02};
//const float butd2[2] = { 1.000000000000000e+00, -8.816185923631891e-01};

/**
 * Butterworth filter coefficients
 * For a 2nd order Butterworth filter 
 * with a cutoff frequency of 5 Hz normalized
 */
const float butn3[3] = { 2.008336556421125e-02,  4.016673112842249e-02, 2.008336556421125e-02};
const float butd3[3] = { 1.000000000000000e+00, -1.561018075800718e+00, 6.413515380575630e-01};

/**
 * @brief Creates a filter structure.
 * @param fcof Filter order.
 * @return Pointer to the created filter structure.
 */
filter_t *filter_create(uint8_t fcof) {
    // Crear la estructura del filtro
    filter_t *filter = (filter_t *)malloc(sizeof(filter_t));
    if (filter == NULL) {
        return NULL; // Error al asignar memoria
    }

    // Asignar memoria dinámica para los arreglos
    filter->input = (float *)calloc(fcof, sizeof(float));
    filter->output = (float *)calloc(fcof, sizeof(float));
    filter->num = (float *)calloc(fcof, sizeof(float));
    filter->den = (float *)calloc(fcof, sizeof(float));

    if (filter->input == NULL || filter->output == NULL || filter->num == NULL || filter->den == NULL) {
        // Liberar memoria en caso de error
        filter_destroy(filter);
        return NULL;
    }

    filter->fcof = fcof;
    return filter;
}

/**
 * @brief Destroys the filter structure.
 * @param filter Pointer to the filter structure.
 */
void filter_destroy(filter_t *filter) {
    if (filter != NULL) {
        // Liberar memoria dinámica
        free(filter->input);
        free(filter->output);
        free(filter->num);
        free(filter->den);
        free(filter); // Liberar la estructura
    }
}

/**
 * @brief Initializes the filter structure.
 * @param filter Pointer to the filter structure.
 * @param num Coefficients of the numerator.
 * @param den Coefficients of the denominator.
 */
void filter_init(filter_t *filter, const float *num, const float *den) {
    if (filter == NULL || num == NULL || den == NULL ) {
        return; // Validación de parámetros
    }

    // Inicializar coeficientes del numerador y denominador
    for (uint8_t i = 0; i < filter->fcof; i++) {
        filter->num[i] = num[i];
        filter->den[i] = den[i];
    }

    // Limpiar las memorias de entrada y salida
    for (uint8_t i = 0; i < filter->fcof; i++) {
        filter->input[i] = 0.0f;
        filter->output[i] = 0.0f;
    }
}

/**
 * Low-pass filter
 * @brief Applies a low-pass filter to the input data.
 * @param filter Pointer to the filter structure.
 * @param datain Input data to be filtered.
 * @param dataout Pointer to the output filtered data.
 */
void lp_filter(filter_t *filter, float datain, float *dataout){
    uint8_t i;
    for(i=filter->fcof-1; i>0; i--){
        filter->input[i] = filter->input[i-1];
        filter->output[i] = filter->output[i-1];
    }
    filter->input[0] = datain;

    filter->output[0] = (filter->num[0] * filter->input[0]);
    for(i=1; i<filter->fcof; i++){
        filter->output[0] += (filter->num[i] * filter->input[i]) 
                            - (filter->den[i] * filter->output[i]);
    }

    *dataout = filter->output[0];
}

/**
 * @brief Calculates the median of an array of floats.
 * @param data Pointer to the array of floats.
 * @param new_value The new value to be added to the array.
 * @param size Size of the array.
 * @return The median value.
 */
float median(float *data, float new_value, uint8_t size) {

    if (data == NULL || size == 0) {
        return 0.0f; // invalid input
    }

    float temp[size];

    for (uint8_t i = 0; i < size; i++){
        data[i] = data[i+1];
        temp[i] = data[i];
    }

    data[size-1] = new_value;
    temp[size-1] = data[size-1];

    // sort temporary array
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = i + 1; j < size; j++) {
            if (temp[i] > temp[j]) {
                float t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }

    // compute median
    if (size % 2 == 0) {
        return (temp[size / 2 - 1] + temp[size / 2]) / 2.0f;
    } else {
        return temp[size / 2];
    }
}

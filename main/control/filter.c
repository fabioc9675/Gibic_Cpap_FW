#include "filter.h"

const float butn[2] = { 8.6364023e-02,  8.6364023e-02};
const float butd[2] = { 1.0000000e+00, -8.2727194e-01};

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

void lp_filter(filter_t *filter, float datain, float *dataout){
    uint8_t i;
    for(i=filter->fcof-1; i>0; i--){
        filter->input[i] = filter->input[i-1];
        filter->output[i] = filter->output[i-1];
    }
    filter->input[0] = datain;

    filter->output[0] = (filter->num[0] * filter->input[0]);
    for(i=1; i<filter->fcof; i++){
        filter->output[0] += (filter->num[i] * filter->input[i]) - (filter->den[i] * filter->output[i]);
    }

    *dataout = filter->output[0];
}

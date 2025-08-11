#ifndef FILTER_H
#define FILTER_H


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern const float butn[2];
extern const float butd[2];

typedef struct {
    uint8_t fcof; // filter order
    float *input;
    float *output;
    float *num;
    float *den;
} filter_t;

// Prototipos de funciones
filter_t *filter_create(uint8_t fcof);
void filter_destroy(filter_t *filter);
void filter_init(filter_t *filter, const float *num, const float *den);
void lp_filter(filter_t *filter, float datain, float *dataout);

#endif // FILTER_H
#ifndef LUT_H
#define LUT_H

#include <stdio.h>

// Lookup table structure
typedef struct {
    float *x; // Array de entradas (valores de entrada)
    float *y; // Array de salidas (valores correspondientes)
    size_t size; // Tamaño de la tabla
} lookup_table_t;

extern lookup_table_t lut_p; 

void init_lut_p(void);
float lookup_table_get(const lookup_table_t *table, float input);


#endif // LUT_H
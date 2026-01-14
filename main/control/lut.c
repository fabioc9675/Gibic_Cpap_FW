#include "lut.h"

float x_values[] = { 4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f}; // Entradas
float y_values[] = {0.15f, 0.22f, 0.35f, 0.28f, 0.30f, 0.40f, 0.43f, 0.48f, 0.48f,
                    0.52f, 0.62f, 0.70f, 0.70f, 0.80f, 0.73f, 0.85f, 0.82f}; // Salidas

lookup_table_t lut_p;

void init_lut_p(void){
    lut_p.x = x_values;
    lut_p.y = y_values;
    lut_p.size = sizeof(x_values) / sizeof(x_values[0]);
}                    

float lookup_table_get(const lookup_table_t *table, float input) {
    // Manejar casos fuera de los límites
    if (input <= table->x[0]) {
        return table->y[0];
    }
    if (input >= table->x[table->size - 1]) {
        return table->y[table->size - 1];
    }

    // Búsqueda lineal para encontrar el intervalo adecuado
    for (size_t i = 0; i < table->size - 1; i++) {
        if (input >= table->x[i] && input < table->x[i + 1]) {
            // Interpolación lineal
            float t = (input - table->x[i]) / (table->x[i + 1] - table->x[i]);
            return table->y[i] + t * (table->y[i + 1] - table->y[i]);
        }
    }

    // Esto no debería ocurrir
    return 0.0f;
}
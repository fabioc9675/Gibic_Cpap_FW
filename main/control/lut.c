#include "lut.h"

float x_values[] = { 4.0f,  6.0f,  8.0f, 10.0f, 12.0f,
                    14.0f, 16.0f, 18.0f, 20.0f, 22.0f,
                    24.0f, 26.0f, 28.0f, 30.0f}; // Entradas
float y_values[] = {24.0f, 28.0f, 32.0f, 36.0f, 40.0f,
                    43.0f, 47.0f, 50.0f, 53.0f, 55.0f,
                    58.0f, 60.0f, 62.0f, 64.0f}; // Salidas

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
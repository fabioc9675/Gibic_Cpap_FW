#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>

//#define control1
#define control2
 


// Constantes y parámetros
//#define M_PI 3.14159265358979323846
// #define PMIN 4.0    // Presión mínima de tratamiento (cmH2O)
// #define PMAX 20.0   // Presión máxima de tratamiento (cmH2O)
// #define PWM_MIN 1           // PWM mínimo (1%)
// #define PWM_MAX 1000        // PWM máximo (100%)

/**
 * Prototipos de funciones
 */

 /**
 * brief: Funcion para calcular el control PID
 * param: setpointPresion: presion objetivo del sistema
 * param: presion: presion actual del sistema
 * return: salida del control PID
 */
uint16_t controller(uint8_t setpointPresion, float presion, float flow);

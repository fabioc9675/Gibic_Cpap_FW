#include "control.h"


// Constantes del control PID
#define KP 150  // Ganancia proporcional
#define KI 2000  // Ganancia integral
#define KD 30.4 // Ganancia derivativa

// Constantes y parámetros
#define PRESION_MINIMA 4.0    // Presión mínima de tratamiento (cmH2O)
#define PRESION_MAXIMA 20.0   // Presión máxima de tratamiento (cmH2O)
#define PWM_MIN 0           // PWM mínimo (0%)
#define PWM_MAX 1000        // PWM máximo (100%)

// Intervalo de muestreo (en segundos)
float TIEMPO_MUESTREO = 0.05;

// Variables globales
float error_acumulado = 0;  // Acumulador para el término integral
float error_anterior = 0;   // Error anterior para el término derivativo

/**
 * prototipos de funciones
 */
uint16_t control_pid(float presion_actual, float presion_objetivo);

// Función para calcular la salida del control PID
uint16_t control_pid(float presion_actual, float presion_objetivo) {
    // Calcular el error actual
    float error = presion_objetivo - presion_actual;
    
    // Término proporcional
    float P = KP * error;

    // Término integral
    error_acumulado += error * TIEMPO_MUESTREO;
    float I = KI * error_acumulado;

    // Término derivativo
    float delta_error = (error - error_anterior) / TIEMPO_MUESTREO;
    float D = KD * delta_error;

    // Guardar el error actual para la siguiente iteración
    error_anterior = error;

    //ESP_LOGI("CONTROL", "error: %0.2f,  P: %0.2f, I: %0.2f, D: %0.2f", error, P, I, D);
    // Calcular la salida del PID
    float salida_pid = P + I + D;


    int16_t salida_pid_int = (int16_t)round(salida_pid);
    // Limitar la salida a los valores de PWM permitidos
    if (salida_pid_int < PWM_MIN) {
        salida_pid_int = PWM_MIN;
    } else if (salida_pid_int > PWM_MAX) {
        salida_pid_int = PWM_MAX;
    }

    return salida_pid_int;
}

/**
 * brief: Funcion para calcular el control PID
 * param: setpointPresion: presion objetivo del sistema
 * param: presion: presion actual del sistema
 * return: salida del control PID
 */
uint16_t controller(uint8_t setpointPresion, float presion) {
    // Inicialización
    float presion_objetivo = (float)setpointPresion;
    uint16_t pwm_output;

    //ESP_LOGI("MAIN", "bldc_sp: %d", bldc_sp);
    //ESP_LOGI("MAIN", "bldc_sp: %d", bldc_sp);
    //ESP_LOGI("CONTROL", "presion: %0.2f, presionObjetivo: %0.2f", presion, presion_objetivo);
    // Calcular la salida del control PID
    pwm_output = control_pid(presion, presion_objetivo);

    //ESP_LOGI("CONTROL", "PWM: %d", pwm_output);
    return pwm_output;
}





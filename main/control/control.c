#include "control.h"

#ifdef control1

#define caract
#define escalon

    #ifdef escalon
    #define delta 100
        uint16_t puntos[]={180, 550, 180, 800, 180, 1};
        uint8_t cnt1=0;
    #endif

    #ifdef caract
        uint16_t carcnt = 0;
        int16_t pwmtmp = 0;
    #endif

    // Constantes del control PID
    /**
     * Ziegler-Nichols: Kp=9.37, Ti=2.21s, Td=0.55s
     * Cohen-Coon: Kp=17.98, Ti=1.67s, Td=0.23s
     */
    //#define KP 150  // Ganancia proporcional
    //#define KI 1750  // Ganancia integral  // estaba en 2000
    //#define KD 30.4 // Ganancia derivativa
    float KP = 200;  // Ganancia proporcional
    float KI = 300;  // Ganancia integral  // estaba en 2000
    float KD = 30; // Ganancia derivativa

    uint16_t pwm_output = 0; // Salida del PWM inicial

    // Intervalo de muestreo (en segundos)
    float TIEMPO_MUESTREO = 0.01;

    // Variables globales
    float error_acumulado = 0;  // Acumulador para el término integral
    float error_anterior = 0;   // Error anterior para el término derivativo

    /**
     * prototipos de funciones
     */
     uint16_t control_pid(float presion_actual, float presion_objetivo);

    /**
     * brief: Funcion para calcular el control PID
     * param: setpointPresion: presion objetivo del sistema
     * param: presion: presion actual del sistema
     * return: salida del control PID
     */
    uint16_t controller(uint8_t setpointPresion, float presion, float flow) {
    #ifdef caract
        #ifdef escalon
        if (carcnt++ >= 6000) {
            carcnt =0;
            pwm_output = puntos[cnt1++];
             ESP_LOGI("FIN", "caracterizacion strp>. %d", pwm_output);
        }
        if (cnt1 >= 6) {
            cnt1 = 5;
        }
        
        if(cnt1 >= 5){
            pwm_output = 1;
            ESP_LOGI("FIN", "caracterizacion finalizada");
        }
        
        #else
            if (carcnt++ >= 6000) {
                carcnt = 0;
                pwmtmp += 50;
                //pwm_output += 25;
                ESP_LOGI("Caract", "pwmtmp: %d", pwmtmp);
            }

            if (pwmtmp > 0){
                pwm_output = pwmtmp;
            }else{
                pwm_output = 1;
            }
            if (pwm_output > 1000) {
                return 1;
            }
        #endif 
    
    #else
        //ESP_LOGI("CONTROL", "presion: %0.2f, presionObjetivo: %0.2f", presion, presion_objetivo);
        // Calcular la salida del control PID
        pwm_output = control_pid(presion, (float)setpointPresion);
        //ESP_LOGI("CONTROL", "PWM: %d", pwm_output);
        
    #endif
        //ESP_LOGI("PWM", "PWM: %d", pwm_output);
        return pwm_output;
    }

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

        
        // Calcular la salida del PID
        float salida_pid = P + I + D;
        ESP_LOGI("CONTROL", "error: %0.2f,  P: %0.2f, I: %0.2f, D: %0.2f, PID: %0.2f", error, P, I, D, salida_pid);
    


        int16_t salida_pid_int = (int16_t)round(salida_pid);
        // Limitar la salida a los valores de PWM permitidos
        if (salida_pid_int < PWM_MIN) {
            salida_pid_int = PWM_MIN;
        } else if (salida_pid_int > PWM_MAX) {
            salida_pid_int = PWM_MAX;
        }
        //salida_pid_int = 1;
        return salida_pid_int;
    }

#endif



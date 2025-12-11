#include "control.h"

#ifdef control1

#define caract
//#define escalon




    #ifdef escalon

    // PRBS params
    #define N_BITS 9
    // taps (9,5): bits indexados 1..N
    #define TAPS_MASK ((1<<(9-1)) | (1<<(5-1)))

    static uint16_t lfsr_reg = 0x1FF; // semilla no nula

    static inline uint16_t next_lfsr_bit(uint16_t reg) {
        uint16_t x = reg & TAPS_MASK;
        uint8_t fb = 0;
        while (x) { fb ^= (x & 1); x >>= 1; }
        return fb & 0x1;
    }

    static inline uint16_t step_lfsr(uint16_t reg, int *out_bit) {
        uint16_t msb = (reg >> (N_BITS-1)) & 1u;
        uint16_t fb = next_lfsr_bit(reg);
        reg = ((reg << 1) & ((1u<<N_BITS)-1)) | fb;
        if (reg == 0) reg = 1;
        *out_bit = msb;
        return reg;
    }

    // niveles duty
    static float duty_min = 0.25f;
    static float duty_max = 0.60f;
    static float duty_levels[2];

    #define delta 100
        uint16_t puntos[]={180, 550, 180, 800, 180, 1};
        uint8_t cnt1=0;
    #endif

    #ifdef caract
        uint16_t carcnt = 0;
        int16_t pwmtmp = 0;
    #endif

    /**
     * brief: Funcion para calcular el control PID
     * param: setpointPresion: presion objetivo del sistema
     * param: presion: presion actual del sistema
     * return: salida del control PID
     */
    int16_t controller(uint8_t setpointPresion, float presion, float flow) {
        int16_t pwm_output;
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
                if (carcnt++ >= 3000) {
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

#endif



#include "control.h"
#include "lut.h"
#include <math.h> 

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
    int16_t controller(float setpointPresion, float presion, float flow) {
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


#ifdef control2
/*------------ Ganancias PID ------------*/
// Integral
#define Kpi   15.0f    

// Derivativo 
#define Kpd   1.5f           

// Proporcionales 
#define Kpp_BASE    5.0f     
#define Kpp_BOOST   10.0f 

/*------------ D de Flujo ------------*/
#define KdQ  0.35f

/*---------- Zona muerta ----------*/
#define DEADZONE_PRESSURE 0.00f  //0.30f

/*------------ FILTROS DE RUIDO ------------*/
#define D_FILTER_ALPHA  0.94f 
#define Q_FILTER_ALPHA  0.94f  

/*------------ Límites Integrador (CORRECCIÓN CRÍTICA) ------------*/
#define PWM_MAX_INTEGRAL_POS  30.0f 
#define PWM_MAX_INTEGRAL_NEG  -10.0f   

#define I_LIMIT_POS     (PWM_MAX_INTEGRAL_POS / Kpi)
#define I_LIMIT_NEG     (PWM_MAX_INTEGRAL_NEG / Kpi)

/*------------ Límites Actuador ------------*/
#define U_MIN   1.0f 
#define U_MAX   100.0f


// Variables estáticas para los filtros
static float last_pressure = 0.0f; 
static float dp_filtered = 0.0f; 
static float last_flow = 0.0f;
static float dq_filtered = 0.0f;

uint8_t flag = 0;

static inline float clamp(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

int16_t controller(float setpointPresion, float presion, float flow)
{
    float u = 0.0f;
    static float integral = 0.0f;
    float tmp = lookup_table_get(&lut_p,setpointPresion);
    float sp = setpointPresion + tmp; // Compensación por fricción estática

    // 1. FEEDFORWARD
    //uff = a0 + a1*p + a2*q + a3*p**2 + a4*q**2 + a5*p*q (lpos)
    //a0 = 3.450217, a1 = 2.612286, a2 = 18.733521, a3 = -0.004993, a4 = 3.129878, a5 = 0.431910
    float a0 = 3.450217, a1 = 2.612286, a2 = 18.733521, a3 = -0.004993, a4 = 3.129878, a5 = 0.431910;
    float flow_ff = (flow < 0.0f) ? 0.0f : flow/60; // Convertir a L/s
    //float uff = (flow_ff + (KTP * sp)) / KTU;
    float uff = a0 +a1*sp + a2*flow_ff + a3*sp*sp + a4*flow_ff*flow_ff + a5*sp*flow_ff;
    // 2. ERROR
    float ep = (sp - presion);
    
    // 3. DERIVATIVO DE FLUJO
    float dq_raw = (flow - last_flow) / DT;
    dq_filtered = (Q_FILTER_ALPHA * dq_filtered) + ((1.0f - Q_FILTER_ALPHA) * dq_raw);
    //printf("dq_raw: %.2f, dq_filt: %.2f\n", dq_raw, dq_filtered);
    last_flow = flow;
    
    float ufd = KdQ * dq_filtered;

    /*------------------ Veto UFD ------------------*/
    if(fabsf(ep)< DEADZONE_PRESSURE) { // Zona muerta para evitar oscilaciones
       ufd = 0.0f;
    }
    else
    {
        if (presion > (float)setpointPresion && ufd > 0.0f) {
            ufd = 0.0f;
        }
        if (presion<(float)setpointPresion && ufd < 0.0f) {
            ufd = 0.0f;
        }
    }
    ufd = clamp(ufd, -15.0f, 25.0f); // Limitar la influencia del ufd
        
    // 4. PROPORCIONAL DINÁMICO
    float current_Kpp;
    if(fabsf(ep)<DEADZONE_PRESSURE){
        current_Kpp = Kpp_BASE;
    } else {
        current_Kpp = Kpp_BASE + (fabsf(ep) * Kpp_BOOST);
    }
    float upp = current_Kpp * ep;    

    // 5. DERIVATIVO DE PRESIÓN 
    float dp_raw = (presion - last_pressure) / DT;
    dp_filtered = (D_FILTER_ALPHA * dp_filtered) + ((1.0f - D_FILTER_ALPHA) * dp_raw);
    float upd;
    if(fabsf(ep)<DEADZONE_PRESSURE){
        upd = 0.0f;
    } else {
        upd = -Kpd * dp_filtered;
    }
    last_pressure = presion; 

    // 6. INTEGRAL

    //float I_term = pi->integral_prev + (pi->Ki * pi->Ts / 2.0f) * (error + pi->error_prev);
    float u_tentativa = uff + ufd + upp + upd + (Kpi * integral);
    
    uint8_t saturado_max = (u_tentativa >= U_MAX && ep > 0);
    uint8_t saturado_min = (u_tentativa <= U_MIN && ep < 0);
    
    if (!saturado_max && !saturado_min) {
        integral += ep * DT;
    }

    integral = clamp(integral, I_LIMIT_NEG, I_LIMIT_POS);
    float upi = 0;
    float upi_calc = Kpi * integral;
    if (ep < 0.0f && upi > 0.0f) {
        upi = 0.0f; // Evitar que la integral sume en sentido negativo
    } else if (ep > 0.0f && upi < 0.0f) {
        upi = 0.0f; // Evitar que la integral sume en sentido positivo
    } else {
        upi =upi_calc;
    }

    // 7. SALIDA TOTAL
    u = uff + ufd + upp + upi + upd;
    u = clamp(u, U_MIN, U_MAX);

    // 8. LOGGING
    if (flag++ >= 4){
        flag = 0;
        // printf("> P:%.2f, Q:%.2f, U:%.2f\n",
        //         presion,flow/10.0f,   u/10  ); 
        // printf("> P:%.2f, Q:%.2f, U:%.2f, Kp:%.1f, uff:%.2f, upp:%.2f, upi_calc:%.2f, upi:%.2f, upd:%.2f, ufd:%.2f\n",
        //         (presion-tmp),flow/10.0f,u/10.0f,current_Kpp,uff,upp,      upi_calc,      upi,      upd,      ufd); 
        // printf("> P:%.2f, Q:%.2f, U:%.2f, uff:%.2f, upp:%.2f, upi:%.2f, upd:%.2f, ufd:%.2f\n",
        //         (presion-tmp),flow/10.0f,u/10.0f,uff,upp,     upi,      upd,      ufd); 
    }        
    return (uint16_t)lrintf(u * 10.0f); 
}
#endif


#ifdef control3

typedef enum {
    STATE_INSP = 0,     // Fase Inspiratoria
    STATE_BRAKE = 1,    // Frenado Activo (Transición)
    STATE_EXP = 2       // Fase Expiratoria / Espera de Trigger
} CPAP_State;

#define BRAKE_DURATION_TICKS 200  // 1000ms de frenado activo a 100Hz
#define Q_DROP_PERCENTAGE 0.90f // Disparo al caer al 80% del pico
#define SLOPE_BRAKE_THRESHOLD -10.0f // Sensibilidad de la pendiente dQ/dt

/*------------ Ganancias PID ------------*/
#define Kpi   17.0f    // Integral
#define Kpd   1.5f     // Derivativo           
#define Kpp   7.0f     // Proporcional

/*------------ D de Flujo ------------*/
#define KdQ  0.1f

/*---------- Zona muerta ----------*/
#define DEADZONE_PRESSURE 0.00f  //0.30f

/*------------ Límites Integrador (CORRECCIÓN CRÍTICA) ------------*/
#define PWM_MAX_INTEGRAL_POS  30.0f 
#define PWM_MAX_INTEGRAL_NEG  -10.0f   

#define I_LIMIT_POS     (PWM_MAX_INTEGRAL_POS / Kpi)
#define I_LIMIT_NEG     (PWM_MAX_INTEGRAL_NEG / Kpi)

/*------------ Límites Actuador ------------*/
// #define U_MIN   1.0f 
#define U_MIN   0.5f 
#define U_MAX   100.0f

/*------------ FILTROS DE RUIDO ------------*/
#define D_FILTER_ALPHA  0.94f 
#define Q_FILTER_ALPHA  0.94f  

// Variables estáticas para los filtros
static float last_pressure = 0.0f; 
static float last_flow = 0.0f;
static float dp_filtered = 0.0f; 
static float dq_filtered = 0.0f;

uint8_t flag = 0;

static inline float clamp(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

int16_t controller(float setpointPresion, float presion, float flow)
{
    static CPAP_State currentState = STATE_INSP;
    static float Q_peak = 0.0f;
    static int brakeCounter = 0;
    static float integral = 0.0f;

    float u = 0.0f;
    float tmp = lookup_table_get(&lut_p,setpointPresion);
    float sp_nominal = (float)setpointPresion + tmp; 
    float sp_active = sp_nominal;                   // Referencia real para el PID

    // 1. CÁLCULO DE DERIVADA
    float dq_raw = (flow - last_flow) / DT;
    last_flow = flow;
    dq_filtered = (Q_FILTER_ALPHA * dq_filtered) + ((1.0f - Q_FILTER_ALPHA) * dq_raw);
    float uqd = KdQ * dq_filtered;

    // 2. LÓGICA DE TRANSICIÓN DE LA FSM
    switch (currentState) {
        case STATE_INSP:
            if (flow > Q_peak) Q_peak = flow; // Seguimiento del pico

            // Condición de Predicción: ¿Caída repentina de flujo?
            if (flow < (Q_DROP_PERCENTAGE * Q_peak) && dq_filtered < SLOPE_BRAKE_THRESHOLD) {
                brakeCounter = 0;
                integral = 0.0f;
                //sp_active = sp_nominal - 1.0f; 
                currentState = STATE_BRAKE;
                // Reset de integral para evitar "windup" durante el pico de presión exhalatoria
                // integral *= 0.2f; 
            }
            break;

        case STATE_BRAKE:
            brakeCounter++;

            // ESTRATEGIA: "Bajar el setpoint"
            // Reducimos el setpoint activo 1.0 cmH2O por debajo del nominal para forzar el frenado

            // CONDICIÓN DE SALIDA: Presión cae por debajo de (sp_nominal - 0.5)
            // Se incluye un timeout de seguridad de 200ms (20 ticks)
            if (presion < (sp_nominal - 0.2f)  && (flow < Q_peak/2.0f))//|| brakeCounter >= BRAKE_DURATION_TICKS) 
            {
                integral = 0.0f; //
                sp_active = sp_nominal + 1.5f; // Restauramos setpoint nominal para la espiración
                currentState = STATE_EXP;
            }
            else if ((brakeCounter >= BRAKE_DURATION_TICKS) && (flow> Q_peak)) 
            { 
                Q_peak = 0.0f;
                // Timeout de seguridad: Si el frenado se extiende demasiado, forzamos la transición
                integral = 0.0f; // Reset de integral para evitar acumulación excesiva
                currentState = STATE_INSP;
            }
            break;

        case STATE_EXP:
            // Trigger Inspiratorio: El paciente vuelve a demandar flujo
            if (dq_filtered > 5.0f && flow > 15.0f) { 
                Q_peak = 0.0f;
                sp_active = sp_nominal;
                currentState = STATE_INSP;
            }
            break;
    }

    // 3. CÁLCULO DE ACCIÓN DE CONTROL SEGÚN EL ESTADO
    float a0 = 3.450217, a1 = 2.612286, a2 = 18.733521, a3 = -0.004993, a4 = 3.129878, a5 = 0.431910;
    float flow_ff = (flow < 0.0f) ? 0.0f : flow/60.0f;
    float uff = a0 + a1*sp_active + a2*flow_ff + a3*sp_active*sp_active + a4*flow_ff*flow_ff + a5*sp_active*flow_ff;

    float upp = 0.0f;
    float upi = 0.0f;
    float upd = 0.0f;


    float ep = (sp_active - presion);

    // Proporcional
    upp = Kpp * ep;



//     /*------------ FILTROS DE RUIDO ------------*/
// #define D_FILTER_ALPHA  0.94f 
// #define Q_FILTER_ALPHA  0.94f  

// // Variables estáticas para los filtros
// static float last_pressure = 0.0f; 
// static float last_flow = 0.0f;

    // // 5. DERIVATIVO DE PRESIÓN 
    // float dp_raw = (presion - last_pressure) / DT;
    // dp_filtered = (D_FILTER_ALPHA * dp_filtered) + ((1.0f - D_FILTER_ALPHA) * dp_raw);
    // float upd;
    // if(fabsf(ep)<DEADZONE_PRESSURE){
    //     upd = 0.0f;
    // } else {
    //     upd = -Kpd * dp_filtered;
    // }
    // last_pressure = presion; 


    // Derivativo de Presión
    float dp_raw = (presion - last_pressure) / DT;
    dp_filtered = (D_FILTER_ALPHA * dp_filtered) + ((1.0f - D_FILTER_ALPHA) * dp_raw);
    last_pressure = presion;
    upd = (fabsf(ep) < DEADZONE_PRESSURE) ? 0.0f : -Kpd * dp_filtered;

    // Integral con Anti-windup
    if (!((uff + upp + upd + (Kpi * integral) >= U_MAX && ep > 0) || 
            (uff + upp + upd + (Kpi * integral) <= U_MIN && ep < 0))) {
        integral += ep * DT;
    }
    integral = clamp(integral, I_LIMIT_NEG, I_LIMIT_POS);
    upi = Kpi * integral;    

    if (currentState == STATE_BRAKE) {
        uff *= 0.75f; // Durante el frenado activo, anulamos la salida para maximizar la caída de presión
        // uff *= 0.3f; // Reducción adicional durante el frenado activo
        if (upd > 0.0f) upd *= -1.0f; // Atenuación del derivativo si va en dirección de aumentar presión
        if (uqd > 0.0f) uqd *= -1.0f; // Evitar que la integral sume durante el frenado
    }

    // accion total
    u = uff + upp + upi + upd + uqd;
    u = clamp(u, U_MIN, U_MAX);

    // 8. LOGGING
    // flag = !flag;
    if (flag++ >= 4){
        flag = 0;
        // printf("> P:%.2f, Q:%.2f, U:%.2f\n",
        //   presion,flow/10.0f,   u/10  ); 
        // printf("> P:%.2f, Q:%.2f, U:%.2f, Kp:%.1f, uff:%.2f, upp:%.2f, upi_calc:%.2f, upi:%.2f, upd:%.2f, ufd:%.2f\n",
        //          (presion-tmp),flow/10.0f,u/10.0f,current_Kpp,uff,upp,      upi_calc,      upi,      upd,      ufd); 
        // printf("> P:%.2f, Q:%.2f, U:%.2f, uff:%.2f, upp:%.2f, upi:%.2f, upd:%.2f, uqd:%.2f, dq:%.2f, Q_peak:%.2f, state:%u \n",
                // (presion-tmp),flow/10.0f,u/10.0f,uff,upp,     upi,      upd,      uqd,      dq_filtered,      Q_peak,      currentState); 
    }        
    return (uint16_t)lrintf(u * 10.0f); 
}

#endif
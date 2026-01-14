#include "control.h"
#include "lut.h"
#include <math.h> 

#ifdef control2

/* ---------- Configuración ---------- */
#define SAMPLE_HZ          100.0f
#define DT                 0.01f

/* ------------ Modelo Turbina ------------ */
#define KTU         1.468543f 
#define KTP         3.692028f

/*------------ Ganancias PID ------------*/
// Integral
#define Kpi   25.0f          // 15 // 12.0f  Subimos un poco para recuperar más rápido

// Derivativo 
#define Kpd   0.045f           //0.045   0.9   1.8  Antes 1.5f  

// Proporcionales 
#define Kpp_BASE    6.0f     // 5
#define Kpp_BOOST   1.0f    //25   10 

/*------------ Anticipación de Flujo ------------*/
#define KdQ   0.02f   //0.2 0.35f

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

/*---------- Zona muerta ----------*/
#define DEADZONE_PRESSURE 0.00f  //0.30f
// Variables estáticas para los filtros
static float last_pressure = 0.0f; 
static float dp_filtered = 0.0f; 
static float last_flow = 0.0f;
static float dq_filtered = 0.0f;

uint8_t flag = 0;

static inline float clamp(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

int16_t controller(uint8_t setpointPresion, float presion, float flow)
{
    float u = 0.0f;
    static float integral = 0.0f;
    float tmp = lookup_table_get(&lut_p,setpointPresion);
    float sp = (float)setpointPresion + tmp; // Compensación por fricción estática

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
    //if(fabsf(ep)< DEADZONE_PRESSURE) { // Zona muerta para evitar oscilaciones
    //    ufd = 0.0f;
    //}
    //else
    // {
    //     if (presion > (float)setpointPresion && ufd > 0.0f) {
    //         ufd = 0.0f;
    //     }
    //     if (presion<(float)setpointPresion && ufd < 0.0f) {
    //         ufd = 0.0f;
    //     }
    // }
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
    // if(fabsf(ep)<DEADZONE_PRESSURE){
    //     upd = 0.0f;
    // } else {
        upd = -Kpd * dp_filtered;
    // }
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
    flag = !flag;
    if (flag){
        // printf("> P:%.2f, Q:%.2f, U:%.2f\n",
        //   presion,flow/10.0f,   u  ); 
        // printf("> P:%.2f, Q:%.2f, U:%.2f, Kp:%.1f, uff:%.2f, upp:%.2f, upi_calc:%.2f, upi:%.2f, upd:%.2f, ufd:%.2f\n",
        //          (presion-tmp),flow/10.0f,u/10.0f,current_Kpp,uff,upp,      upi_calc,      upi,      upd,      ufd); 
        printf("> P:%.2f, Q:%.2f, U:%.2f, uff:%.2f, upp:%.2f, upi:%.2f, upd:%.2f, ufd:%.2f\n",
                (presion-tmp),flow/10.0f,u/10.0f,uff,upp,     upi,      upd,      ufd); 
    }        
    return (uint16_t)lrintf(u * 10.0f); 
}

#endif
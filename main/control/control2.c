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

/*------------ Ganancias PID (Reajuste Estratégico) ------------*/
#define Kpi   10.0f           

// CAMBIO SOLICITADO: Bajamos la ganancia derivativa de presión.
// Ya no necesitamos que sea tan agresiva porque la derivada de flujo (KdQ)
// está haciendo el trabajo pesado de anticipación.
// Esto eliminará la oscilación nerviosa en 'upd'.
#define Kpd   4.0f            // Antes 10.0

/*------------ Ganancias Proporcionales Dinámicas ------------*/
#define Kpp_BASE    4.0f      
#define Kpp_BOOST   12.0f      

/*------------ Anticipación de Flujo (El Nuevo "Líder") ------------*/
// Mantenemos esta ganancia. Ahora es el actor principal para la respuesta rápida.
#define KdQ   0.30f   

/*------------ FILTROS DE RUIDO ------------*/
// Mantenemos los filtros altos para asegurar suavidad total.
#define D_FILTER_ALPHA  0.90f 
#define Q_FILTER_ALPHA  0.90f  

/*------------ Límites Integrador ------------*/
#define PWM_MAX_INTEGRAL_POS  30.0f 
#define PWM_MAX_INTEGRAL_NEG  0.0f   

#define I_LIMIT_POS     (PWM_MAX_INTEGRAL_POS / Kpi)
#define I_LIMIT_NEG     (PWM_MAX_INTEGRAL_NEG / Kpi)

/*------------ Límites Actuador ------------*/
#define U_MIN   1.0f 
#define U_MAX   100.0f

uint8_t flag = 0;
// Variables estáticas para los filtros
static float last_pressure = 0.0f; 
static float dp_filtered = 0.0f; 

static float last_flow = 0.0f;
static float dq_filtered = 0.0f;

static inline float clamp(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

int16_t controller(uint8_t setpointPresion, float presion, float flow)
{
    float u = 0.0f;
    static float integral = 0.0f;
    
    // 1. FEEDFORWARD
    float flow_ff = (flow < 0.0f) ? 0.0f : flow;
    float uff = (flow_ff + (KTP * (float)setpointPresion)) / KTU;
    
    // 2. ANTICIPACIÓN DE FLUJO (El Motor Principal de Reacción)
    float dq_raw = (flow - last_flow) / DT;
    // Filtro suave
    dq_filtered = (Q_FILTER_ALPHA * dq_filtered) + ((1.0f - Q_FILTER_ALPHA) * dq_raw);
    last_flow = flow;
    
    float u_anticipacion = KdQ * dq_filtered;

    // 3. ERROR
    float ep = ((float)setpointPresion - presion);
            
    // 4. PROPORCIONAL DINÁMICO
    float current_Kpp = Kpp_BASE + (fabsf(ep) * Kpp_BOOST);
    float upp = current_Kpp * ep;    

    // 5. DERIVATIVO DE PRESIÓN (El Amortiguador)
    float dp_raw = (presion - last_pressure) / DT;
    // Filtro suave
    dp_filtered = (D_FILTER_ALPHA * dp_filtered) + ((1.0f - D_FILTER_ALPHA) * dp_raw);
    
    // Ahora 'upd' será mucho más pequeño y estable, actuando solo para evitar sobreimpulsos
    float upd = -Kpd * dp_filtered;
    last_pressure = presion; 

    // 6. INTEGRAL
    float u_tentativa = uff + u_anticipacion + upp + upd + (Kpi * integral);
    
    uint8_t saturado_max = (u_tentativa >= U_MAX && ep > 0);
    uint8_t saturado_min = (u_tentativa <= U_MIN && ep < 0);
    
    if (!saturado_max && !saturado_min) {
        integral += ep * DT;
    }
    
    integral = clamp(integral, I_LIMIT_NEG, I_LIMIT_POS);
    float upi = Kpi * integral;

    // 7. SALIDA TOTAL
    u = uff + u_anticipacion + upp + upi + upd;
    u = clamp(u, U_MIN, U_MAX);

    // 8. LOGGING
    flag = !flag;
    if (flag){
        printf("> P:%.2f, Q:%.2f, U:%.2f, Kp:%.1f, upi:%.2f, upd:%.2f, udq:%.2f\n",
                  presion, flow, u, current_Kpp, upi, upd, u_anticipacion); 
    }        
    return (uint16_t)lrintf(u * 10.0f); 
}

#endif
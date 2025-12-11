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

/*------------ Ganancias PID (Modo Robusto) ------------*/
#define Kpi   12.0f           // 12.0f  Subimos un poco para recuperar más rápido

// CORRECCIÓN 1: Restauramos el Paracaídas.
// Necesitamos este valor para frenar la caída de presión en picada.
// Con el filtro alto (0.94), 3.0 es un valor seguro y estable.
#define Kpd   1.5f          // Antes 3.0f  

/*------------ Ganancias Proporcionales Dinámicas ------------*/
// Subimos un poco la base para que el sistema se sienta más "rígido" y no tan suelto.
#define Kpp_BASE    5.0f     // 3 Antes 2.0
#define Kpp_BOOST   10.0f     // 5 Antes 14.0

/*------------ Anticipación de Flujo ------------*/
#define KdQ   0.350f   

/*------------ FILTROS DE RUIDO (Mantener Suavidad) ------------*/
// Mantenemos el filtrado agresivo para que el Kpd de 3.0 no meta ruido.
#define D_FILTER_ALPHA  0.94f 
#define Q_FILTER_ALPHA  0.94f  

/*------------ Límites Integrador (CORRECCIÓN CRÍTICA) ------------*/
#define PWM_MAX_INTEGRAL_POS  30.0f 

// CORRECCIÓN 2: Tapamos el "Pozo".
// Volvemos a 0.0. Permitir valores negativos estaba haciendo que el integrador
// restara potencia justo cuando más la necesitábamos (al inicio de la inhalación).
#define PWM_MAX_INTEGRAL_NEG  -10.0f   

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
    
    // 2. ANTICIPACIÓN DE FLUJO
    float dq_raw = (flow - last_flow) / DT;
    dq_filtered = (Q_FILTER_ALPHA * dq_filtered) + ((1.0f - Q_FILTER_ALPHA) * dq_raw);
    last_flow = flow;
    
    float u_anticipacion = KdQ * dq_filtered;

    /* ----- NUEVA LÓGICA: VETO DE SOBREPRESIÓN ----- */
    // Si la Presión > Setpoint (tenemos exceso), PROHIBIMOS que udq sume potencia.
    // Esto evita que ruidos de flujo o rebotes inflen la presión innecesariamente.
    // Nota: Si udq es negativo (frenado), SIEMPRE lo permitimos porque ayuda a bajar la presión.
    if (presion > (float)setpointPresion && u_anticipacion > 0.0f) {
        u_anticipacion = 0.0f;
    }
    if (presion<(float)setpointPresion && u_anticipacion < 0.0f) {
        u_anticipacion = 0.0f;
    }
    /* ----- NUEVO: LIMITADOR DE ACELERACIÓN ----- */
    // Evitamos el "Efecto Látigo". Limitamos la ayuda positiva a un máximo de 25 puntos.
    // Esto permite arrancar rápido sin disparar la presión al cielo.
    if (u_anticipacion > 25.0f) {
        u_anticipacion = 25.0f;
    }


    // 3. ERROR
    float ep = ((float)setpointPresion - presion);
            
    // 4. PROPORCIONAL DINÁMICO
    float current_Kpp = Kpp_BASE + (fabsf(ep) * Kpp_BOOST);
    float upp = current_Kpp * ep;    

    // 5. DERIVATIVO DE PRESIÓN (Restaurado)
    float dp_raw = (presion - last_pressure) / DT;
    dp_filtered = (D_FILTER_ALPHA * dp_filtered) + ((1.0f - D_FILTER_ALPHA) * dp_raw);
    
    // El "Paracaídas" está activo de nuevo para evitar el crash de presión
    float upd = -Kpd * dp_filtered;
    last_pressure = presion; 

    // 6. INTEGRAL
    float u_tentativa = uff + u_anticipacion + upp + upd + (Kpi * integral);
    
    uint8_t saturado_max = (u_tentativa >= U_MAX && ep > 0);
    uint8_t saturado_min = (u_tentativa <= U_MIN && ep < 0);
    
    if (!saturado_max && !saturado_min) {
        integral += ep * DT;
    }
    
    // Clamp crítico: Nunca dejamos que sea menor a 0.0
    integral = clamp(integral, I_LIMIT_NEG, I_LIMIT_POS);
    float upi = Kpi * integral;

    // 7. SALIDA TOTAL
    u = uff + u_anticipacion + upp + upi + upd;
    u = clamp(u, U_MIN, U_MAX);

    // 8. LOGGING
    flag = !flag;
    if (flag){
        printf("> P:%.2f, Q:%.2f, U:%.2f, Kp:%.1f, uff:%.2f, upp:%.2f, upi:%.2f, upd:%.2f, udq:%.2f\n",
                  presion, flow, u, current_Kpp, uff, upp, upi, upd, u_anticipacion); 
    }        
    return (uint16_t)lrintf(u * 10.0f); 
}

#endif
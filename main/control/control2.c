#include "control.h"

#ifdef control2

/* ---------- Compile‑time configuration ---------- */
#define SAMPLE_HZ          100.0f          // Control loop frequency (Hz)
#define DT                 (1.0f/SAMPLE_HZ)

/* ---------- Pneumatic model (nominal) ---------- */
static const float Ku        = 0.1796f;      // (L s⁻¹)/(% PWM)1.796f; 90.0 supone un dead band de 10%

static const float C_L       = 0.25f;      // Compliance (L/cmH₂O)     // 0.8f
static const float R_aw      = 5.0f;       // Airway resistance ((cmH₂O·s)/L)    // 3.0f
static const float kL        = 0.05f;      // Linear leak (L s⁻¹/cmH₂O)
static const float tau       = (R_aw + kL) * C_L;   // Time constant 0.7625

/* IMC tuning (λ = 0.3 τ) */
static const float lambda_imc = 0.3f * tau;
static const float Kc         = (0.5f * tau) / (lambda_imc * Ku * C_L);//0.5
static const float Ti         = 5.0f * tau;                 // 3 Integrator time
static const float Kd         = 0.01f * Kc * Ti;           // Derivative gain if desired

/* ---------- Globals ---------- */
static volatile float PEAP_cmH2O  = 0.0f;      // Ambient gauge reference
/* Control state */
static float error = 0.0f;
static float e_prev = 0.0f; // Previous error for derivative
static float up = 0.0f;            // Proportional term
static float ui = 0.0f;            // Integrator term
static float ud = 0.0f;            // Derivative term
static float ud_p[5] = {0.0f};

/* ---------- Helper functions ---------- */
static inline float clamp(float x, float lo, float hi)
{
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

static float ud_filter(float input)
{
    float temp[5];

    for (uint8_t i = 0; i < 4; i++){
        ud_p[i] = ud_p[i+1];
        temp[i] = ud_p[i];
    }
    ud_p[4] = input;
    temp[4] = ud_p[4];
    
    // compute median
    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = i + 1; j < 5; j++) {
            if (temp[i] > temp[j]) {
                float t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }

    return temp[2];
}

/**
 * Controller function
 * @param setpointPresion: Desired pressure setpoint
 * @param presion: Current pressure reading
 * @param flow: Current flow reading
 * @return PWM value for the BLDC motor
 */
uint16_t controller(uint8_t setpointPresion, float presion, float flow)
{
    /* === Feedback IMC‑PID === */

    float u = 0.0f;           // Control signal
    error = setpointPresion - presion;

    // Proportional term
    up = Kc * error;
    //float up = Kc * (error + e_prev);
    
    // Integral term
    float u_unsat = up + ui + ud;
    float u_sat   = clamp(u_unsat, 5.0f, 100.0f);
    ui += (Kc*DT/Ti)*error + (DT/(Ti/Kc))*(u_sat - u_unsat);
    // if ((u_unsat >= 100.0f && error > 0.0f) || (u_unsat <= 5.0f && error < 0.0f)){
    //     ui += 0.0f; // Anti‑windup
    // }else
    //     ui += (Kc * DT / Ti) * error;
    ui = clamp(ui, -30.0f, 30.0f);            // Anti‑windup

    // Derivative term
    ud = 0.0f * Kd;
    ud = Kd * (error - e_prev) / DT;
    //ud = clamp(ud, -20.0f, 20.0f);
    //ud = ud_filter(ud);
    
    u = up + ui + ud;
    
    /* === Actuator saturation & apply === */
    u = clamp(u, 3.0f, 100.0f);
    e_prev = error;

    printf("> P:%f, U:%f, Up:%f, Ui:%f, Ud:%f\n",presion, (u/10.0f), up, ui, ud);
    return (uint16_t)lrintf(u * 10.0f); // Return PWM value in [0, 1000]

}

#endif // control2
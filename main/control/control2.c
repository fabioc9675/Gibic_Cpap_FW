#include "control.h"

#ifdef control2

/* ---------- Compile‑time configuration ---------- */
#define SAMPLE_HZ          100.0f          // Control loop frequency (Hz)
#define DT                 (1.0f/SAMPLE_HZ)

/* ---------- Pneumatic model (nominal) ---------- */
static const float Ku        = 0.45f;      // (L s⁻¹)/(% PWM)
//static const float Ku_inv    = 1.0f/Ku;
static const float C_L       = 0.25f;      // Compliance (L/cmH₂O)     // 0.8f
static const float R_aw      = 3.0f;       // Airway resistance ((cmH₂O·s)/L)    // 3.0f
static const float kL        = 0.02f;      // Linear leak (L s⁻¹/cmH₂O)
//static const float leak_exp  = 1.0f;       // Exponent n in Q_L = kL ΔPⁿ
static const float tau       = (R_aw + kL) * C_L;   // Time constant

/* IMC tuning (λ = 0.3 τ) */
static const float lambda_imc = 0.3f * tau;
static const float Kc         = (0.5f *tau) / (lambda_imc * Ku * C_L);//0.5
static const float Ti         = 20.0f * tau;                 // 20 Integrator time
//static const float Kd         = 0.2f*Kc*Ti;           // Derivative gain if desired

/* ---------- Globals ---------- */
static volatile float PEAP_cmH2O  = 0.0f;      // Ambient gauge reference
/* Control state */
static float ui = 0.0f;     // Integrator term
static float e_prev = 0.0f; // Previous error for derivative

/* ---------- Helper functions ---------- */
static inline float clamp(float x, float lo, float hi)
{
    return (x < lo) ? lo : (x > hi) ? hi : x;
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
    float error = setpointPresion - presion;

    ui += (Kc * DT / Ti) * error;
    printf("ui: %f\n", ui);
    ui = clamp(ui, -200.0f, 100.0f);            // Anti‑windup

    float ud = 0.0f;
    //if (Kd > 0.0f)
    //    ud = Kd * (error - e_prev) / DT;
    e_prev = error;
    //float u = u_ff + Kc * error + ui; //+ ud;
    float u = Kc * error + ui + ud;

    /* === Actuator saturation & apply === */
    u = clamp(u, 10.0f, 100.0f);
     
    return (uint16_t)roundf(u * 10.0f); // Return PWM value in [0, 1000]
}

#endif // control2
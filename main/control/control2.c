#include "control.h"
#ifdef control2

//static filter_t *fl_dif = NULL; // Declaración global

/* ---------- Compile‑time configuration ---------- */
#define SAMPLE_HZ          100.0f          // Control loop frequency (Hz)
#define DT                 (1.0f/SAMPLE_HZ)

/* ---------- Pneumatic model (nominal) ---------- */
static const float Ku        = 0.1796f;      // (L s⁻¹)/(% PWM)1.796f; 90.0 supone un dead band de 10%

static const float C_L       = 0.30f;      // Compliance (L/cmH₂O)     // 0.8f
static const float R_aw      = 5.0f;       // Airway resistance ((cmH₂O·s)/L)    // 3.0f
static const float kL        = 0.05f;      // Linear leak (L s⁻¹/cmH₂O)
static const float tau       = (R_aw + kL) * C_L;   // Time constant 0.7625

/* IMC tuning (λ = 0.3 τ) */
static const float lambda_imc = 0.3f * tau;
static const float Kc         = (0.5f * tau) / (lambda_imc * Ku * C_L);//0.5
static const float Ti         = 3.0f * tau;                 // 3 Integrator time
static const float Kd         = 0.1f * Kc * Ti;           // Derivative gain if desired

/* ---------- Globals ---------- */
static volatile float PEAP_cmH2O  = 0.0f;      // Ambient gauge reference
/* Control state */
static float error = 0.0f;
static float e_prev = 0.0f; // Previous error for derivative
static float p_prev = 0.0f; // Previous pressure for derivative
static float up = 0.0f;            // Proportional term
static float ui = 0.0f;            // Integrator term
static float ud = 0.0f;            // Derivative term
static float ud_m[5] = {0.0f};     // mean derivative vector

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
    float u = 0.0f;           // Control signal
    error = setpointPresion - presion;

    // Proportional term
    up = Kc * error;
    
    // Integral term
    float u_unsat = up + ui + ud;
    float u_sat   = clamp(u_unsat, 5.0f, 100.0f);
    ui += (Kc*DT/Ti)*error + (DT/(Ti/Kc))*(u_sat - u_unsat);
    ui = clamp(ui, -25.0f, 25.0f);            // Anti‑windup

    // Derivative term
    // ud = 0.0f * Kd;
    ud = Kd * (p_prev - presion) / DT;
    //float median(float *data, float new_value, uint8_t size);
    ud = median(ud_m, ud, 5);
    
    // Combine terms
    u = up + ui + ud;

    // diff_fl = flow - tmp;
    // ddiff_fl = (diff_fl - diff_fl_p) / DT;
    // ddiff_fl = median(diff_fl_m, ddiff_fl, 5);
    // if (error > 0){ //presion por debajo del sp
    //     if ((diff_fl <= -3.0f) && (ddiff_fl <= -5.0f))
    //     {
    //          u = 2.0f; 
    //     } 
    // }
    
    // if(!flag){
    //     if((diff_fl < -1.5f) && ((error > 0.0f) && (error < 0.8f))) {
    //         printf("hola mundo\n");
    //         ui /= 2.0f;
    //         up /= 2.0f;
    //         flag = 1;
    //     }
    // }else{
    //     ui /= 2.0f;
    //     up /= 2.0f;
    //     if (error < 0){
    //         flag = 0;
    //     }
    // }


    /* === Actuator saturation & apply === */
    u = clamp(u, 3.0f, 100.0f);
    e_prev = error;
    p_prev = presion;

    //printf("> P:%f, U:%f, Up:%f, Ui:%f, Ud:%f\n",presion, (u/10.0f), up, ui, ud);
    printf("> P:%f, U:%f, Up:%f, Ui:%f, Ud:%f,",presion, (u/10.0f), up, ui, ud);
    printf("FL:%f \n",flow);
    return (uint16_t)lrintf(u * 10.0f); // Return PWM value in [0, 1000]
}

#endif // control2
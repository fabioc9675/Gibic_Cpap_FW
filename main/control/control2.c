#include "control.h"
#include "lut.h"
#ifdef control2

/* ---------- Compile‑time configuration ---------- */
#define SAMPLE_HZ          100.0f          // Control loop frequency (Hz)
#define DT                 (1.0f/SAMPLE_HZ)

/* ------------ Turbine model (nominal) ------------ */
//Qt(u,p) = KTU*u - KTP*p
#define KTU         1.468543f    // Ktu
#define KTP         3.692028f    // Ktp

/*------------ Mask leak model ------------*/
//Ql(p) = KLP*p + KLQ           1.547*p + 20.27
//#define KLP    1.547f
//#define KLQ    20.27f  // [L/min]

/*------------Controller terms------------*/
// --gain values
#define Kpp   4.5f           //1.0f           // Pressure proportional gain
#define Kpi   0.5f           // 1.0f           // Pressure Integral gain
#define Kpd   0.2f          //0.08f           // Pressure Derivative gain
#define Kfpi  0.35f          //1/Ktu             // Flow proportional inspiration gain
#define Kfpe  0.45f          //0.45f          // Flow proportional expiration gain

// --vars
float uff = 0.0f;            // feedforward term
float upp = 0.0f;            // Proportional pressure term
float ufp = 0.0f;            // Proportional flow term
float upi = 0.0f;            // Integrator pressure term
float upd = 0.0f;            // Derivative pressure term
float u = 0.0f;              // output term
float p_prev = 0.0f;         // previous pressure term
float q_prev = 0.0f;         // previous flow term
//float Qpi = 0.0f;            // patient flow
//float Qpe = 0.0f;          // patient flow

/*------- Constraints -------*/
//#define COR_D_S     6.0f    // no bajar más de 8 pts PWM por debajo de uff
#define COR_D      35.0f    // no bajar más de 8 pts PWM por debajo de uff
#define COR_U      35.0f    // no subir más de 15 por encima
#define UFP_D      25.0f    // flujo sólo puede quitar 6 pts PWM
#define UFP_U      25.0f    // y añadir 6 pts como máximo

// Gating de la integral
#define I_MIN           2.0f    // Límite absoluto de la integral
#define I_MAX           2.0f    // Límite absoluto de la integral

uint8_t flag =0;

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
int16_t controller(uint8_t setpointPresion, float presion, float flow)
{
    // Calculamos fuga en mascara en ss
    //float Ql_sp = KLP * setpointPresion + KLQ; // leak flow at setpoint
    float Ql_sp = lookup_table_get(&lut_p, (float)setpointPresion); // leak flow at setpoint from LUT
    // error pression
    float ep = ((float)setpointPresion - presion);
        
    // calculamos u para llevar a presion setpoint en ss
    uff = (Ql_sp + (KTP * setpointPresion))/KTU;

    // compensacion fuerte por flujo
    // estimacion flujo paciente
    // float Ql_pr = KLP * presion + KLQ; // fugas mascara presion actual
    float Ql_pr = lookup_table_get(&lut_p, presion); // leak flow at current pressure from LUT
    float Qp = flow - Ql_pr;           // flujo paciente actual 
    //float Qp = flow - Ql_sp;
    //if (fabs(ep) < 0.5f){
    //    ufp = 0.0f;
   // } else
    // if ((Qp > 0.0f || q_prev < Qp ) && ep > 0.0f){  // INSPIRACION
    //     if (fabsf(Qp) < 10.0f){
    //        // Qp = 10.0f;
    //     }
    //     Qpe = 0.0f;
    //     ufp = Kfpi * (fabsf(Qp)); // inspiracion, presion por debajo de setpoint
    // } else if ((Qp < 0.0f || q_prev > Qp) && ep < 0.0f){ // EXPIRACION
    //     if(Qpe == 0.0f){
    //         Qpe = fabsf(Qp);
    //     }
    //     if(Qpe > fabsf(Qp)){
    //         ufp = -Kfpe * (Qpe + fabsf(Qp)); 
    //     }else{
    //         ufp = -Kfpe * (fabsf(Qp));
    //     }
    // }else{
    //     ufp = 0.0f;
    // }
    if (Qp > 0.0f){ // inspiracion
        //if (ep >= 0.0f){
            ufp = Kfpi * Qp;
        //}else{
       //     ufp = 0.0f;
      // }
    }else{ // expiracion
        //if(ep <= 0.0f){
            ufp = Kfpe * Qp;
       // }else{
            //ufp = 0.0f;
       // }
    }
    ufp = clamp(ufp, -UFP_D, UFP_U);
    q_prev = Qp;
    
    // PID pressure controller
    // Proportional term
    upp = Kpp * ep ;    

    // Derivative term
    float dp_dt = (presion - p_prev) / DT;
    upd = Kpd * (-dp_dt);
    p_prev = presion;

    // Integral term
    float u_cor = 0.0f;
    if ((fabsf(ep) < 1.2f)){
        upi += Kpi * ep * DT;
        upi = clamp(upi, -I_MIN, I_MAX);
        if (fabsf(ep) < 0.5f){
            u_cor = upp + upi + upd + ufp;    
        }else{
            u_cor = upp + upd + ufp; 
        }  
    }else{
        u_cor = upp + upd + ufp; 
    }
    //u_cor = upp + upi + upd + ufp;
    u_cor = clamp(u_cor, -COR_D, COR_U);

    u = uff + u_cor;
    u = clamp(u, 3.0f, 100.0f);
    
//printf("> P:%f, U:%f, Up:%f, Ui:%f, Ud:%f\n",presion, (u/10.0f), up, ui, ud);
//     // printf("> P:%f, U:%f, Up:%f, Ui:%f, Ud:%f,",presion, (u/10.0f), up, ui, ud);
//     // printf("FL:%f \n",flow);
    flag = !flag;
    if (flag){
        printf("> P:%0.2f, Q:%0.2f, Qp:%0.2f, U:%0.2f, uff:%0.2f, ucor:%0.2f, upp:%0.2f, upi:%0.2f, upd:%0.2f, ufp:%0.2f \n",
            presion, flow, Qp, (u/1.0f), uff, u_cor, upp, upi, upd, ufp);
    }        
    return (uint16_t)lrintf(u * 10.0f); // Return PWM value in [0, 1000]
}

#endif // control2
#include "humidificador.h"
#include "driver/ledc.h"

// #define GPIO_OUTPUT_IO_17    17
// #define GPIO_OUTPUT_PIN_SEL  (1ULL << GPIO_OUTPUT_IO_17)

#define GPIO_PWM_17       17
#define LEDC_CANAL        LEDC_CHANNEL_0
#define LEDC_MODO         LEDC_LOW_SPEED_MODE
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_RESOLUCION   LEDC_TIMER_8_BIT // 0-255


// --------- NTC (calibrado) ----------
const float I_NTC = 101.44e-6;
const float R0    = 10341.0;
const float T0    = 298.15;
const float BETA  = 3576.0;

// // --------- Control ----------
#define N_MUESTRAS  50
float acumTemp = 0.0;
int muestraCount = 0;
const float HISTERESIS = 2.0;
// const unsigned long PERIODO_MS = 1000;

// float Tset = 0.0;
bool controlActivo = false;
// bool calefaccionON = false;

// // unsigned long tiempoAnterior = 0;

// void configurar_gpio() {
//     gpio_config_t io_conf = {
//         .intr_type = GPIO_INTR_DISABLE,        // Deshabilitar interrupciones
//         .mode = GPIO_MODE_OUTPUT,               // Configurar como salida
//         .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,    // Seleccionar el pin 17
//         .pull_down_en = 0,                      // Deshabilitar pull-down
//         .pull_up_en = 0                         // Deshabilitar pull-up
//     };
//     gpio_config(&io_conf);
// }

void configurar_gpio(){
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODO,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_RESOLUCION,
        .freq_hz          = 10000, 
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODO,
        .channel        = LEDC_CANAL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GPIO_PWM_17,
        .duty           = 0, // Inicia en 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void activar_pin() {
    // gpio_set_level(GPIO_OUTPUT_IO_17, 1);
    ledc_set_duty(LEDC_MODO, LEDC_CANAL, 5); // 25% duty cycle
    ledc_update_duty(LEDC_MODO, LEDC_CANAL);
}

void desactivar_pin() {
    // gpio_set_level(GPIO_OUTPUT_IO_17, 0);
    ledc_set_duty(LEDC_MODO, LEDC_CANAL, 0);
    ledc_update_duty(LEDC_MODO, LEDC_CANAL);
}

float temperaturaDesdeVoltaje(float Vntc) {
  if (Vntc <= 0.0001f) return -1000.0f;
  float R = Vntc / I_NTC;
  float invT = (1.0 / T0) + (1.0 / BETA) * log(R / R0);
  return (1.0 / invT) - 273.15;
}

void activarHumidificadorControl(void) {
    controlActivo = true;
}

void desactivarHumidificadorControl(void) {
    controlActivo = false;
    desactivar_pin();
}

void actualizarControl(float T, float Tset) {
    printf("Setpoint humidificador: %.2f C \t temperatura actual: %.2f C\n", Tset, T);
    if (!controlActivo) {
        desactivar_pin();
    } else {
        if (T <= (Tset - HISTERESIS)) {
            // if (!calefaccionON) {
                activar_pin();
            // calefaccionON = true;
                // printf("Humidificador ON\n");
            // }
        } else if (T >= (Tset - 1.5f)) {
            // if (calefaccionON) {
                desactivar_pin();
            // calefaccionON = false;
                // printf("Humidificador OFF\n");
            // }
        }
    }
}

void inicializarHumidificador() {
    muestraCount = 0;
    acumTemp = 0.0;
    configurar_gpio();
    desactivar_pin();
    activarHumidificadorControl();

}

void controlarHumidificador(float setpoint,float Vntc) {
    if (muestraCount < N_MUESTRAS) {
        acumTemp += Vntc;
        muestraCount++;
        return;
    }else{
        muestraCount = 0;
        float Vprom = acumTemp / N_MUESTRAS;
        float T = temperaturaDesdeVoltaje(Vprom); 
        // printf("temperatura humidificador: %.2f C \t Voltaje: %.2f\n", T, Vprom);
        acumTemp = 0.0;
        actualizarControl(T, setpoint);
    }
}
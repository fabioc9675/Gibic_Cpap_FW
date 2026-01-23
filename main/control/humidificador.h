#ifndef HUMIDIFICADOR_H
#define HUMIDIFICADOR_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "math.h"
#include "driver/gpio.h"

void controlarHumidificador(float setpoint,float Vntc);
void inicializarHumidificador(void);
void activarHumidificadorControl(void);
void desactivarHumidificadorControl(void);

#endif // HUMIDIFICADOR_H
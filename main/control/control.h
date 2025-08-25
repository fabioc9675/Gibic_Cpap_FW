#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>

//#define control1
#define control2

 /**
 * brief: Funcion para calcular el control PID
 * param: setpointPresion: presion objetivo del sistema
 * param: presion: presion actual del sistema
 * return: salida del control PID
 */
uint16_t controller(uint8_t setpointPresion, float presion, float flow);

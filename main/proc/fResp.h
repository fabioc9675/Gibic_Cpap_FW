#ifndef FRESP_H
#define FRESP_H

#include <math.h>
#include <stdbool.h>
#include "../common/common.h"
#include "../control/filter.h"

// #define smooth_win  round(0.5 * fs);     % 500 ms smoothing - 0.5 sec
// #define center_win  round(5 * fs);       % 5 seconds for slow baseline drift

void fResp_init(void);
void processed_signal(float new_point, float *smp, float *cp);
void rfrec(float data_value);
void zeros_crossings(void);
#endif // FRESP_H


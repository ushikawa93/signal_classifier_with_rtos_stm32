/*
 * signals.h
 *
 *  Created on: 19 jun 2026
 *      Author: Mati
 */

#ifndef INC_SIGNALS_H_
#define INC_SIGNALS_H_

#include "string.h"

#define PRESCALER 199
#define CLK 4000000
#define N_FREQS 4

extern RNG_HandleTypeDef hrng;
static const uint32_t signal_sen[128]      = {2048, 2148, 2248, 2348, 2447, 2545, 2642, 2737, 2831, 2923, 3012, 3100, 3185, 3267, 3346, 3422,3495, 3564, 3630, 3692, 3750, 3803, 3853, 3898, 3939, 3975, 4006, 4033, 4055, 4072, 4085, 4092,	4095, 4092, 4085, 4072, 4055, 4033, 4006, 3975,	3939, 3898, 3853, 3803, 3750, 3692, 3630, 3564,	3495, 3422, 3346, 3267, 3185, 3100, 3012, 2923,	2831, 2737, 2642, 2545, 2447, 2348, 2248, 2148,	2048, 1947, 1847, 1747, 1648, 1550, 1453, 1358,	1264, 1172, 1083,  995,  910,  828,  749,  673,	 600,  531,  465,  403,  345,  292,  242,  197,	 156,  120,   89,   62,   40,   23,   10,    3,	   1,    3,   10,   23,   40,   62,   89,  120, 156,  197,  242,  292,  345,  403,  465,  531,	 600,  673,  749,  828,  910,  995, 1083, 1172,	1264, 1358, 1453, 1550, 1648, 1747, 1847, 1947};
static const uint32_t signal_cuadrada[128]   = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095	};
static const uint32_t signal_triangular[128] = {1,33,65,97,129,161,193,225,257,289,321,353,385,417,449,481,513,545,577,609,641,673,705,737,769,801,833,865,897,929,961,993,1025,1057,1089,1121,1153,1185,1217,1249,1281,1313,1345,1377,1409,1441,1473,1505,1537,1569,1601,1633,1665,1697,1729,1761,1793,1825,1857,1889,1921,1953,1985,2017,2049,2081,2113,2145,2177,2209,2241,2273,2305,2337,2369,2401,2433,2465,2497,2529,2561,2593,2625,2657,2689,2721,2753,2785,2817,2849,2881,2913,2945,2977,3009,3041,3073,3105,3137,3169,3201,3233,3265,3297,3329,3361,3393,3425,3457,3489,3521,3553,3585,3617,3649,3681,3713,3745,3777,3809,3841,3873,3905,3937,3969,4001,4033,4065};
uint32_t signal_sen_ruido[128];
uint32_t signal_cuadrada_ruido[128];
uint32_t signal_triangular_ruido[128];

static const uint32_t *signals[3] = { signal_sen_ruido, signal_cuadrada_ruido, signal_triangular_ruido };

static const uint32_t frecuencias_dac[] = {1, 2, 8, 16};

// Periodo del timer del dac considerando 128 Muestras x ciclo
int getTIMER_DAC ( int signal_freq )
{
	return (CLK / ((PRESCALER + 1) * signal_freq * 128)) - 1;
}


int getRandom (int amplitud)
{
	if(amplitud == 0){return 0;}
	uint32_t rnd;
	HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
	return (rnd % amplitud) - amplitud/2;
}

void addNoise(uint32_t *signal, int len, int amplitud)
{
    for(int i = 0; i < len; i++)
    {
        int32_t val = signal[i] + getRandom(amplitud);

        if(val < 0)
            val = 0;

        if(val > 4095)
            val = 4095;

        signal[i] = val;
    }
}

void generateSignalsWithNoise(int amplitud)
{
	memcpy(signal_sen_ruido, signal_sen, sizeof(signal_sen));
	memcpy(signal_cuadrada_ruido, signal_cuadrada, sizeof(signal_cuadrada));
	memcpy(signal_triangular_ruido, signal_triangular, sizeof(signal_triangular));

	addNoise(signal_sen_ruido, 128, amplitud);
	addNoise(signal_cuadrada_ruido, 128, amplitud);
	addNoise(signal_triangular_ruido, 128, amplitud);
}




#endif /* INC_SIGNALS_H_ */

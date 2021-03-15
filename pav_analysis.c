#include <math.h>
#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N) {
    float t_pow = 0;
    for(int i = 0; i < N; i++){
        t_pow += x[i]*x[i];
    }
    float power = 10 * log10(t_pow/N);
    return power;   
    }

float compute_am(const float *x, unsigned int N) {
    float am = 0;

    for(int i = 0; i < N; i++){
        am += fabs(x[i]);
    }
    return am / N;
 }

float compute_zcr(const float *x, unsigned int N, float fm) {
    float t_crosses = 0;

    for(int i = 0; i < N; i++){
        if((x[i] > 0 && x[i-1] < 0 ) || (x[i] < 0 && x[i-1] > 0)){
            t_crosses++;
        }
        
    }
    float zcr = (fm/2)*(t_crosses/(N-1)); 
    return zcr;
}

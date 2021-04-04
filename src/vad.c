#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MV", "MS"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  feat.zcr = compute_zcr(x,N,16000);
  feat.p = compute_power(x,N);
  feat.am = compute_am(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate, int number_init, float n_alpha1, float n_alpha2) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->alpha1 = n_alpha1;
  vad_data->alpha2 = n_alpha2;
  vad_data->counter_N = number_init;
  vad_data->counter_init = 0;
  vad_data->k0 = 0;
  vad_data->k1 = 0;
  vad_data->k2 = 0;
  vad_data->frames_MV = 5;  // numero de frames esperando para pasar a VOZ
  vad_data->frames_MS = 10;  // numero de frames max para que vuelva a VOZ

  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data, VAD_STATE state) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  if(state!= ST_SILENCE && state!= ST_VOICE)
    state = ST_SILENCE;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  int fr_und = 0, min_back_voice = 2, min_back_voice_counter = 0; // frames indefinidos (MV/MS)
  
  switch (vad_data->state) {
  case ST_INIT:
    if(vad_data->counter_init < vad_data->counter_N){
      vad_data->counter_init++;
      vad_data->k0 += pow(10, f.p/10);
    }
    else{
      vad_data->k0 = 10*log10(vad_data->k0/vad_data->counter_N);
      vad_data->k1 = vad_data->k0 + vad_data->alpha1;
      vad_data->k2 = vad_data->k1 + vad_data->alpha2;
      vad_data->state = ST_SILENCE;
    }

    break;

  case ST_SILENCE:
    if (f.p > vad_data->k1)
      vad_data->state = ST_MV;
    break;

  case ST_VOICE:
    if (f.p < vad_data->k2)
      vad_data->state = ST_MS;
    break;
  
  case ST_MV:
    if(f.p < vad_data->k1 || fr_und >= vad_data->frames_MV){
      vad_data->state = ST_SILENCE;
      fr_und = 0;
    }
    else{
      if(f.p > vad_data->k2){
        vad_data->state = ST_VOICE;
        fr_und = 0;
      }
      else{
        fr_und++;
      }
    }      
    break;

  case ST_MS:
    if(f.p > vad_data->k2 && fr_und <= vad_data->frames_MS){
      min_back_voice_counter++; // contador para mirar si es únicamente una muestra la que sobrepasa de k2
      if(min_back_voice_counter >= min_back_voice ){
        vad_data->state = ST_VOICE;
        fr_und = 0;
      }
      else{
        fr_und++;
      }
    }
    else{
      min_back_voice_counter = 0;
      if(f.p < vad_data->k1 || fr_und >= vad_data->frames_MS){  // llevamos mucho tiempo esperando
        vad_data->state = ST_SILENCE;
        fr_und = 0;
      }
      else{ // entre k1 y k2, y llevamos poco tiempo
        fr_und++;
      }
      
    } 
    break;

  case ST_UNDEF:
    break;
  }
  if (vad_data->state == ST_SILENCE || vad_data->state == ST_VOICE)
    return vad_data->state;
  else if(vad_data->state == ST_INIT)
    return ST_SILENCE;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}

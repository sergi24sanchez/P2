#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>

#include "vad.h"
#include "vad_docopt.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[]) {
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;

  VAD_DATA *vad_data;
  VAD_STATE state, last_state;

  float *buffer, *buffer_zeros;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  unsigned int t, last_t, last_fr_und; /* in frames */

  char	*input_wav, *output_vad, *output_wav;
  unsigned int number_init, frames_mv, frames_ms;
  float alpha1, alpha2;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");  /*devuelve cadenas de texto*/

  verbose    = args.verbose ? DEBUG_VAD : 0;
  input_wav  = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;
  number_init = atoi(args.number_init);
  alpha1 = atof(args.alpha1);
  alpha2 = atof(args.alpha2);
  frames_mv = atoi(args.frames_mv);
  frames_ms = atoi(args.frames_ms);

  if (input_wav == 0 || output_vad == 0) {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0) {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1) {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0) {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav) {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0) {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  vad_data = vad_open(sf_info.samplerate, number_init, alpha1, alpha2, frames_mv, frames_ms);
  /* Allocate memory for buffers */
  frame_size   = vad_frame_size(vad_data);
  buffer       = (float *) malloc(frame_size * sizeof(float));
  buffer_zeros = (float *) malloc(frame_size * sizeof(float));
  for (i=0; i< frame_size; ++i) buffer_zeros[i] = 0.0F;

  frame_duration = (float) frame_size/ (float) sf_info.samplerate;
  last_state = ST_UNDEF;
 
  for (t = last_t = 0; ; t++) { /* For each frame ... */
    /* End loop when file has finished (or there is an error) */
    if  ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size) break;

    last_fr_und = vad_data->fr_und;
    state = vad(vad_data, buffer); //actualmente solo tiene los valores {ST_UNDEF, ST_SILENCE, ST_VOICE}
    printf("last_feature %3d = %.4f\n", t, vad_data->last_feature); // para ver la potencia
    if (verbose & DEBUG_VAD) vad_show_state(vad_data, stdout);

    if (sndfile_out != 0 && state != ST_SILENCE) {
      sf_write_float(sndfile_out, buffer, frame_size);
    } else {
      sf_write_float(sndfile_out, buffer_zeros, frame_size);
    }

    /* TODO: print only SILENCE and VOICE labels */
    
    if (state != last_state && state!= ST_UNDEF) {  // si hay cambio de estado

      if (t != last_t)
        fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, (t - last_fr_und) * frame_duration, state2str(last_state));

      last_t = (t - last_fr_und);  //  cuando escribimos, hay que actualizar last_t 
      last_state = state;
    }    
  }
  printf("k0 = %.4f\nk1 = %.4f\nk2 = %.4f\n",vad_data->k0, vad_data->k1, vad_data->k2);

  state = vad_close(vad_data,state);
  if(state != ST_SILENCE && state != ST_VOICE){
    sf_write_float(sndfile_out, buffer_zeros, frame_size);
    state = ST_SILENCE;
  }
    
  /* TODO: what do you want to print, for last frames? */
  if (t != last_t)
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(state));

  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out) sf_close(sndfile_out);
  return 0;
}

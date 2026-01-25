/* -------------------------------------------------------------------------- */
#include "clap_utils.h"
#include <math.h>
/* -------------------------------------------------------------------------- */
double db_gain_to_mult(double db_gain)
{
	return pow(10, db_gain * 0.05);
}
/* -------------------------------------------------------------------------- */
// void process_buffer_64(double **in, double **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i)
// {
// 	double in_l  = in[0][i];
// 	double in_r  = in[1][i];
//
// 	double out_l, out_r;
// 	out_l = in_l;
// 	out_r = in_r;
//
// 	if (plug->gain_db >= 0) {
// 		out_l *= 4.0;
// 		out_r *= 4.0;
//
// 	} else {
// 		out_l *= 0.25;
// 		out_r *= 0.25;
// 	}
// 	// out_l = in_l * plug->gain_multiplier;
// 	// out_r = in_r * plug->gain_multiplier;
//
// 	// store output samples
// 	out[0][i] = out_l;
// 	out[1][i] = out_r;
//
// }
/* -------------------------------------------------------------------------- */
// void process_buffer_32(float **in, float **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i)
// {
// 	double in_l  = in[0][i];
// 	double in_r  = in[1][i];
//
// 	double out_l, out_r;
// 	out_l = in_l;
// 	out_r = in_r;
//
// 	if (plug->gain_db >= 0) {
// 		out_l *= 4.0;
// 		out_r *= 4.0;
//
// 	} else {
// 		out_l *= 0.25;
// 		out_r *= 0.25;
// 	}
// 	// out_l = in_l * plug->gain_multiplier;
// 	// out_r = in_r * plug->gain_multiplier;
//
// 	// store output samples
// 	out[0][i] = out_l;
// 	out[1][i] = out_r;
//
// }
/* -------------------------------------------------------------------------- */


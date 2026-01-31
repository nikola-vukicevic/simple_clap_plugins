/* -------------------------------------------------------------------------- */
#ifndef UTIL_LOADED
#define UTIL_LOADED
/* -------------------------------------------------------------------------- */
#include "../include/clap/clap.h"
/* -------------------------------------------------------------------------- */
typedef struct
{
	clap_plugin_t plugin;
	const clap_host_t *host;
	const clap_host_latency_t *hostLatency;
	const clap_host_log_t *hostLog;
	const clap_host_thread_check_t *hostThreadCheck;
	const clap_host_params_t *hostParams;

	double gain_db, gain_multiplier;
} clap_gain_staging_plug;
/* -------------------------------------------------------------------------- */
double db_gain_to_mult(double db_gain);
void process_buffer_64(double **in, double **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i);
void process_buffer_32(float **in, float **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i);
/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */


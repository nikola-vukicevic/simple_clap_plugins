/* -------------------------------------------------------------------------- */
// Copyright (c) Nikola Vukićević 2026.
/* -------------------------------------------------------------------------- */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <clap/clap.h>
// #include <math.h>
#include <assert.h>
#include "clap_utils.h"
/* -------------------------------------------------------------------------- */
#define MIN_DB -36
#define MAX_DB 36
/* -------------------------------------------------------------------------- */
static const clap_plugin_descriptor_t s_gain_staging_desc = {
	.clap_version = CLAP_VERSION_INIT,
	.id           = "org.nikola-vukicevic.clap-gain-staging",
	.name         = "Gain staging",
	.vendor       = "Nikola Vukicevic",
	.url          = "https://surge-synth-team.org/",
	.manual_url   = "",
	.support_url  = "",
	.version      = "1.0.0",
	.description  = "A simple utility gain staging plugin",
	.features     = (const char *[]){
		CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
		CLAP_PLUGIN_FEATURE_STEREO,
		NULL
	},
};
/* -------------------------------------------------------------------------- */
enum ParamIds
{
	PID_GAIN = 1001,
};
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
static void gain_staging_process_event(clap_gain_staging_plug *plug, const clap_event_header_t *hdr);
/* -------------------------------------------------------------------------- */
/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////
/* -------------------------------------------------------------------------- */
static uint32_t gain_staging_audio_ports_count(const clap_plugin_t *plugin, bool is_input) {
	return 1;
}
/* -------------------------------------------------------------------------- */
static bool gain_staging_audio_ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input,
                                    clap_audio_port_info_t *info)
{
	if (index > 0) return false;
	info->id = 0;
	if (is_input)
		snprintf(info->name, sizeof(info->name), "%s", "Stereo In");
	else
		snprintf(info->name, sizeof(info->name), "%s", "Output");
	info->channel_count = 2;
	info->flags         = CLAP_AUDIO_PORT_IS_MAIN |
		                  CLAP_AUDIO_PORT_SUPPORTS_64BITS |
		                  CLAP_AUDIO_PORT_PREFERS_64BITS;
	info->port_type     = CLAP_PORT_STEREO;
	info->in_place_pair = CLAP_INVALID_ID;
	return true;
}

static const clap_plugin_audio_ports_t s_gain_staging_audio_ports = {
    .count = gain_staging_audio_ports_count,
    .get   = gain_staging_audio_ports_get,
};

//////////////////
// clap_latency //
//////////////////

uint32_t gain_staging_latency_get(const clap_plugin_t *plugin) {
	return 0;
}

static const clap_plugin_latency_t s_gain_staging_latency = {
    .get = gain_staging_latency_get,
};

//////////////////
// clap_porams //
//////////////////
/* -------------------------------------------------------------------------- */
uint32_t gain_staging_param_count(const clap_plugin_t *plugin) {
	return 1;
}
/* -------------------------------------------------------------------------- */
bool gain_staging_param_get_info(const clap_plugin_t *plugin, uint32_t param_index, clap_param_info_t *param_info)
{
	switch (param_index) {
		case 0: // gain
			param_info->id            = PID_GAIN;
			strncpy(param_info->name, "Gain", CLAP_NAME_SIZE);
			param_info->module[0]     = 0;
			param_info->default_value = 0.;
			param_info->min_value     = MIN_DB;
			param_info->max_value     = MAX_DB;
			param_info->flags         = CLAP_PARAM_IS_AUTOMATABLE;
			param_info->cookie        = NULL;
		break;
		default: return false;
	}

	return true;
}
/* -------------------------------------------------------------------------- */
bool gain_staging_param_get_value(const clap_plugin_t *plugin, clap_id param_id, double *value)
{
	if (param_id != PID_GAIN) return false;

	clap_gain_staging_plug *plug = plugin->plugin_data;
	*value = plug->gain_db;
	return true;
}
/* -------------------------------------------------------------------------- */
bool gain_staging_param_value_to_text(const clap_plugin_t *plugin, clap_id param_id, double value, char *display,
                                 uint32_t size)
{
	if (param_id != PID_GAIN) return false;

	clap_gain_staging_plug *plug = plugin->plugin_data;
	snprintf(display, size, "%f", value);
	return true;
}
/* -------------------------------------------------------------------------- */
bool gain_staging_text_to_value(const clap_plugin_t *plugin, clap_id param_id, const char *display, double *value)
{
	// TODO
	return false;
}
/* -------------------------------------------------------------------------- */
void gain_staging_flush(const clap_plugin_t *plugin, const clap_input_events_t *in, const clap_output_events_t *out)
{
	clap_gain_staging_plug *plug = plugin->plugin_data;
	int s                        = in->size(in);
	int q;

	for (q = 0; q < s; ++q) {
		const clap_event_header_t *hdr = in->get(in, q);
		gain_staging_process_event(plug, hdr);
	}
}
/* -------------------------------------------------------------------------- */
static const clap_plugin_params_t s_gain_staging_params = {
	.count         = gain_staging_param_count,
	.get_info      = gain_staging_param_get_info,
	.get_value     = gain_staging_param_get_value,
	.value_to_text = gain_staging_param_value_to_text,
	.text_to_value = gain_staging_text_to_value,
	.flush         = gain_staging_flush
};
/* -------------------------------------------------------------------------- */
static const clap_plugin_state_t s_gain_staging_state = {
	// .save = gain_staging_state_save,
	// .load = gain_staging_state_load
};
/* -------------------------------------------------------------------------- */
/////////////////
// clap_plugin //
/////////////////
/* -------------------------------------------------------------------------- */
static bool gain_staging_init(const struct clap_plugin *plugin)
{
	clap_gain_staging_plug *plug = plugin->plugin_data;

	// Fetch host's extensions here
	plug->hostLog         = plug->host->get_extension(plug->host, CLAP_EXT_LOG);
	plug->hostThreadCheck = plug->host->get_extension(plug->host, CLAP_EXT_THREAD_CHECK);
	plug->hostLatency     = plug->host->get_extension(plug->host, CLAP_EXT_LATENCY);

	plug->gain_db         = 0.f;
	plug->gain_multiplier = 1.0f;
	return true;
}
/* -------------------------------------------------------------------------- */
static void gain_staging_destroy(const struct clap_plugin *plugin)
{
	clap_gain_staging_plug *plug = plugin->plugin_data;
	free(plug);
}
/* -------------------------------------------------------------------------- */
static bool gain_staging_activate(const struct clap_plugin *plugin, double sample_rate, uint32_t min_frames_count,
                             uint32_t max_frames_count)
{
	return true;
}
/* -------------------------------------------------------------------------- */
static void gain_staging_deactivate(const struct clap_plugin *plugin) {}

static bool gain_staging_start_processing(const struct clap_plugin *plugin) { return true; }

static void gain_staging_stop_processing(const struct clap_plugin *plugin) {}

static void gain_staging_reset(const struct clap_plugin *plugin) {}
/* -------------------------------------------------------------------------- */
static void gain_staging_process_event(clap_gain_staging_plug *plug, const clap_event_header_t *hdr)
{
	if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
		switch (hdr->type) {
			case CLAP_EVENT_PARAM_VALUE: {
				const clap_event_param_value_t *ev = (const clap_event_param_value_t *)hdr;
				// TODO: handle parameter change
				switch (ev->param_id) {
					case PID_GAIN:
						plug->gain_db         = ev->value;
						plug->gain_multiplier = db_gain_to_mult(plug->gain_db);
					break;
				}
				break;
			}
		}
	}
}
/* -------------------------------------------------------------------------- */
void process_buffer_32(float **in, float **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i)
{
	double in_l  = in[0][i];
	double in_r  = in[1][i];

	double out_l, out_r;

	out_l = in_l * plug->gain_multiplier;
	out_r = in_r * plug->gain_multiplier;

	out[0][i] = out_l;
	out[1][i] = out_r;

}
/* -------------------------------------------------------------------------- */
void process_buffer_64(double **in, double **out, clap_gain_staging_plug *plug, const clap_process_t *process, int i)
{
	double in_l  = in[0][i];
	double in_r  = in[1][i];

	double out_l, out_r;

	out_l = in_l * plug->gain_multiplier;
	out_r = in_r * plug->gain_multiplier;

	out[0][i] = out_l;
	out[1][i] = out_r;

}
/* -------------------------------------------------------------------------- */
static clap_process_status gain_staging_process(const struct clap_plugin *plugin, const clap_process_t *process)
{
	clap_gain_staging_plug *plug = plugin->plugin_data;
	const uint32_t nframes       = process->frames_count;
	const uint32_t nev           = process->in_events->size(process->in_events);
	uint32_t ev_index            = 0;
	uint32_t next_ev_frame       = (nev > 0) ? 0 : nframes;

	for (uint32_t i = 0; i < nframes;) {
		/* handle every event that happens at the frame "i" */
		while (ev_index < nev && next_ev_frame == i) {
			const clap_event_header_t *hdr = process->in_events->get(process->in_events, ev_index);
			if (hdr->time != i) {
				next_ev_frame = hdr->time;
				break;
			}

			gain_staging_process_event(plug, hdr);
			++ev_index;

			if (ev_index == nev) {
				// we reached the end of the event list
				next_ev_frame = nframes;
				break;
			}
		}

		/* process every samples until the next event */
		for (; i < next_ev_frame; ++i) {
			if (process->audio_inputs[0].data64) {
				process_buffer_64(process->audio_inputs[0].data64, process->audio_outputs[0].data64, plug, process, i);
				continue;
			}
			if (process->audio_inputs[0].data32) {
				process_buffer_32(process->audio_inputs[0].data32, process->audio_outputs[0].data32, plug, process, i);
				continue;
			}
		}
	}

	return CLAP_PROCESS_CONTINUE;
}
/* -------------------------------------------------------------------------- */
static const void *gain_staging_get_extension(const struct clap_plugin *plugin, const char *id)
{
	if (!strcmp(id, CLAP_EXT_LATENCY))
		return &s_gain_staging_latency;

	if (!strcmp(id, CLAP_EXT_AUDIO_PORTS))
		return &s_gain_staging_audio_ports;

	if (!strcmp(id, CLAP_EXT_PARAMS))
		return &s_gain_staging_params;

	if (!strcmp(id, CLAP_EXT_STATE))
		return &s_gain_staging_state;

	return NULL;
}
/* -------------------------------------------------------------------------- */
static void gain_staging_on_main_thread(const struct clap_plugin *plugin) {}
/* -------------------------------------------------------------------------- */
clap_plugin_t *gain_staging_create(const clap_host_t *host)
{
	clap_gain_staging_plug *p  = calloc(1, sizeof(*p));
	p->host                    = host;
	p->plugin.desc             = &s_gain_staging_desc;
	p->plugin.plugin_data      = p;
	p->plugin.init             = gain_staging_init;
	p->plugin.destroy          = gain_staging_destroy;
	p->plugin.activate         = gain_staging_activate;
	p->plugin.deactivate       = gain_staging_deactivate;
	p->plugin.start_processing = gain_staging_start_processing;
	p->plugin.stop_processing  = gain_staging_stop_processing;
	p->plugin.reset            = gain_staging_reset;
	p->plugin.process          = gain_staging_process;
	p->plugin.get_extension    = gain_staging_get_extension;
	p->plugin.on_main_thread   = gain_staging_on_main_thread;

	// Don't call into the host here

	return &p->plugin;
}
/* -------------------------------------------------------------------------- */
/////////////////////////
// clap_plugin_factory //
/////////////////////////
/* -------------------------------------------------------------------------- */
static struct
{
    const clap_plugin_descriptor_t *desc;
    clap_plugin_t *(*create)(const clap_host_t *host);
} s_plugins[] = {
    {
        .desc = &s_gain_staging_desc,
        .create = gain_staging_create,
    },
};
/* -------------------------------------------------------------------------- */
static uint32_t plugin_factory_get_plugin_count(const struct clap_plugin_factory *factory)
{
    return sizeof(s_plugins) / sizeof(s_plugins[0]);
}
/* -------------------------------------------------------------------------- */
static const clap_plugin_descriptor_t *
plugin_factory_get_plugin_descriptor(const struct clap_plugin_factory *factory, uint32_t index)
{
    return s_plugins[index].desc;
}
/* -------------------------------------------------------------------------- */
static const clap_plugin_t *plugin_factory_create_plugin(const struct clap_plugin_factory *factory,
                                                         const clap_host_t *host,
                                                         const char *plugin_id)
{
	if (!clap_version_is_compatible(host->clap_version)) {
		return NULL;
	}

	const int N = sizeof(s_plugins) / sizeof(s_plugins[0]);
	for (int i = 0; i < N; ++i)
		if (!strcmp(plugin_id, s_plugins[i].desc->id)) return s_plugins[i].create(host);

	return NULL;
}
/* -------------------------------------------------------------------------- */
static const clap_plugin_factory_t s_plugin_factory = {
    .get_plugin_count = plugin_factory_get_plugin_count,
    .get_plugin_descriptor = plugin_factory_get_plugin_descriptor,
    .create_plugin = plugin_factory_create_plugin,
};
/* -------------------------------------------------------------------------- */
////////////////
// clap_entry //
////////////////
/* -------------------------------------------------------------------------- */
static bool entry_init(const char *plugin_path)
{
    // called only once, and very first
    return true;
}
/* -------------------------------------------------------------------------- */
static void entry_deinit(void)
{
    // called before unloading the DSO
}
/* -------------------------------------------------------------------------- */
static const void *entry_get_factory(const char *factory_id)
{
	if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) return &s_plugin_factory;
	return NULL;
}
/* -------------------------------------------------------------------------- */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
	.clap_version = CLAP_VERSION_INIT,
	.init         = entry_init,
	.deinit       = entry_deinit,
	.get_factory  = entry_get_factory,
};
/* -------------------------------------------------------------------------- */


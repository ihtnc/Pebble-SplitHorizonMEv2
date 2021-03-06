#include "pebble_os.h"
#include "pebble_app.h"
#include "monitor.h"
#include "app_options.h"

static bool is_connected;
static bool needs_vibe;
static uint8_t vibe_freq_count = 13;
static uint8_t vibe_freq[] = {1, 1, 1, 1, 5, 5, 5, 5, 5, 30, 30, 30, 60};
static uint8_t vibe_index = 0;
static int initial_disconnect;
const VibePattern vibes_disconnect_pattern = 
{
	.durations = (uint32_t []) {200, 100, 200, 100, 400, 200},
	.num_segments = 6
};

const VibePattern vibes_connect_pattern = 
{
	.durations = (uint32_t []) {100, 100, 100, 100},
	.num_segments = 4
};

void handle_result(int result)
{
	switch (result) 
	{
		case HTTP_SEND_TIMEOUT:
		case HTTP_NOT_CONNECTED:
		case HTTP_BRIDGE_NOT_RUNNING:
		case HTTP_NOT_ENOUGH_STORAGE:
		{		
			//we just got disconnected
			if(is_connected == true)
			{
				static PblTm time;
				get_time(&time);
				
				vibe_index = 0;
				is_connected = false;
				initial_disconnect = time.tm_min;
			}
		
			if(needs_vibe == true) vibes_enqueue_custom_pattern(vibes_disconnect_pattern);
			break;
		}
		case HTTP_OK:
		{
			if(is_connected == false) //connection was restored
			{
				vibe_index = 0;
				is_connected = true;
				vibes_enqueue_custom_pattern(vibes_connect_pattern);
			}
			break;
		}
		//all the other error codes should not happen or are not considered error in connectivity
	}
}

void failed(int32_t cookie, int http_status, void* context)
{
	handle_result(http_status >= 1000 ? http_status - 1000 : http_status);
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context)
{	
	handle_result(http_status);
}

void reconnect(void* context)
{
	handle_result(HTTP_OK);
}

void ping()
{
	#ifdef PHONE_HAS_HTTPPEBBLE
		needs_vibe = true;
	
		if(is_connected == false)
		{
			static PblTm time;
			get_time(&time);
			
			//if still disconnected, the vibrate frequency gets reduced over time, until it no longer vibrates 
			if(vibe_index > vibe_freq_count) needs_vibe = false;
			else if(time.tm_min % vibe_freq[vibe_index] != initial_disconnect % vibe_freq[vibe_index]) needs_vibe = false;
			else vibe_index++;
		}
		
		http_time_request();
	#endif
}

void monitor_init(AppContextRef ctx) 
{
	http_set_app_id(APP_ID);
	
	vibe_index = 0;
	is_connected = true;
	
	http_register_callbacks((HTTPCallbacks)
							{
								.failure=failed,
								.success=success,
								.reconnect=reconnect
							}, 
							(void*)ctx);
}
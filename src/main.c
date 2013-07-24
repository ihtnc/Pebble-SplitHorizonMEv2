/**
* Split Horizon: Minutes Edition Pebble Watchface
* Author: Chris Lewis
* Date: 3rd June 2013
* Tweaked by: ihopethisnamecounts
*/

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <math.h>
#include "monitor.h"

/* 
Because of the way that httpebble works, a different UUID is needed for Android and iOS compatibility. 
If you are building this to use with Android, then leave the #define ANDROID line alone, and if 
you're building for iOS then remove or comment out that line.
*/

//#define ANDROID
#ifdef ANDROID
	#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x10, 0x34, 0xBF, 0xBE, 0x12, 0x98 }
#else
	#define MY_UUID HTTP_UUID
#endif
	
//#define DEBUG
	
//this flag changes the behavior of the horizon into 12 hours to rise and 12 hours to fall
//instead of 24 hours to rise
//#define RISEFALL 
	
#define container_of(ptr, type, member) \
	({ \
		char *__mptr = (char *)(uintptr_t) (ptr); \
		(type *)(__mptr - offsetof(type,member) ); \
	 })
	
//Prototypes
void animateLayer(PropertyAnimation *animation, Layer *input, GRect startLocation, GRect finishLocation, int duration);

Window window;
TextLayer lowerLayer, timeLayer, amPmLayer, dayLayer, dateLayer, monthLayer;
InverterLayer topShade, bottomShade;
PropertyAnimation topAnimation, bottomAnimation;

PblTm current_time;
int horizonOffset = 0;
int horizonOffsetMin = 0;
int horizonOffsetMax = 88;
int horizonOffsetTotal = 88;

//minus 1 since that 1 is the first position already
double horizonOffsetPerHour = 88 / 23.0;  //23 = 24 - 1

const int splash_none = 0;
const int splash_start = 1;
const int splash_change = 2;
const int splash_end = 3;
static int splashStatus;

#ifndef DEBUG
	int enableTick = true;
#else
	int enableTick = false;
#endif

PBL_APP_INFO(MY_UUID, 
             #ifndef DEBUG
                 "Split Horizon ME v2", 
             #else
                 "Debug: Split Horizon ME v2", 
             #endif
             "ihopethisnamecounts", 
             1, 3, 
             RESOURCE_ID_IMAGE_MENU_ICON, 
             #ifndef DEBUG
                 APP_INFO_WATCH_FACE
             #else
                 APP_INFO_STANDARD_APP
             #endif
            );

/**
* Function to set the time and date features on the TextLayers
*/
void setTime(PblTm *t) 
{
	int day = t->tm_mday;
	int seconds =	t->tm_sec;
	int hours = t->tm_hour;

	//Time string
	static char hourText[] = "00:00";
	if(clock_is_24h_style())
		string_format_time(hourText, sizeof(hourText), "%H:%M", t);
	else
		string_format_time(hourText, sizeof(hourText), "%I:%M", t);
	text_layer_set_text(&timeLayer, hourText);

	//AM/PM
	if(!clock_is_24h_style())
	{
		layer_set_frame(&amPmLayer.layer, GRect(113, -4 + horizonOffset, 40, 40));
		if(hours < 12)
			text_layer_set_text(&amPmLayer, "AM");
		else
			text_layer_set_text(&amPmLayer, "PM");
	} 
	else
		layer_set_frame(&amPmLayer.layer, GRect(0, 0, 0, 0));

	//Day string
	static char dayText[] = "Wed";
	string_format_time(dayText, sizeof(dayText), "%a", t);
	text_layer_set_text(&dayLayer, dayText);

	//Date string
	static char dateText[] = "xxxx";
	string_format_time(dateText, sizeof(dateText), "%eth", t);
	if((day == 1) || (day == 21) || (day == 31)) 
	{	//1st, 21st, 31st
		dateText[2] = 's';
		dateText[3] = 't';
	} 
	else if ((day == 2) || (day == 22))
	{	//2nd, 22nd
		dateText[2] = 'n';
		dateText[3] = 'd';
	}
	else if((day == 3) || (day == 23))
	{	//3rd, 23rd
		dateText[2] = 'r';
		dateText[3] = 'd';
	}
	text_layer_set_text(&dateLayer, dateText);

	//Month string
	static char monthText[] = "September";
	string_format_time(monthText, sizeof(monthText), "%B", t);
	text_layer_set_text(&monthLayer, monthText);
}

void realign_horizon()
{
	if(horizonOffset < horizonOffsetMin) horizonOffset = horizonOffsetMin;
	else if(horizonOffset > horizonOffsetMax) horizonOffset = horizonOffsetMax;

	layer_set_frame(&lowerLayer.layer, GRect(0, 45 + horizonOffset, 144, 123 - horizonOffset));	
	layer_set_frame(&timeLayer.layer, GRect(1, 2 + horizonOffset, 150, 50));
	layer_set_frame(&dateLayer.layer, GRect(70, 36 + horizonOffset, 150, 50));
	
	layer_set_frame(&dayLayer.layer, GRect(1, 36 + horizonOffset, 150, 50));
	layer_set_frame(&monthLayer.layer, GRect(1, 56 + horizonOffset, 150, 50));
	
	if(!clock_is_24h_style()) layer_set_frame(&amPmLayer.layer, GRect(113, -4 + horizonOffset, 40, 40));

	if(splashStatus == splash_none) layer_mark_dirty(&window.layer);
}

int get_offset(PblTm *time)
{
	int offset;
	#ifndef RISEFALL
		offset = round(horizonOffsetTotal - (time->tm_hour * horizonOffsetPerHour));
	#else
		int hr = time->tm_hour % 12;
		int current = ((hr * 60) + time->tm_min) / 30;
		if(time->tm_hour < 12)
		{
			offset = round(horizonOffsetTotal - (current * horizonOffsetPerHour));
		}
		else
		{
			offset = round(current * horizonOffsetPerHour);
		}
	#endif
		
	return offset;
}

/**
* Handle function called every second
*/
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
	(void)ctx;

	if(enableTick == false || splashStatus != splash_none) return;
	
	//Get the time
	get_time(&current_time);
	int seconds =	current_time.tm_sec;

	if(seconds == 59)
	{
		//Animate shades in
		animateLayer(&topAnimation, &topShade.layer, GRect(0,0,144,0), GRect(0,0,144,45 + horizonOffset), 400);
		animateLayer(&bottomAnimation, &bottomShade.layer, GRect(0,168,144,0), GRect(0,45 + horizonOffset,144,123 - horizonOffset), 400);
	}

	if(seconds == 0) 
	{	
		#ifndef RISEFALL
			if(t->tick_time->tm_min == 0)
			{	
				horizonOffset = get_offset(t->tick_time);
				realign_horizon();
			}
		#else
			if(t->tick_time->tm_min % 30 == 0)
			{	 
				horizonOffset = get_offset(t->tick_time);
				realign_horizon();
			}
		#endif
			
		//Change time
		setTime(t->tick_time);
		ping();
	}

	if(seconds == 1) 
	{	
		//Animate shades out
		animateLayer(&topAnimation, &topShade.layer, GRect(0,0,144,45 + horizonOffset), GRect(0,0,144,0), 400);
		animateLayer(&bottomAnimation, &bottomShade.layer, GRect(0,45 + horizonOffset,144,123 - horizonOffset), GRect(0,168,144,0), 400);
	}
}

void showSplash()
{
	//Time layer
	static char hourText[] = "ihtnc";
	text_layer_set_text(&timeLayer, hourText);
	
	//AM/PM layer
	text_layer_set_text(&amPmLayer, "tweaked by");
	
	//Day layer
	static char dayText[] = "(c) 2013 ME v2";
	text_layer_set_font(&dayLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_14)));
	text_layer_set_text(&dayLayer, dayText);

	//Date layer
	static char dateText[] = "";
	text_layer_set_text(&dateLayer, dateText);

	//Month layer
	static char monthText[] = "SplitHorizon";
	text_layer_set_font(&monthLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_14)));
	text_layer_set_text(&monthLayer, monthText);
	
	realign_horizon();
	
	layer_set_frame(&dayLayer.layer, GRect(1, 41 + horizonOffset, 150, 50));
	layer_set_frame(&monthLayer.layer, GRect(1, 56 + horizonOffset, 150, 50));
	layer_set_frame(&amPmLayer.layer, GRect(1, -4 + horizonOffset, 144, 40));
	
	layer_mark_dirty(&window.layer);
}

#ifdef DEBUG
	void handle_up_single_click(ClickRecognizerRef recognizer, Window *window) 
	{	
		#ifndef RISEFALL
			current_time.tm_hour++;
			if(current_time.tm_hour >= 24) current_time.tm_hour = 0;
		#else
			current_time.tm_min = current_time.tm_min + 30;
			if(current_time.tm_min > 60) 
			{
				current_time.tm_min = current_time.tm_min % 60;
				current_time.tm_hour++;
			}
			if(current_time.tm_hour >= 24) current_time.tm_hour = 0;
		#endif
		
		horizonOffset = get_offset(&current_time);
		setTime(&current_time);
		realign_horizon();
	}

	void handle_select_single_click(ClickRecognizerRef recognizer, Window *window) 
	{
		enableTick = !enableTick;
		get_time(&current_time);
		horizonOffset = get_offset(&current_time);
		setTime(&current_time);
		ping();
	}

	void handle_down_single_click(ClickRecognizerRef recognizer, Window *window) 
	{	
		#ifndef RISEFALL
			current_time.tm_hour--;
			if(current_time.tm_hour < 0) current_time.tm_hour = 23;
		#else
			current_time.tm_min = current_time.tm_min - 30;
			if(current_time.tm_min < 0) 
			{
				current_time.tm_min = 60 - ((current_time.tm_min * -1) % 60);
				current_time.tm_hour--;
			}
			if(current_time.tm_hour < 0) current_time.tm_hour = 23;
		#endif
		
		horizonOffset = get_offset(&current_time);
		setTime(&current_time);
		realign_horizon();
	}

	void config_provider(ClickConfig **config, Window *window) 
	{
		config[BUTTON_ID_UP]->click.handler = (ClickHandler) handle_up_single_click;
		config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

		config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) handle_down_single_click;
		config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;	

		config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) handle_select_single_click;
	}
#endif

static void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie)
{
	if(splashStatus == splash_start) 
	{
		splashStatus = splash_change;
		
		//Animate shades in
		animateLayer(&topAnimation, &topShade.layer, GRect(0,0,144,0), GRect(0,0,144,45 + horizonOffset), 400);
		animateLayer(&bottomAnimation, &bottomShade.layer, GRect(0,168,144,0), GRect(0,45 + horizonOffset,144,123 - horizonOffset), 400);
		
		app_timer_send_event(ctx, 1000, cookie);
	}
	else if(splashStatus == splash_change)
	{
		splashStatus = splash_end;
		
		text_layer_set_font(&dayLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_25)));
		text_layer_set_font(&monthLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_20)));
		
		get_time(&current_time); 
		setTime(&current_time);
		realign_horizon();
		
		layer_mark_dirty(&window.layer);
		
		app_timer_send_event(ctx, 1000, cookie);
	}
	else if(splashStatus == splash_end)
	{
		splashStatus = splash_none;
		
		animateLayer(&topAnimation, &topShade.layer, GRect(0,0,144,45 + horizonOffset), GRect(0,0,144,0), 400);
		animateLayer(&bottomAnimation, &bottomShade.layer, GRect(0,45 + horizonOffset,144,123 - horizonOffset), GRect(0,168,144,0), 400);
	}
}

/**
* Resource initialisation handle function
*/
void handle_init(AppContextRef ctx)
{
	//Init window
	window_init(&window, "Main");

	#ifdef DEBUG
		window_set_fullscreen(&window, true);
		window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);
	#endif

	window_stack_push(&window, true /* Animated */);
	window_set_background_color(&window, GColorWhite);

	//Init resources
	resource_init_current_app(&APP_RESOURCES);

	//Init bottom frame
	text_layer_init(&lowerLayer, GRect(0, 84 + horizonOffset, 144, 84 + horizonOffset));
	text_layer_set_background_color(&lowerLayer, GColorBlack);
	layer_add_child(&window.layer, &lowerLayer.layer);

	//Init TextLayers
	text_layer_init(&timeLayer, GRect(1, 41 + horizonOffset, 150, 50));
	text_layer_set_background_color(&timeLayer, GColorClear);
	text_layer_set_text_color(&timeLayer, GColorBlack);
	text_layer_set_font(&timeLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_43)));
	text_layer_set_text_alignment(&timeLayer, GTextAlignmentLeft);
	layer_add_child(&window.layer, &timeLayer.layer);

	text_layer_init(&amPmLayer, GRect(0, 0, 0, 0));
	text_layer_set_background_color(&amPmLayer, GColorClear);
	text_layer_set_text_color(&amPmLayer, GColorBlack);
	text_layer_set_font(&amPmLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_20)));
	text_layer_set_text_alignment(&amPmLayer, GTextAlignmentLeft);
	layer_add_child(&window.layer, &amPmLayer.layer);

	text_layer_init(&dayLayer, GRect(1, 75 + horizonOffset, 150, 50));
	text_layer_set_background_color(&dayLayer, GColorClear);
	text_layer_set_text_color(&dayLayer, GColorWhite);
	text_layer_set_font(&dayLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_25)));
	text_layer_set_text_alignment(&dayLayer, GTextAlignmentLeft);
	layer_add_child(&window.layer, &dayLayer.layer);

	text_layer_init(&dateLayer, GRect(70, 75 + horizonOffset, 150, 50));
	text_layer_set_background_color(&dateLayer, GColorClear);
	text_layer_set_text_color(&dateLayer, GColorWhite);
	text_layer_set_font(&dateLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_25)));
	text_layer_set_text_alignment(&dateLayer, GTextAlignmentLeft);
	layer_add_child(&window.layer, &dateLayer.layer);

	text_layer_init(&monthLayer, GRect(1, 95 + horizonOffset, 150, 50));
	text_layer_set_background_color(&monthLayer, GColorClear);
	text_layer_set_text_color(&monthLayer, GColorWhite);
	text_layer_set_font(&monthLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_20)));
	text_layer_set_text_alignment(&monthLayer, GTextAlignmentLeft);
	layer_add_child(&window.layer, &monthLayer.layer);

	//Init InverterLayers
	inverter_layer_init(&topShade, GRect(0,0,144,0));
	inverter_layer_init(&bottomShade, GRect(0,0,144,0));
	layer_add_child(&window.layer, &topShade.layer);
	layer_add_child(&window.layer, &bottomShade.layer);

	//Set initial display
	get_time(&current_time); 
	horizonOffset = get_offset(&current_time);
	showSplash();
	
	splashStatus = splash_start;
	app_timer_send_event(ctx, 2000, 0);
	
	monitor_init(ctx);
}

/**
* Main Pebble loop
*/
void pbl_main(void *params) 
{
	PebbleAppHandlers handlers = 
	{
		.init_handler = &handle_init,

		.tick_info = 
		{
			.tick_handler = &handle_second_tick,
			.tick_units = SECOND_UNIT
		},
			
		.messaging_info = 
		{
			.buffer_sizes = 
			{
				.inbound = 124,
				.outbound = 256
			}
		},
			
		.timer_handler = &handle_timer
	};
	app_event_loop(params, &handlers);
}

/**
* Function to linearly animate any layer between two GRects
*/
void animateLayer(PropertyAnimation *animation, Layer *input, GRect startLocation, GRect finishLocation, int duration)
{
	property_animation_init_layer_frame(animation, input, &startLocation, &finishLocation);
	animation_set_duration(&animation->animation, duration);
	animation_set_curve(&animation->animation, AnimationCurveEaseInOut);	
	animation_schedule(&animation->animation);
}
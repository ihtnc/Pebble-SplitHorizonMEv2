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

//#define DEBUG

#define MY_UUID { 0x05, 0x68, 0x40, 0xEE, 0x3B, 0x4B, 0x4C, 0x6E, 0x82, 0x24, 0xF2, 0x5E, 0x6E, 0xBE, 0x32, 0xEA }

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
double horizonOffsetPerHour = 88 / 23.0; 

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
             1, 0, 
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
	layer_set_frame(&dayLayer.layer, GRect(1, 36 + horizonOffset, 150, 50));
	layer_set_frame(&dateLayer.layer, GRect(70, 36 + horizonOffset, 150, 50));
	layer_set_frame(&monthLayer.layer, GRect(1, 56 + horizonOffset, 150, 50));
	
	//AM/PM
	if(!clock_is_24h_style()) layer_set_frame(&amPmLayer.layer, GRect(113, -4 + horizonOffset, 40, 40));
	
	layer_mark_dirty(&window.layer);
}

/**
* Handle function called every second
*/
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
	(void)ctx;
	
	if(enableTick == false) return;
	
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
		if(t->tick_time->tm_min == 0)
		{
			horizonOffset = round(horizonOffsetTotal - (t->tick_time->tm_hour * horizonOffsetPerHour)); 
			realign_horizon();
		}
		
		//Change time
		setTime(t->tick_time);
	}
	
	if(seconds == 1) 
	{	
		//Animate shades out
		animateLayer(&topAnimation, &topShade.layer, GRect(0,0,144,45 + horizonOffset), GRect(0,0,144,0), 400);
		animateLayer(&bottomAnimation, &bottomShade.layer, GRect(0,45 + horizonOffset,144,123 - horizonOffset), GRect(0,168,144,0), 400);
	}
}

#ifdef DEBUG
	void handle_up_single_click(ClickRecognizerRef recognizer, Window *window) 
	{	
		current_time.tm_hour++;
		if(current_time.tm_hour >= 24) current_time.tm_hour = 0;
		horizonOffset = round((double) horizonOffsetTotal - (current_time.tm_hour * horizonOffsetPerHour));
		setTime(&current_time);
		realign_horizon();
	}
	
	void handle_select_single_click(ClickRecognizerRef recognizer, Window *window) 
	{
		enableTick = !enableTick;
		if(enableTick)
		{
			get_time(&current_time);
			setTime(&current_time);
			vibes_short_pulse();
		}
		else vibes_double_pulse();
	}
	
	void handle_down_single_click(ClickRecognizerRef recognizer, Window *window) 
	{	
		current_time.tm_hour--;
		if(current_time.tm_hour < 0) current_time.tm_hour = 23;
		horizonOffset = round((double) horizonOffsetTotal - (current_time.tm_hour * horizonOffsetPerHour));  
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

/**
* Resource initialisation handle function
*/
void handle_init(AppContextRef ctx)
{
	(void)ctx;

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
	
	//Set initial time so display isn't blank
	get_time(&current_time);
	setTime(&current_time); 
	
	horizonOffset = round(horizonOffsetTotal - (current_time.tm_hour * horizonOffsetPerHour));
	realign_horizon();
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
		}
	};
	app_event_loop(params, &handlers);
}

//Other functions
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

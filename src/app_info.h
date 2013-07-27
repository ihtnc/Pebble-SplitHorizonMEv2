#include "app_options.h"

#define APP_VER_MAJOR 1	
#define APP_VER_MINOR 4

#ifndef DEBUG
	#ifdef PHONE_HAS_HTTPPEBBLE
		#ifdef ANDROID
			#ifdef RISEFALL
				#define APP_NAME "Split Horizon ME v2 [ARF]"
			#else
				#define APP_NAME "Split Horizon ME v2 [AR]"
			#endif
		#else
			#ifdef RISEFALL
				#define APP_NAME "Split Horizon ME v2 [IRF]"
			#else
				#define APP_NAME "Split Horizon ME v2 [IR]"
			#endif
		#endif
	#else
		#ifdef RISEFALL
			#define APP_NAME "Split Horizon ME v2 [RF]"
		#else
			#define APP_NAME "Split Horizon ME v2 [R]"
		#endif
	#endif
#else
	#ifdef PHONE_HAS_HTTPPEBBLE
		#ifdef ANDROID
			#ifdef RISEFALL
				#define APP_NAME "Debug: SplitHorizonMEv2 [ARF]"
			#else
				#define APP_NAME "Debug: SplitHorizonMEv2 [AR]"
			#endif
		#else
			#ifdef RISEFALL
				#define APP_NAME "Debug: SplitHorizonMEv2 [IRF]"
			#else
				#define APP_NAME "Debug: SplitHorizonMEv2 [IR]"
			#endif
		#endif
	#else
		#ifdef RISEFALL
			#define APP_NAME "Debug: SplitHorizonMEv2 [RF]"
		#else
			#define APP_NAME "Debug: SplitHorizonMEv2 [R]"
		#endif
	#endif
#endif

#define APP_AUTHOR "ihopethisnamecounts"
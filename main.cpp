//Usa WIFI para transmitir o som

#define APP_TARGET							APP_TARGET_RASPBERRY_PICO
//#define TEST_SERVER_QUESTION
//#define APPLE2
/////////////////////////////////////////////////////
#define MCU_SHAREDBUFFER_SZ					 512
/////////////////////////////////////////////////////
//#define APP_CONNECT_ON_START				 0
#define APP_TEST_RECORDS_OPTION				 0
#define APP_DEBUG_MIC_LEVELS				 0
#define APP_DEBUG_MIC_RECORDING				 0
#define APP_DEBUG_NOTES						 0
//#define APP_DEBUG_FORMAT_AT_STARTUP

#define DISP_CLK_DELAY1     DELAY_IDLE(37)
#define DISP_CLK_DELAY2		DELAY_IDLE(7) // obrigatório para NFP096H-01AF

/////////////////////////////////////////////////////
//#define AUDIOREC_USES_EXTERNAL_BUFFER		 mcuSharedBuffer
//#define AUDIOREC_USES_EXTERNAL_BUFFERSZ	 MCU_SHAREDBUFFER_SZ
#define AUDIOPLAYER_AUXPIN					 PIN_BUZZER
#define ULTRA_FREQ_BEGIN					 440
#define ULTRA_FREQ_DELTA					 10
#define ULTRA_FREQ_END						 1000
#define ULTRA_FREQ_UNIT						 1
/////////////////////////////////////////////////////
#define ST7735_PARTNUMBER_IS				ST7735_NFP096H_01AY
#define DRAW_USES_BUFFER
/////////////////////////////////////////////////////
#include "mcu.h"
#include "mcu_util.h"
#include <cmath>

W0 record_tests(W0* args_=0);
W0 record_to_file(W0* args_=0);
W0 mic_levels(W0* args_=0);
W0 play_ultra_wave(W0* args_=0); 
W0 play_ultra_soundfile(W0* args_=0); 
W0 format_disk(W0* args_);

#include "app_main.h"
#include "app_notes.h"
#include "app_miclevels.h"
#include "app_formatssd.h"
#include "app_ultraspk.h"
 
#ifdef APP_ENABLE_TEST
	#include "tests.h"
#endif

SW32 main()
{
	mcu_init();				// inicializar PI PICO 
	init_pins();			// inicializar pinos
	sound_init();
	ui_init();
	ui_display_str(ui_strs_init, ui_strs_apptitle);
	ui_sound_emit(SOUND_WELCOME);
	fs_mount();
	config.initialized = true;

	PIN_WRITE(PIN_LED, 1); 
	
	#ifdef APP_DEBUG_FORMAT_AT_STARTUP
		//format_disk();
		//record_to_file();
	#endif


	#if APP_ENABLE_TEST
		tests::test();
	#else
		ui_sound_emit(SOUND_BEEP);
		ui_display_str(ui_strs_ready, ui_strs_apptitle);
		ui_menu_init();  

		#ifdef APP_USES_WIFI
			esp_clearRx();
		#endif

		while(1)
		{
			ui_menu_draw();
			while (ui_newAction==UI_BUTTON_NONE) 
			{
				tight_loop_contents(); 
			}
			ui_menu_update();
		}
	#endif

	return 0;
}


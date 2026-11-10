// DEBUG DE PINOS --------------------------------------------


//#define FS_USES_EXTERNAL_BUFFER		mcuSharedBuffer
////////////////////////////////////////////////////////////////////////////////
// interface


#if (APP_USE_USB_PERIPHERALS==1)
	#define mcu_log(STR_,...) printf(STR_,__VA_ARGS__);	
	
	#ifdef DEBUG_USB_PERIPHERALS
		#define DEBUG_USB(STR_,...)\
				sprintf(mcuSharedBuffer,"usb:" STR_, __VA_ARGS__);\
				esp_txStr(mcuSharedBuffer);
	#else
		#define DEBUG_USB(STR_,...) 
	#endif

#else
	#define DEBUG_USB(STR_,...) 
	#define mcu_log(STR_,...) 	
#endif

// Definiçoes ------------------------------------------------

#define APP_START_DELAY		500 // tempo para poder abrir o putty
#define DELAY_SHORT_		asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
#define DELAY_SHORT			DELAY_SHORT_ DELAY_SHORT_ DELAY_SHORT_ DELAY_SHORT_ DELAY_SHORT_  
 

// DEBUG  ------------------------------------------------
	//#define SD_DEBUG_DATA
	//#define SD_DEBUG_DATABITS
	//#define SD_DEBUG_CMDS 
	//#define SD_DEBUG_REPLIES
	//#define SD_DEBUG_CMD_STRS
	//#define SD_DEBUG_REPLY_TIME
	//#define SD_DEBUG_RXTX
	//#define SD_DEBUG_REGISTERS
	//#define SD_CRC_IS_FATAL

	//#define UI_DEBUG_BUTTONS 
	//#define UI_DEBUG_ICONS
	//#define UI_DEBUG_STRINGSOURCE
	//#define UI_DEBUG_LISTSCROLL

	//#define FS_DEBUG
	//#define FS_DEBUG DEBUG

// PEQUENO JOYSTICK -------------------------
	#define PIN_BUTTON_UP						IO_PIN_18
	#define PIN_BUTTON_DOWN						IO_PIN_21
	#define PIN_BUTTON_LEFT						IO_PIN_20
	#define PIN_BUTTON_RIGHT					IO_PIN_19
	#define PIN_BUTTON_ENTER					PIN_BUTTON_RIGHT
	#define PIN_ROTARY_ENCODER_DATA				IO_PIN_15
	#define PIN_ROTARY_ENCODER_CLK				IO_PIN_17


// External Audio ---------------------------
	#define AUDIOPLAYER_PIN_GENERIC_BUTTON		PIN_BUTTON_RIGHT
	#define PIN_BUZZER							IO_PIN_1
	#define PIN_MIC_IN_LEFT						IO_PIN_ADC2
	#define PIN_MIC_IN_RIGHT					IO_PIN_ADC1 
	#define PIN_ULTRASPEAKER					IO_PIN_ADC0
#undef PIN_LED
	#define PIN_LED								IO_PIN_16

// WIFI -------------------------------------
	#define ESP_UART							uart1
	#define PIN_ESP_RX							IO_PIN_5
	#define PIN_ESP_TX							IO_PIN_4
	// alternativo: usa IO1/IO2 do ESP 
	// para comunicar em vez da UART
	#define PIN_ESP_CLK							IO_PIN_5
	#define PIN_ESP_DATA						IO_PIN_4


////////////////////////////////////////////////////////////////////////////////
//ecrã 
	#define PIN_DISP_CLK			IO_PIN_10
	#define PIN_DISP_OUT			IO_PIN_11
	#define PIN_DISP_RES			IO_PIN_12
	#define PIN_DISP_DC				IO_PIN_13
	#define PIN_DISP_CS				IO_PIN_19
	#define PIN_DISP_BRIGHT			IO_PIN_ADC2

	#define DISP_PINS_DEF(X_)\
		X_(PIN_DISP_CLK,			 "p3")\
		X_(PIN_DISP_OUT,			 "p4")\
		X_(PIN_DISP_RES,			 "p5")\
		X_(PIN_DISP_DC,				 "p6")\
		X_(PIN_DISP_CS,				 "p7")

	#define UI_CACHEFILES_MAXSIZE	12		

	#include "st7735.h"
	#include "uiIcons.h"
	#include "draw.h"



////////////////////////////////////////////////////////////////////////////////
// disco SD

	#define SD_USES_FAST_CLOCK
	#define SD_ENABLE_DATA_CRC 0
	#define PIN_SD_CS				IO_PIN_2
	#define PIN_SD_OUT				IO_PIN_3
	#define PIN_SD_CLK				IO_PIN_6
	#define PIN_SD_IN				IO_PIN_7
	#include "sd.h"
	// no adaptador...
	#define SD_PINS_DEF(X_)\
		X_(PIN_SD_OUT,				"p5")\
		X_(PIN_SD_CLK,				"p6")\
		X_(PIN_SD_CS,				"p4")

////////////////////////////////////////////////////////////////////////////////
// sistema de ficheiros	#if (APP_TARGET==APP_TARGET_ARDUINO_NANO)
	#define FS_FAT_USE_CACHE_WITH_SIZE	512
	#define DEV_RD_BLOCK				sd_rd_block
	#define DEV_WR_BLOCK				sd_wr_block
	#include "fs.h"


#include "sound.h"


#if (defined(DEBUG_PINS_SD) || defined(DEBUG_PINS_DISP) || defined(DEBUG_PINS_AUDIO) || defined(DEBUG_PINS_PERIPHERAL))
	#define   DEBUG_PINS
#endif

#define APP_ENABLE_TEST_RECORD_WIFI			0
#define APP_ENABLE_TEST_RECORD_FILE			1
#define APP_ENABLE_TEST_PLAY_FILE			2
#define APP_ENABLE_TEST_PLAY_PWM_FILE		3
#define APP_ENABLE_TEST_DELETE_FILE			4
#define APP_ENABLE_TEST_SYNC				5
#define APP_ENABLE_TEST_LOADTAPE			6
#define APP_ENABLE_TEST_UART				7
#define APP_ENABLE_TEST_BUSES				8
#define APP_ENABLE_TEST_SERVER_QUESTION		9
#define APP_ENABLE_TEST_OPENAI_VOICE		10
#define APP_ENABLE_TEST_PWM_VOICE_RX		11


W1  fs_mount();
W0  ui_menu_draw();
W0  ui_uninstall_buttons();
W0  ui_install_buttons();
W0	ui_display_alert(const SW8* msg1_, const SW8* msg2_, W16 delay_=1000);

struct Config
{
	W1   initialized;
	W1   fsInitialized;
	//
	W1   txRecords;
	W1   ultraWaveOn;
	W1   connectedToServer;
	W1   debugNotes;
	//
	FP32 ultraFreq;
	W32  sampleRate;
	W32  ultraWaveFrameId;
	//
	W1   recStereo;
	W8   recSampleRateId;
	W8	 recSampleBitsId;

	Config()
	{
		recStereo = false;
		debugNotes = false;
		txRecords = true;
		recSampleRateId = 0;
		recSampleBitsId = 0;
		ultraWaveOn  = false;
		ultraFreq    = 440.0f;
		initialized  = false;
	}
} config;

#include "app_strings.h"
#include "app_config.h" 


#ifdef APP_USES_WIFI
	W0  esp_clearRx();
	W1  esp_txStr(const SW8* str_);
	W0  connect(W0*); 
	W1  checkIfConnected();
	struct ESP_PacketHeader;
	W1 esp_txPacket(ESP_PacketHeader& header);
	typedef W0(*ESP_SEND_PROGRESS_CALLBACK_)(W32,W32);
	W1 esp_txFile(const SW8*,  W1, ESP_SEND_PROGRESS_CALLBACK_);
#endif


	W0  ui_sound_emit(const W8 id_)
	{
		sound_emit(id_);
	} 


	#define UI_ANIMATE_DIR          40
	#define UI_ANIMATE_DIR_BUFF		1		// se 1 usa um buffer para animacao (+20K)
	#define UI_MENU_TEXT_RBORDER_DX 4
	#define UI_MENU_OPTION_DY		20
	#define UI_MENU_TITLE_DY		20
	#define UI_MENU_TITLE_PENX		2
	#define UI_MENU_TITLE_PENY		(fontBlockY-1)
	#define UI_MENU_TEXT_PENX		22
	#define UI_MENU_TEXT_PENY		(fontBlockY-1)
	#define UI_MENU_ICON_PENX		3
	#define UI_MENU_ICON_PENY		2

	#include "ui.h"


W1  checkIfFS()
{
	if (config.fsInitialized) return 0;
	ui_display_alert(ui_strs_notmounted, ui_strs_sd);
	return 1;
}
 
	W1  checkIfConnected()
	{
		#ifdef APP_USES_WIFI
			if (config.connectedToServer) return 0;
			ui_display_alert(ui_strs_notconnected, ui_strs_server);
			return 1;
		#endif
	}
	
	#ifdef APP_USES_WIFI
		#include "esp.h"  
		#define AUDIOREC_USES_ESP
	#endif

	W0  connect(W0* args_)
	{
		#ifdef APP_USES_WIFI
			config.connectedToServer = false;
			if (tryFunction(ui_strs_esp, esp_init, 5)) return;
			if (tryFunction(ui_strs_wifi, esp_wifi_connect, ESP_WIFI_RECONNECT_TRIES)) return;

			ui_display_str((SW8*)esp_LOCAL_IP, WIFI_SSID);
			delete[] esp_LOCAL_IP;

			if (tryFunction(ui_strs_server, esp_server_connect, ESP_SERVER_RECONNECT_TRIES)) return;
			ui_display_str((SW8*)SERVER_PORT, SERVER_IP);
			config.connectedToServer = true;
			esp_clearRx();
			return;
		#endif
	}
 
#include "rasp_pico/audiorec.h" 
#include "rasp_pico/audioplayer.h" 


W0  init_pins()
{
	PIN_INIT(PIN_LED)
	PIN_MODE(PIN_LED,			 PIN_MODE_OUTPUT)
	
	PIN_INIT(PIN_MIC_IN_LEFT)
	PIN_MODE(PIN_MIC_IN_LEFT,    PIN_MODE_INPUT)
	
	PIN_INIT(PIN_MIC_IN_RIGHT)
	PIN_MODE(PIN_MIC_IN_RIGHT,   PIN_MODE_INPUT)

	PIN_INIT(PIN_ULTRASPEAKER)
	PIN_MODE(PIN_ULTRASPEAKER,   PIN_MODE_OUTPUT)
	
	PIN_INIT(PIN_BUZZER)
	PIN_MODE(PIN_BUZZER,		 PIN_MODE_OUTPUT)
} 




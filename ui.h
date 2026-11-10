//#define UI_DEBUG_ITEMS
#include "uiHeads.h"

volatile UI_BUTTONS ui_newAction = UI_BUTTON_NONE;
UI_SIMPLE_PAGE	  *uiPageOptions; 
UI_SIMPLE_PAGE	  *uiPageRecord; 
Ret				  uiProgressBar;

W0  ui_menu_draw();
W0  ui_menu_init();
W0  ui_menu_update();
W0  ui_button_irq(uint GPIO_, uint32_t EVENTMASK_);
W0  ui_install_buttons();
W0  ui_uninstall_buttons();
W0  ui_init();


W0 ui_init_progressBar()
{
	const W16 uiProgressBarWidth = 40;
	uiProgressBar = Ret(uiProgressBarWidth, DISP_HEIGHT-12, DISP_WIDTH-uiProgressBarWidth*2, 2);
	uiProgressBar.expand(4);
	draw_rect(uiProgressBar, Color_Black);
	uiProgressBar.expand(-2);
	draw_rect(uiProgressBar, Color_White);
	DRAW_UPDATE
}
W0 ui_display_str(const SW8* strBot_=0, const SW8* strTop_=ui_strs_apptitle,  
				  SW32 off_=10, SW32 delay_=10)
{

	drawTextOutline = 2;
	drawTextShadowX = +3;
	drawTextShadowY = +3;
	drawTextOutlineColor = DISP_COLOR_MK_W16(0,0,0);
	drawTextShadowColor = DISP_COLOR_MK_W16(10,10,10);


	draw_clear(0xFFF);
	draw_displayFrameBlocks(40,DISP_COLOR_MK_W16(14,14,14),DISP_COLOR_MK_W16(15,15,15));
	draw_text_centered_with_offsets(0,-off_,strTop_,Color_White);
	if (strBot_)
		draw_text_centered_with_offsets(0,+off_,strBot_,Color_White);
	
	DRAW_UPDATE

	DELAY_MS(delay_);
	
	
	drawTextOutline = 0;
	drawTextShadowX = 0;
	drawTextShadowY = 0;
}
W0 ui_display_alert(const SW8* msg1_, const SW8* msg2_, W16 delay_)
{
	ui_display_str(msg1_, msg2_); 
	ui_sound_emit(SOUND_BEEP); 		
	DELAY_MS(delay_)
}
W0 ui_fatal(W8 module_, W8 error_)
{
	sprintf(mcuSharedBuffer, "Mod:%i Err:%i", module_, error_);
	draw_initBuffer();
	draw_error_msg(mcuSharedBuffer);
	DRAW_UPDATE
	STOP
}

//-----------------------------------------------------------------------------
//#define UI_JOYSTICK_PINS_AT_TOP
#ifdef UI_JOYSTICK_PINS_AT_TOP
	#define UI_JOYSTICK_PINS_BUTTON_UP		UI_BUTTON_UP
	#define UI_JOYSTICK_PINS_BUTTON_LEFT	UI_BUTTON_LEFT
	#define UI_JOYSTICK_PINS_BUTTON_DOWN	UI_BUTTON_DOWN
	#define UI_JOYSTICK_PINS_BUTTON_RIGHT	UI_BUTTON_RIGHT
#else
	#define UI_JOYSTICK_PINS_BUTTON_UP		UI_BUTTON_DOWN
	#define UI_JOYSTICK_PINS_BUTTON_LEFT	UI_BUTTON_RIGHT
	#define UI_JOYSTICK_PINS_BUTTON_DOWN	UI_BUTTON_UP
	#define UI_JOYSTICK_PINS_BUTTON_RIGHT	UI_BUTTON_LEFT
#endif

 
W0  ui_menu_init()
{
	 W8 cnt_audio_record = 1;
	 W8 cnt_sd = 1;
	 W8 cnt_wifi = 
		#ifdef APP_USES_WIFI 
		 1;
		#else 
		 0;
		#endif
	 W8 cnt_ultrawaves =
		#ifdef APP_USES_ULTRAWAVES 
		 1;
		#else 
		 0;
		#endif

	uiButton           = UI_BUTTON_NONE;
	uiPageOptions      = new UI_SIMPLE_PAGE();
	if (!uiPageOptions) fatal(UI, UI_ERROR_ALLOC);

	uiPageRecord	   = new UI_SIMPLE_PAGE();
	if (!uiPageRecord) fatal(UI, UI_ERROR_ALLOC);

	UI_ITEM_VALUE* itemValue;
	UI_SIMPLE_PAGE* page;
	W8 menu_items_number;

	//////////////////////////////////////////////////////
	page = uiPageOptions;
	
	menu_items_number = 
				cnt_audio_record +
				cnt_sd +
				cnt_ultrawaves*3 + 
				cnt_wifi ;

	page->init((SW8*)ui_strs_menu, menu_items_number); 
		
			page->addItem(ui_strs_recordmics,	   (UI_PAGE*)uiPageRecord);
			page->addItem(ui_strs_formatDisk,	   format_disk);
			page->addItem(ui_strs_playultrasnd,    play_ultra_soundfile);
			page->addItem(ui_strs_playultrawave,   new UI_ITEM_VALUE(UI_ITEM_VALUE_BOOL, &(config.ultraWaveOn)));
			itemValue = new UI_ITEM_VALUE(UI_ITEM_VALUE_FLT32_RANGE_ENUM, &(config.ultraFreq));
			itemValue->FP32range.start =  ULTRA_FREQ_BEGIN;
					itemValue->FP32range.delta =  ULTRA_FREQ_DELTA;
					itemValue->FP32range.end   =  ULTRA_FREQ_END;
					itemValue->sufixStr = (ULTRA_FREQ_UNIT==1)?ui_strs_hz:ui_strs_khz;
			page->addItem(ui_strs_freq, itemValue);

		#ifdef APP_USES_WIFI
			page->addItem(ui_strs_reconnect, connect);
		#endif
	
	//////////////////////////////////////////////////////
	page = uiPageRecord;
	menu_items_number

			= cnt_audio_record *3 + 
			  cnt_wifi * 1  + 
			  cnt_audio_record * 2;


		page->init((SW8*)ui_strs_record, menu_items_number, uiPageOptions);

			page->addItem(ui_strs_record,     record_to_file);
			page->addItem(ui_strs_stereo,     new UI_ITEM_VALUE( &(config.recStereo)) );
			page->addItem(ui_strs_bits,       new UI_ITEM_VALUE( &(config.recSampleBitsId), sampleBitsArr, 0, 1));
			#ifdef APP_USES_WIFI
				page->addItem(ui_strs_txrecords,  new UI_ITEM_VALUE( &(config.txRecords)));
			#endif
		
			page->addItem(ui_strs_debugnotes, new UI_ITEM_VALUE( &(config.debugNotes)));
			page->addItem(ui_strs_miclevels,  mic_levels);

	uiCurrPage	= uiPageOptions;
	ui_menu_draw();
	ui_install_buttons();
}
W0  ui_menu_draw()
{
	draw_initBuffer();
	uiCurrPage->draw();
	DRAW_UPDATE
}
W0  ui_menu_update()
{
	if (ui_newAction==UI_BUTTON_NONE) return;

	const UI_BUTTONS button = ui_newAction;
	ui_uninstall_buttons();
	if (button==UI_BUTTON_ENTER)
	{
		ui_uninstall_buttons();
		DELAY_MS(200);
		ui_install_buttons();
	}
	uiCurrPage->action(button); 
	ui_menu_draw(); 
	ui_install_buttons();
}

W1 lastData;
W0  ui_button_irq(uint GPIO_, uint32_t EVENTMASK_)
{
	//printf("\nBirq");
	switch(GPIO_)
	{
		case PIN_BUTTON_UP:	
			ui_newAction = UI_BUTTON_UP;
		break;
		case PIN_BUTTON_DOWN:	
			ui_newAction = UI_BUTTON_DOWN;
		break;
		case PIN_BUTTON_LEFT:	
			ui_newAction = UI_BUTTON_LEFT;
		break;
		case PIN_BUTTON_RIGHT:	
			ui_newAction = UI_BUTTON_RIGHT;
		break;
		case PIN_ROTARY_ENCODER_CLK:	
		{
			//printf("*");
			W32 up = 0, down=0;
			const W8 clkNow = PIN_READ(PIN_ROTARY_ENCODER_CLK);
			while(clkNow == PIN_READ(PIN_ROTARY_ENCODER_CLK)) { }
			
			const W8   data = PIN_READ(PIN_ROTARY_ENCODER_DATA);
			
			if ( (clkNow==data) )
					up++;
				else
					down++;
			const W1 isUp = up>=down;
			//printf("\nUp:%i Down:%i ? %i", up , down, isUp?1:0);
			
			DELAY_MS(40);
			ui_newAction = isUp?UI_BUTTON_ROTATE_UP:ui_newAction = UI_BUTTON_ROTATE_DOWN;
		}
		break;
	}
	PIN_IRQ_HANDLER_ACK
}
W0  ui_uninstall_buttons()
{
	PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_UP)
	PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_DOWN)
	PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_LEFT)
	PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT)
	PIN_UNINSTALL_IRQ_ONFALL(PIN_ROTARY_ENCODER_CLK)
	ui_newAction = UI_BUTTON_NONE;
}
W0  ui_install_buttons()
{
	ui_uninstall_buttons();
	
	PIN_INIT(PIN_BUTTON_LEFT)
	PIN_INIT(PIN_BUTTON_RIGHT)
	PIN_INIT(PIN_BUTTON_UP)
	PIN_INIT(PIN_BUTTON_DOWN)
	PIN_INIT(PIN_ROTARY_ENCODER_DATA)
	PIN_INIT(PIN_ROTARY_ENCODER_CLK)
	PIN_MODE(PIN_BUTTON_LEFT,			PIN_MODE_INPUT)
	PIN_MODE(PIN_BUTTON_RIGHT,			PIN_MODE_INPUT)
	PIN_MODE(PIN_BUTTON_UP,				PIN_MODE_INPUT)
	PIN_MODE(PIN_BUTTON_DOWN,			PIN_MODE_INPUT)
	PIN_MODE(PIN_ROTARY_ENCODER_DATA,     PIN_MODE_INPUT)
	PIN_MODE(PIN_ROTARY_ENCODER_CLK,    PIN_MODE_INPUT)

	
	PIN_SET_PULLUP(PIN_BUTTON_LEFT)
	PIN_SET_PULLUP(PIN_BUTTON_RIGHT)
	PIN_SET_PULLUP(PIN_BUTTON_UP)
	PIN_SET_PULLUP(PIN_BUTTON_DOWN)
	PIN_SET_PULLUP(PIN_ROTARY_ENCODER_DATA)
	PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_LEFT,  ui_button_irq)
	PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT, ui_button_irq)
	PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_UP,    ui_button_irq)
	PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_DOWN,  ui_button_irq)
	PIN_INSTALL_IRQ_ONFALL(PIN_ROTARY_ENCODER_CLK,  ui_button_irq)
	//PIN_INSTALL_IRQ_ONRISE(PIN_ROTARY_ENCODER_CLK,  ui_button_irq)
}
W0  ui_init()
{
	draw_init(); 
	W8 y;  
	#ifdef UI_DEBUG_ICONS
		y=ICON_SIZE;
		W8 x=ICON_SIZE;
		for (W8 i=0; i< ICON_NUMBER; i++)
		{	
			draw_icon<1>(x, y, (ICON)i);
			x+=ICON_SIZE;
			if (x > DISP_WIDTH-ICON_SIZE) { x=ICON_SIZE; y+=ICON_SIZE; }
		}
		y+=ICON_SIZE; 
	#else
		y=(DISP_WIDTH-fontBlockY)/2 - fontBlockY; 
	#endif
	draw_clear(0xFFFF);
	//ui_display_str(0,0); 
	DELAY_MS(200);
}
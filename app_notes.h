
#if (APP_TEST_RECORDS_OPTION==1)
W0  record_tests(W0* args_)
{
	config.txRecords = false;
	format_disk(0);

	for (W8 r=0; r < 4; r++)
	{
		config.recSampleRateId = r;
		for (W8 b=0; b< 2; b++)
		{
			config.recSampleBitsId = b;
			record_to_file();
		}
	}

}
#endif


// RECORD TO FILE
audio::AudioRec *audiorec;
struct Notes
{
public:
	W32* arr;
	W16  sz;
	W16  it;
	SW8  filename[32];
	Notes()
	{
		sz = 10;
		it = 0;
		arr = new W32[sz];
	}
	W0 addNote(W32 stamp_)
	{
		#if (APP_DEBUG_NOTES==1)
			printf("*n");
		#endif
		if (it==sz)
		{
			sz+=10;
			W32 *n = new W32[sz];
			memcpy(n, arr, (sz-10)*sizeof(W32));
			delete[] arr;
			arr = n;
		}
		arr[it]=stamp_;
		it++;
	}
	W0 writeFile(const SW8* inFilename_)
	{
		//printf("\nwrite file %i", it);
		W32 bytesWR;
		FSNode node;
		SW8 *buf = (SW8 *)mcuSharedBuffer;
		strcpy(filename, inFilename_);
		SW8* fn = filename + strlen(filename);
		fn[-1]='T';
		fn[-2]='X';
		fn[-3]='T';
		ui_display_str(filename, ui_strs_writingnotes);
		 
		#if (APP_DEBUG_NOTES==1)
			printf("\nNotes: %i ", it);
		#endif

			
		#if (APP_DEBUG_NOTES==1)
			printf("create ");
		#endif
		fs_createFileNode(buf, &node);

		#define NOTES_WRITE_FILE(STR_,...) \
			sprintf(buf, STR_ "\n", __VA_ARGS__); \
			fs_rw_(&node, (W8*)buf, strlen(buf), bytesWR, FS_OP_WRITE);
		#define NOTES_BLOCKS_TO_SECS(BLOCKS_)\
			(((FP32)BLOCKS_*audiorec->AUDIO_NUM_SAMPLES)/(FP32)(audiorec->AUDIO_SAMPLE_RATE))
		
		NOTES_WRITE_FILE("Bits:%i", audiorec->AUDIO_SAMPLE_RATE )
		NOTES_WRITE_FILE("Rate:%i", audiorec->AUDIO_SAMPLE_BITS )
		NOTES_WRITE_FILE("Chans:%i", audiorec->AUDIO_CHANS )
		NOTES_WRITE_FILE("SmplBlks:%i", audiorec->sampleBlocks )
		NOTES_WRITE_FILE("WriBlks:%i", audiorec->writtenBlocks )
		NOTES_WRITE_FILE("FstWrite:%i", (audiorec->fastWrite?1:0) )
		NOTES_WRITE_FILE("Total:%3.2f secs", NOTES_BLOCKS_TO_SECS(audiorec->writtenBlocks) )
		NOTES_WRITE_FILE("Notes:%i", it)
		
		#if (APP_DEBUG_NOTES==1)
			printf("wr ");
		#endif

		for (SW32 i=0; i< it; i++)
		{
			
			#if (APP_DEBUG_NOTES==1)
				printf("%i/%i ",i, it);
			#endif
			// 1 samplingBlock == AUDIO_NUM_SAMPLES
			
			//  AUDIO_SAMPLE_RATE => 1 sec
			//	  samples         => ? sec
			// ? = samples/AUDIO_SAMPLE_RATE
			const FP32 secs = NOTES_BLOCKS_TO_SECS(arr[i]);
			//printf("\nsmpBlock:%i secs:%3.2f", sb, secs);
			NOTES_WRITE_FILE("%3.2f",secs)
			//printf("\nbytes wr: %i %s", bytesWR, filename);
		}
			
		#if (APP_DEBUG_NOTES==1)
			printf("cl ");
		#endif
		fs_close(&node);

		
		#if (APP_DEBUG_NOTES==1)
			printf("done");
		#endif
		#undef NOTES_WRITE_FILE
		#undef NOTES_BLOCKS_TO_SECS

	}
	~Notes()
	{
		delete arr;
	}
} *notes;
W0  record_callback_buttonIrq(uint GPIO_, uint32_t EVENTMASK_)
{
	if (GPIO_==PIN_BUTTON_DOWN) // porque?
	{
		//printf("\nb:%i", audiorec->writtenBlocks);
		notes->addNote(audiorec->writtenBlocks);
		if (config.debugNotes)
		{
			SW8 noteStr[32];
			sprintf(noteStr, "Notes:%i", notes->it);
			ui_display_str(noteStr, ui_strs_recordmics);
		}
		PIN_IRQ_HANDLER_ACK
	}
	else if (GPIO_==PIN_BUTTON_RIGHT)
	{
		audiorec->recording=false;
		PIN_IRQ_HANDLER_ACK
	}
}
W0  record_callback(W8 cause_, W0* args_)
{
	using namespace audio;
	switch (cause_)
	{
		case AUDIOREC_CAUSE_WAIT: 
			ui_display_str(ui_strs_wait, ui_strs_recordmics);
		break;
		case AUDIOREC_CAUSE_START: 
			ui_display_str(ui_strs_recording, ui_strs_recordmics);
			sound_alert(SOUND_BEEP);  
			PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_DOWN)
			PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT)
			PIN_INIT(PIN_BUTTON_DOWN)
			PIN_INIT(PIN_BUTTON_RIGHT)
			PIN_MODE(PIN_BUTTON_DOWN, PIN_MODE_INPUT)
			PIN_MODE(PIN_BUTTON_RIGHT, PIN_MODE_INPUT)
			PIN_SET_PULLUP(PIN_BUTTON_DOWN)
			PIN_SET_PULLUP(PIN_BUTTON_RIGHT)
			PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_DOWN, record_callback_buttonIrq)
			PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT, record_callback_buttonIrq)
		break;
		case AUDIOREC_CAUSE_STOP: 
			ui_display_str(ui_strs_stopping, ui_strs_recordmics);
			sound_alert(SOUND_STOP);  
		break;
	}
}
W0  record_to_file_esp_progbar(W32 done_, W32 total_)
{
	if ((done_%4096==0)||(done_==total_))
		draw_progressbar(uiProgressBar, Color_Blue,  done_, total_);
}
W0  record_to_file_esp_transmit()
{
#ifdef APP_USES_WIFI
	if (!config.connectedToServer) connect();
	
	if (checkIfConnected()) return;

	// transmitir ficheiro de audio <!> nao está a funcionar
	ui_display_str(audiorec->currFilename, ui_strs_sending);
	ui_init_progressBar();
	esp_txFile(audiorec->currFilename, true, record_to_file_esp_progbar);

	DELAY_MS(220);

	// transmitir ficheiro de notas
	ui_display_str(notes->filename, ui_strs_sending);
	ui_init_progressBar();
	if (esp_txFile(notes->filename, true, record_to_file_esp_progbar))
	{
		ui_display_str(ui_strs_txrecords, ui_strs_fail);
		STOP
	}
#endif
}
W0  record_to_file(W0* args_)
{
	ui_uninstall_buttons();
	ui_display_str(ui_strs_wait, ui_strs_recordmics);
	DELAY_MS(500);

	// *** iniciar gravação para próximo ficheiro ".WAV"
	using namespace audio;
	notes    = new Notes();
	audiorec = new AudioRec();
	audiorec->recordingCallbackFx = record_callback;
	const W1 res = audiorec->init(true, 
									sampleBitsArr[config.recSampleBitsId], 
									sampleRatesArr[config.recSampleRateId], 
									PIN_MIC_IN_LEFT,
									config.recStereo?PIN_MIC_IN_RIGHT:0xFF,
									-1,
									AUDIOREC_DEFAULT_SIL_MIN_TH,
									AUDIOREC_DEFAULT_SIL_MAX_TH,
									2); 

	if (res)
	{
		ui_display_str(ui_strs_alloc, ui_strs_fail);
		STOP
	}
	audiorec->recordToNextFile();
	ui_uninstall_buttons();

	// *** escrever ficheiro de notas (difere na extensão "TXT")
	//sprintf(audiorec->currFilename, "test.wav"); ** teste 
	notes->writeFile(audiorec->currFilename);

	// *** se transmitir ficheiro de audio e de notas...
	#ifdef APP_USES_WIFI
		if (config.txRecords) record_to_file_esp_transmit();
	#endif 

skip:
	//PIN_WRITE(PIN_LED, 1);
	delete notes;
	delete audiorec;

}
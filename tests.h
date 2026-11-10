
		//irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler_); 
		//irq_set_enabled(PWM_IRQ_WRAP, true);
		//pwm_set_irq_enabled(slice, true);
	   // set_sys_clock_khz(clockfreq, true); 
		//irq_set_enabled(PWM_IRQ_WRAP, false);
		//pwm_set_enabled(slice, false);
namespace tests
{

	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PWM_VOICE_RX)
		W0 voice_pwm_rx()
		{
			while(1)
			{
				printf("\nVoice rx");
				::openai_test_transcription();
			}
		}
	#endif

	#if ((APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_FILE)||(APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_WIFI))
		W0 record_to_file_callback(W8 cause_, W0* args_)
		{
			switch (cause_)
			{
				case audio::AUDIOREC_CAUSE_WAIT:
					printf("\nRec: Wait...");
				break;
				case audio::AUDIOREC_CAUSE_START:
					printf("\nRec: Start...");
				break;
				case audio::AUDIOREC_CAUSE_STOP:
					printf("\nRec: Stop...");
				break;
			}
		}
	#endif

	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_WIFI)
		W0 esp_transmit_sound()
		{
			audio::AudioRec* audioRec = new audio::AudioRec();
			audioRec->init  (false,         //fastDiskWrite_
							 8,              //AUDIO_SAMPLE_BITS_ 
							 4000,           //AUDIO_SAMPLE_RATE_ 
							 PIN_MIC_IN,     //PIN_ADC_LEFT_  
							 0xFF,           //PIN_ADC_RIGHT_ 
							 -1,             //stopOnSilenceSecs_
							 100,            //...silenceThMin_
							 300,            //...silenceThMax_
							 2               //...LBA_CLUSTERS_
							 );

		
		
			//audioRec->measureSilence(3);
			audioRec->callbackFx = 0;
			audioRec->recordingCallbackFx = record_to_file_callback;
			printf("\nTransmit Sound");

			while(1)
			{
				audioRec->espTransmit(true ); // wait for button
			}
			delete audioRec;
		}
	#endif

	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_FILE)
	
		W0 record_to_file(const SW8* name_)
		{
			audio::AudioRec* audioRec = new audio::AudioRec();
			audioRec->init  (false,         //fastDiskWrite_
							 8,              //AUDIO_SAMPLE_BITS_ 
							 4000,           //AUDIO_SAMPLE_RATE_ 
							 PIN_MIC_IN,     //PIN_ADC_LEFT_  
							 0xFF,           //PIN_ADC_RIGHT_ 
							 -1,             //stopOnSilenceSecs_
							 100,            //...silenceThMin_
							 300,            //...silenceThMax_
							 2               //...LBA_CLUSTERS_
							 );

			//audioRec->measureSilence(3);
			audioRec->callbackFx = 0;
			audioRec->recordingCallbackFx = record_to_file_callback;

			while(1)
			{
				audioRec->recordToFile(
										"\\record.wav", 
										true			 // wait for button
									  ); 
			}
			delete audioRec;

		}
	#endif
	
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PLAY_FILE)
		SW16 usAdjust = -1;
		audio::AudioPlayer *player;
		W0 audio_irqHandler(uint GPIO_, uint32_t EVENTMASK_)
		{
			//player->stop();
			//DELAY_MS(10);
			//player->AUDIOPLAYER_PWM_FRAMES++;
			//printf("\npwm:%i", player->AUDIOPLAYER_PWM_FRAMES);
			player->SAMPLE_VALUE_MIN++;
			//usAdjust--;
			printf("\npwm:%i", player->SAMPLE_VALUE_MIN);
			gpio_acknowledge_irq(GPIO_, EVENTMASK_);

		}
		W0 play_audiofile(const SW8* name_)
		{
			printf("\n %s", name_);
			player->init(PIN_BUZZER);
			player->open(name_);
			player->pwmPlay(usAdjust);
		}
		W0 play_audiofiles()
		{
			PIN_INIT(AUDIOPLAYER_PIN_GENERIC_BUTTON)
			PIN_MODE(AUDIOPLAYER_PIN_GENERIC_BUTTON,   PIN_MODE_INPUT)
			PIN_SET_PULLUP(AUDIOPLAYER_PIN_GENERIC_BUTTON)
			PIN_INSTALL_IRQ_ONFALL(AUDIOPLAYER_PIN_GENERIC_BUTTON, test_audio_irqHandler)
	
			player = new audio::AudioPlayer();
			while(1)
			{
				//sound_emit(SOUND_BEEP);
				//play_audiofile("\\testf.wav");
				ui_sound_emit(SOUND_BEEP);
				play_audiofile("\\test4.wav");
				//sound_emit(SOUND_BEEP);
				//play_audiofile("\\music.wav");
				//sound_emit(SOUND_BEEP);
				//play_audiofile("\\test4sm.wav");
				//sound_emit(SOUND_BEEP);
				//play_audiofile("\\musicsm.wav");
				//sound_emit(SOUND_BEEP);
				//play_audiofile("\\test4b.wav");
			}
		delete player;
	}
	#endif
	
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PLAY_PWM_FILE)
		W0 pwm()
		{
			audio::AudioPlayer *player = new audio::AudioPlayer();
			player->open(mcuSharedBuffer);  
			player->digitalPlay(PIN_AUDIO_LOAD);  
			player->open("\\voice8.wav");
			player->pwmPlay(PIN_AUDIO_LOAD);
			player->open("\\voice84.wav");
			player->pwmPlay(PIN_AUDIO_LOAD);
			player->open("\\voice82.wav");
			player->pwmPlay(PIN_AUDIO_LOAD);
			delete player;
		}
	#endif

	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_DELETE_FILE)
	W1 delete_files()
	{
		if (fs_delete("\\test.wav")) return 1;
		if (fs_delete("\\test2.wav")) return 1;
		return 0;
	}
	#endif
	
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_SYNC)
		W0 load_sync()
		{
			AUDIO_SELECT_MCU
			DELAY_MS(500);
			audio::AudioPlayer *player = new audio::AudioPlayer();
			while(1)
			{ 
				player->open("\\sync.wav");  
				player->digitalPlay(PIN_AUDIO_LOAD);  
			}
			delete player;
		}
	#endif
	
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_LOADTAPE)
		W0 load_button_irqhandler(uint GPIO_, uint32_t EVENTMASK_)
		{
			//audio_adj++;
			SW8 cstr[64];

			//sprintf(cstr, "LET A=%i", audio_adj );
			//player->stop();
			//printf("%s",cstr);

			apple2::sendString(cstr);
			apple2::sendChar(13);
	
			//audio_it_ext+=2; 
			gpio_acknowledge_irq(GPIO_, EVENTMASK_);
		} 
		W0 load_tape(const SW8* filename_)
		{
	
			//PIN_INIT(AUDIOPLAYER_PIN_GENERIC_BUTTON)
			//PIN_MODE(AUDIOPLAYER_PIN_GENERIC_BUTTON,   PIN_MODE_INPUT)
			//PIN_SET_PULLUP(AUDIOPLAYER_PIN_GENERIC_BUTTON)
			//PIN_INSTALL_IRQ_ONFALL(AUDIOPLAYER_PIN_GENERIC_BUTTON, load_button_irqhandler)
	
			//mcu_log("\nplaying",0);
	 
			//for (W32 i=0; i< 4+audio_it_ext; i++)
			//{
			//	PIN_WRITE(PIN_LED, 0);
			//	DELAY_MS(500)
			//	PIN_WRITE(PIN_LED, 1);
			//	DELAY_MS(500)
			//}
		
			DELAY_MS(APP_START_DELAY)
			PERIPHERAL_SELECT_MCU
			AUDIO_SELECT_MCU
			DELAY_MS(500);
			audio::AudioPlayer *player = new audio::AudioPlayer();
			player->open(filename_);
			sprintf(mcuSharedBuffer, "load '%s' %uHz %uBits", filename_, 
						player->waveHeader.wfex.nSamplesPerSec, 
						player->waveHeader.wfex.wBitsPerSample);
			apple2::sendString(mcuSharedBuffer,true); 
			DELAY_MS(300);
			apple2::sendString("LOAD\r"); 
			DELAY_MS(500);
			player->digitalPlay(PIN_AUDIO_LOAD);
			delete player;
		}
	#endif
	
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_UART)
		W0  uart()
		{
			for (SW32 i=0; i< 3; i++)
			{
				PIN_WRITE(PIN_LED, 1);
				mcu_log("\r\n hello %i \r\n", i);
				DELAY_MS(250);
				PIN_WRITE(PIN_LED, 0);
				DELAY_MS(250)
			}
			mcu_log("started\r\n", 0);
		}
	#endif
		
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_BUSES)
		W0  buses()
		{
	
			PERIPHERAL_SELECT_MCU
	
			W8 byte = 0;
			SW8 str[16];
			while(1)
			{
				for (W32 i=0; i< 256; i++, byte++)
				{
					//sprnValueBits<W8,8>(byte, str);
					//mcu_log("byte %X | %s  \r\n", byte, str)
					W8 b = byte;
					for (W32 j=0; j< 8; j++)
					{
						PIN_WRITE(PIN_PERIPHERAL_DATA, (b&1));
						PIN_WRITE(PIN_PERIPHERAL_CLK,  1);		// 1.93MHz
						DELAY_SHORT
						PIN_WRITE(PIN_PERIPHERAL_CLK,  0);
						DELAY_SHORT
						b>>=1;
					}
					LATCH_KEYB
					LATCH_GAMEPAD
					//PIN_WRITE(PIN_LED, 1);
					//DELAY_MS(500);
					//PIN_WRITE(PIN_LED, 0);
					//DELAY_MS(500);
				}
			}
		} 
	#endif	
		 
	#if (APP_ENABLE_TEST==APP_ENABLE_TEST_SERVER_QUESTION)
		W0  serverQuestion()
		{
			while(1)
			{
				SW8* str;
				W32 questionLen;
				esp_send1(0xBB);
				esp_receive(&questionLen, 4);
				esp_send1(0xAA);
				str = new SW8[questionLen];
				esp_receive(str, questionLen);
				openai_text(str);
				delete[] str;
			}
		}
	#endif

	W0  test()
	{
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PWM_VOICE_RX)
			voice_pwm_rx();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_OPENAI_VOICE)
			openai_voice();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_WIFI)
			esp_transmit_sound();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_RECORD_FILE)
			record_to_file("record.wav");
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PLAY_FILE)
			play_audiofiles();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_PLAY_PWM_FILE)
			pwm();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_DELETE_FILE)
			delete_files();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_SYNC)
			load_sync();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_LOADTAPE)
			load_tape("test.wav");
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_UART)
			uart();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_BUSES)
			buses();
		#endif
		#if (APP_ENABLE_TEST==APP_ENABLE_TEST_SERVER_QUESTION)
			// ser o servidor a enviar a questao
			// e fazer forward ao servidor a sua
			// questao com ChatGPT
			serverQuestion();
		#endif
	}
};
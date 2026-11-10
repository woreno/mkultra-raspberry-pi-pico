#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/rtc.h"
#include "hardware/timer.h"
#include <sstream>
#include <string>
#include <ostream> 
#include "sound.h"


// opções
	//#define AUDIOREC_USES_BLINK
	#define AUDIOREC_USE_STATS
	#define AUDIOREC_USE_TRIANGLE_TEST //escreve um triangulo em vez do som

#define AUDIOREC_TEST_IMMEDIATE_SAVING
	// em vez de multicore, a cada array de samples, grava imediatamente

#define DEBUG_AUDIOREC(X_,...)  { printf("\nAR:",0); printf(X_, __VA_ARGS__); fflush(stdout);}
#define AUDIOREC_LOG(X_,...)    { printf("\nAR:",0); printf(X_, __VA_ARGS__); fflush(stdout);}
//#define AUDIOREC_LOG(X_,...)   {}

#ifdef AUDIOREC_USE_STATS
	#define AUDIOREC_LOG_STAT(X_,...) { printf("\n",0); printf(X_, __VA_ARGS__); fflush(stdout);}
#else
	#define AUDIOREC_LOG_STAT(X_,...) {}
#endif
#define AUDIOREC_PREALOC_CLUSTERS	(-1) 
#define AUDIOREC_DEFAULT_SIL_MIN_TH	(2048-150)
#define AUDIOREC_DEFAULT_SIL_MAX_TH (2048+150)
	// para usar isto é preciso escrever os clusters nao usados a zero! (por implementar)

namespace audio {

class AudioRec;
AudioRec		*audioRecInstance;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

	enum		AUDIOREC_TARGET		 { AUDIOREC_TARGET_FILE, 
									   AUDIOREC_TARGET_ESP,
									   AUDIOREC_TARGET_MEASURE};
	// 12-bit unsigned para 16-bit signed
	//#define AUDIO_ENCODE(X_)		(((SW16)( ((W16)(X_))<<3 ))-32767)
	#define		AUDIO_ENCODE_16(X_)	 ((SW16)((X_)&0xFFF)-2048)	// 16-bit com sinal
	#define		AUDIO_ENCODE_8(X_)	 (((X_)&0xFFF)>>4)			// 8-bit sem sinal
	
	enum AUDIOREC_CAUSES { AUDIOREC_CAUSE_START, AUDIOREC_CAUSE_WAIT, AUDIOREC_CAUSE_BLOCK, AUDIOREC_CAUSE_STOP };

class AudioRec
{
public:
	W8  PIN_ADC_LEFT;
	W8  PIN_ADC_RIGHT;
	W8  SAMPLE_DMA_CHANNEL;
	W8  AUDIO_CHANS;
	W8  AUDIO_SAMPLE_BITS;
	W16 AUDIO_SAMPLE_RATE;
	W16 AUDIO_NUM_SAMPLES;
	SW8 currFilename[16]; 

	W0(*recordingCallbackFx)(W8, W0*);
	W0* recordingCallbackFxArgs;

	static W0 recordingCallbackFxDefault(W8 cause_, W0* args_)
	{
		if (cause_==AUDIOREC_CAUSE_START)
			sound_alert(SOUND_BEEP);  
	}

	W0(*callbackFx)(W0*);
	W0* callbackFxArgs;


	AUDIOREC_TARGET	    target;

	volatile W0 		*diskBuffer; 
	volatile W16		*dmaBuffer;
	FSNode				fileNode; 
	struct				FastWrite
	{
		W32 fastWriteBytesExtra;
		W32	beginCluster, 
			currCluster, 
			lastCluster, 
			bytesExtra;
		W32	discardedBlocks;
	} *fastWrite;
	W32					bytesToWrite;
	volatile W32		sampleBlocks;
	volatile W32		writtenBlocks;
	volatile			W1 readyToWrite;
	volatile			W1 recording; 

	struct Silence
	{
		W32 counter;
		SW32 maxSamplesOnSilenceToStart;
		SW32 maxSamplesOnSilenceToStop;
		
		struct Measure
		{
			W16	minTh, maxTh;
			W32 maxSamples;
		} 
		measure;
		
		W0 reset()
		{
			counter = 0;
			maxSamplesOnSilenceToStart = 0;
			maxSamplesOnSilenceToStop = 0;
			measure.minTh = measure.maxTh = measure.maxSamples = 0;
		}
		W0 print()
		{
			AUDIOREC_LOG("Sil Th - min:%i max:%i", measure.minTh, measure.maxTh);
		}

		Silence()
		{
			reset();
		}
	} *silence;


	#ifdef AUDIOREC_USE_STATS
	class Stats
	{
	public:
		W32 				t0 , core1_t0, core1_t1;
		FP    				idleTimeSum;
		FP    				recordingTimeSum;
		FP    				samplingTimeSum;
		
		inline W0 core1_wait_begin(...)
		{
			core1_t0 = time_us_32();
		}
		inline W0 core1_wait_end(...)
		{
			core1_t1 = time_us_32();
			idleTimeSum += core1_t1 - core1_t0;
		}
		inline W0 core1_write_end(...)
		{
			recordingTimeSum += time_us_32() - core1_t1;
		}
		inline W0 core0_dma_update(...)
		{
			const W32 t1 = time_us_32();
			samplingTimeSum +=  t1 - t0;
			t0 = t1;
		}
		inline W0 core0_finish(W32 sampleBlocks_, W32 writtenBlocks_, W32 AUDIO_SAMPLE_RATE, W32 AUDIO_NUM_SAMPLES_)
		{
			const float sampleBlkSpeed = (W32)(samplingTimeSum/(float)sampleBlocks_);
			const W32 sampleSpeed      = sampleBlkSpeed/AUDIO_NUM_SAMPLES_;
			const W32 freq             = 1e6/(float)sampleSpeed;
			const W32 writeSpeed       = (W32)(recordingTimeSum/(float)sampleBlocks_);
			const W32 idleTime         = (W32)(idleTimeSum/(float)sampleBlocks_);
			const W32 totalTime        = (W32)(((float)sampleBlocks_ * AUDIO_NUM_SAMPLES_)/(float)(AUDIO_SAMPLE_RATE));

			datetime_t  timeT1;
			rtc_get_datetime(&timeT1);

			AUDIOREC_LOG_STAT(" time: %uh %umin %usec",
					(W32)timeT1.hour , (W32)timeT1.min, (W32)timeT1.sec)

			AUDIOREC_LOG_STAT(" computed: %i sec", totalTime)
	
			AUDIOREC_LOG_STAT(" %i blks each with %i samples / %i blks written",	
					sampleBlocks_, AUDIO_NUM_SAMPLES_, writtenBlocks_)

			AUDIOREC_LOG_STAT(" discarded: %i blks",	
					sampleBlocks_ - writtenBlocks_)
	
			AUDIOREC_LOG_STAT(" %i usec/block"	//1
					"\n %i usec/sample (freq: %iHz)" //2,3
					"\n idle:%i usec/block"	//4
					"\n write:%i usec/block", //5
					(W32)sampleBlkSpeed,   //1
					sampleSpeed,			//2
					freq,					//3
					idleTime,             //4
					writeSpeed);			   //5

		}
		W0 reset()
		{
			idleTimeSum      = 0;
			recordingTimeSum = 0;
			samplingTimeSum  = 0;
	
			//datetime_t timeT0;
			//timeT0.year  = 2020;
			//timeT0.month = 06;
			//timeT0.day   = 05;
			//timeT0.dotw  = 5;
			//timeT0.sec   = 0;
			//timeT0.min   = 0;
			//timeT0.hour  = 0;
			//rtc_set_datetime(&timeT0); 
			t0 = time_us_32();   
		}
	} stats;
		#define AUDIOREC_STATS(X_,...) stats.X_ (__VA_ARGS__); 
	#else
		#define AUDIOREC_STATS(X_,...)
	#endif

	AudioRec()
	{
		recordingCallbackFx = recordingCallbackFxDefault;
		callbackFx			= 0;
		silence				= 0;
		fastWrite			= 0;
		target				= audio::AUDIOREC_TARGET_FILE;
		PIN_ADC_LEFT		= -1;
		PIN_ADC_RIGHT		= -1; 
		SAMPLE_DMA_CHANNEL	= 2;
		diskBuffer			= 0;
		dmaBuffer			= 0;
		currFilename[0]     = 0;
	}

	W0 adc_init_()
	{
		// ADC poem na queue um sample de 4bytes a 44.1KHz
		// ADC emite DREQ a DMA2
		// DMA copia todos os samples na queue (que vai ser só 1)
		// ADC shifta 1 byte (pelo que só fica 1) esta função é especifica para DMAs de 1byte
		adc_run(false);
		adc_gpio_init(PIN_ADC_LEFT);
		if (AUDIO_CHANS==2)	 adc_gpio_init(PIN_ADC_RIGHT);
		adc_init();
		adc_set_temp_sensor_enabled(false);
		adc_select_input(0) ;
		adc_set_round_robin(AUDIO_CHANS);	//<<- aqui se especifica quantos adc
		adc_fifo_setup(
			true,    // Write each completed conversion to the sample FIFO
			true,    // Enable DMA request (DREQ)
			1,       // DREQ (and IRQ) asserted when at least 1 sample present
			false,   // We won't see the ERR bit because of 8 bit reads; disable.
			false	 // shift the sample 8 bits (12->20bits)
			);

		// ao DMA se dá a frequencia do SOM (na forma de divisor)
		// as amostras são postas a esta frequencia
		const W32 divisor = RASP_PICO_ADC_BASE_FREQ_HZ/(AUDIO_SAMPLE_RATE); 
		adc_set_clkdiv(divisor);
		DEBUG_AUDIOREC("ADC clkdiv: %i", divisor)

		// ADC+DMA /////////////////////////////////////////////////////////
		// configuração canal 2 amostragens
		////////////////////////////////////////////////////////////////////
		dma_channel_config  dma_cfg;
		dma_cfg = dma_channel_get_default_config(SAMPLE_DMA_CHANNEL);
		uint32_t freq = clock_get_hz(clk_sys);

		channel_config_set_read_increment(&dma_cfg, false);  // no read incrementing
		channel_config_set_write_increment(&dma_cfg, true);  // yes write incrementing
		channel_config_set_dreq(&dma_cfg, DREQ_ADC);         // paced by DREQ_ADC
		channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);

		
		dma_channel_configure(SAMPLE_DMA_CHANNEL, 
							  &dma_cfg,
							  dmaBuffer,			// destination buffer
							  &adc_hw->fifo,		// ADC data register
							  AUDIO_NUM_SAMPLES,	// transfer count
							  false);				// start immediately
	

		dma_channel_set_irq0_enabled(SAMPLE_DMA_CHANNEL, true);
		if (AUDIO_SAMPLE_BITS==8)
			irq_set_exclusive_handler(DMA_IRQ_0, core0_process_dmaSamples_<W8,8>);
		else
			irq_set_exclusive_handler(DMA_IRQ_0, core0_process_dmaSamples_<SW16,16>);
		irq_set_enabled(DMA_IRQ_0, true);
	}
	
// começar/interromper gravação com botão
#ifdef AUDIOREC_PIN_START_STOP_BUTTON
	W0 core0_button_irqhandler(uint GPIO_, uint32_t EVENTMASK_)
	{
		if (GPIO_==AUDIOREC_PIN_START_STOP_BUTTON) //???
		{
			DEBUG_AUDIOREC("button",0)
			recording= !recording;
		}
		PIN_IRQ_HANDLER_ACK
	}
	static  W0 core0_button_irqhandler_(uint GPIO_, uint32_t EVENTMASK_)
	{
		audio::audioRecInstance->core0_button_irqhandler(GPIO_, EVENTMASK_);
	}
#endif

// core1: gravar para ficheiro (normal ou escrita rápida) ou transmitir por esp8266

	#define AUDIOREC_WAIT_READY_TO_WRITE while(!readyToWrite){}
	#define AUDIOREC_SET_WRITE_DONE     readyToWrite = false;
	#define AUDIOREC_SET_WRITE_READY    readyToWrite = true;
		
	#ifdef APP_USES_WIFI
	W0 core1_espTransmit()
	{
		ESP_PacketVoice pack;	
		while (recording)
		{
			AUDIOREC_WAIT_READY_TO_WRITE //*** lock
			pack.size = AUDIO_NUM_SAMPLES*(AUDIO_SAMPLE_BITS/8);
			esp_send((W0*)&pack, sizeof(ESP_PacketVoice));
			esp_send((W0*)diskBuffer, pack.size);
			AUDIOREC_SET_WRITE_DONE //*** lock
		}
	}
	#endif
	W0 core1_fastDiskSave(W1 once_=false)
	{
			DEBUG_AUDIOREC("c1: start",0)
			while (recording)
			{
				AUDIOREC_STATS(core1_wait_begin)
				DEBUG_AUDIOREC("c1: wait",0)
				AUDIOREC_WAIT_READY_TO_WRITE  //*** lock
				AUDIOREC_STATS(core1_wait_end)
				DEBUG_AUDIOREC("c1: write",0)
				DEBUG_AUDIOREC("%X/%X", fastWrite->currCluster, fastWrite->lastCluster);
				if (fs_fastWrite((W8*)diskBuffer, fastWrite->currCluster, fastWrite->lastCluster)!=RES_OK)
					goto cantwritemore;
				writtenBlocks++;
				AUDIOREC_STATS(core1_write_end) 
				AUDIOREC_SET_WRITE_DONE //*** lock
				if (once_) return;
			}
			cantwritemore:
				DEBUG_AUDIOREC("c1: can't wr more...",0);
			end: 
				AUDIOREC_SET_WRITE_DONE
				AUDIOREC_STATS(core1_write_end)
	}
	W0 core1_diskSave(W1 once_=false)
	{ 
		while (recording)
		{
			AUDIOREC_WAIT_READY_TO_WRITE //*** lock
			W32 bytesWr;
			if (fs_rw_(&fileNode,(W8*)diskBuffer, bytesToWrite,  bytesWr, FS_OP_WRITE, AUDIOREC_PREALOC_CLUSTERS)!=RES_OK)
				goto cantwritemore;
			writtenBlocks++;
			AUDIOREC_SET_WRITE_DONE //*** lock
		}
		cantwritemore:
		end: 
			AUDIOREC_SET_WRITE_DONE //*** lock

	}
	static W0 core1_saveTask_()
	{
		stdio_init_all();
		switch(audioRecInstance->target)
		{
			case audio::AUDIOREC_TARGET_FILE:
				if (audioRecInstance->fastWrite)
					audioRecInstance->core1_fastDiskSave();
				else
					audioRecInstance->core1_diskSave();
			break;
			#ifdef APP_USES_WIFI
			case audio::AUDIOREC_TARGET_ESP:
					audioRecInstance->core1_espTransmit();
			break;
			#endif
		}
	}

// core0: ao completar o buffer por DMA
	inline W1 core0_process_dmaSamples_sampleSilence()
	{
		// fazer o valor minimo e máximo
		W16 vmin = 1<<12, vmax = 0;
		for (SW32 i=0; i < AUDIO_NUM_SAMPLES; i++) 
		{
			const W16 val = dmaBuffer[i]&0xFFF;
			vmin = VL_MIN(vmin, val);
			vmax = VL_MAX(vmax, val);
		}
			
		// se é para medir apenas preencher a estrutura e sair
		if (target == audio::AUDIOREC_TARGET_MEASURE)
		{
			silence->measure.minTh = VL_MIN(silence->measure.minTh, vmin);
			silence->measure.maxTh = VL_MAX(silence->measure.maxTh, vmax);
			//printf("\nm:%i M:%i",silence->minTh, silence->maxTh);

			silence->counter += AUDIO_NUM_SAMPLES;
			if (silence->counter >= silence->measure.maxSamples)
				return 1;
			return 0;
		}

		//printf("\nm:%i M:%i",vmin,vmax);

		// se for para verificar se entrou em silencio
		if ((vmin>=silence->measure.minTh) && (vmax<=silence->measure.maxTh))
		{
			silence->counter += AUDIO_NUM_SAMPLES;
			if (silence->counter >= silence->maxSamplesOnSilenceToStop)
				return 1;
		} 
		else
		{
			// repor o contador se foi fora do intervalo
			silence->counter = 0;
		}

		return 0;
	}


	#ifdef AUDIOREC_TEST_IMMEDIATE_SAVING
		W32 maxSamplesBocks;
	#endif

	template<typename DT, int BITS_>
	W0 core0_process_dmaSamples()
	{
		volatile DT *b = (DT*)diskBuffer;
		DEBUG_AUDIOREC("samp",0)
		goto end;
		//if ((target != audio::AUDIOREC_TARGET_MEASURE) && readyToWrite)
		//{
		//	sampleBlocks++;
		//	dma_channel_set_write_addr(SAMPLE_DMA_CHANNEL, dmaBuffer, true);
		//	dma_hw->ints0 = 1u << SAMPLE_DMA_CHANNEL;
		//	return;
		//}

		// retirar do buffer DMA apenas a amostragem
		// (ignorar os MSB, e escalar os restantes) 
		// atencao a tirar os ultimos bits do adc com FFF
		
		if (silence)
		{
			// 
			if (core0_process_dmaSamples_sampleSilence())
				goto terminate;
		} 

		#ifdef AUDIOREC_USE_TRIANGLE_TEST
			for (SW32 i=0; i < AUDIO_NUM_SAMPLES; i++) 
				dmaBuffer[i] = ((FP32)i/(AUDIO_NUM_SAMPLES-1))*4095; //12bit
		#endif

		if (BITS_==8)
		{
			for (SW32 i=0; i < AUDIO_NUM_SAMPLES; i++) 
				b[i] = AUDIO_ENCODE_8(dmaBuffer[i]);
		}
		else
		{
			for (SW32 i=0; i < AUDIO_NUM_SAMPLES; i++) 
				b[i] = AUDIO_ENCODE_16(dmaBuffer[i]);
		}

		dma_channel_set_write_addr(SAMPLE_DMA_CHANNEL, dmaBuffer, true);
		AUDIOREC_STATS(core0_dma_update)
		sampleBlocks++;

		AUDIOREC_SET_WRITE_READY

		#ifdef AUDIOREC_TEST_IMMEDIATE_SAVING
			DEBUG_AUDIOREC("\n%i/%i", sampleBlocks, maxSamplesBocks)
			core1_fastDiskSave(true);
			if (sampleBlocks == maxSamplesBocks)
				goto terminate;
		#endif
		dma_hw->ints0 = 1u << SAMPLE_DMA_CHANNEL;

		return;

		terminate:
			AUDIOREC_SET_WRITE_DONE
			adc_run(false);
			AUDIOREC_LOG("silence detected",0);
			DELAY_MS(400);
			recording = false;
		end:
			dma_hw->ints0 = 1u << SAMPLE_DMA_CHANNEL;
	}

	template<typename DT, int BITS_>
	static W0 core0_process_dmaSamples_()
	{
		audio::audioRecInstance->core0_process_dmaSamples<DT,BITS_>();
	}

	W0 core0_stop_()
	{
		DEBUG_AUDIOREC("c0: finishing...",0)
	
		adc_run(false);
		irq_set_enabled(DMA_IRQ_0, false);

		if (recordingCallbackFx)
			recordingCallbackFx(AUDIOREC_CAUSE_STOP, recordingCallbackFxArgs);

		// aguardar que core1 acabe de escrever
		while(readyToWrite){}
		DELAY_MS(200);
		multicore_reset_core1();
	
		AUDIOREC_STATS(core0_finish, sampleBlocks, writtenBlocks, AUDIO_SAMPLE_RATE, AUDIO_NUM_SAMPLES)

		if (target == audio::AUDIOREC_TARGET_FILE)
		{ 
			// como houve prealocação de blocos, é preciso zerar os buffers até
			// prefazer o cluster
			if (fastWrite)
			{
				if (fs_fastWriteEnd(&fileNode, fastWrite->beginCluster, fastWrite->currCluster)!=FS_OK)
				{	
					DEBUG_AUDIOREC("cannot close fast wr!",0)
				}
			}
			// escrever o header do ficheiro
			// definindo o tamanho total
			WAVEFORM_ waveHeader;
			W32 bytesWr;
			const W32 waveSize = ((fastWrite)?(fastWrite->bytesExtra):0) + writtenBlocks * 
								 AUDIO_NUM_SAMPLES*((AUDIO_SAMPLE_BITS==8)?1:2);
			waveHeader.fill(waveSize , AUDIO_SAMPLE_RATE, AUDIO_SAMPLE_BITS, AUDIO_CHANS);
 
			if (fs_seek(&fileNode, 0)!=FS_OK)
			{
				DEBUG_AUDIOREC("cant seek!",0)
			}
			if (fs_write(&fileNode, &waveHeader, sizeof(WAVEFORM_), bytesWr)!=FS_OK)
			{
				DEBUG_AUDIOREC("cant wr hd!",0)
			}
			if (fs_close(&fileNode)!=FS_OK)
			{
				DEBUG_AUDIOREC("cant close!",0)
			}
			DEBUG_AUDIOREC("file total:%i", waveSize); 
		}
		else
		{
			#ifdef APP_USES_WIFI
				ESP_PacketVoice pack;	
				pack.size = 0;
				esp_send(&pack, sizeof(ESP_PacketVoice));
			#endif
		}
		 
		DEBUG_AUDIOREC("c0: end",0)
		sound_alert(SOUND_BEEP); 
	}
	W1 core0_readyStream_()
	{
		switch(target)
		{ 
			case audio::AUDIOREC_TARGET_FILE:
			{ 
				W32 bytesWr;
				
				DEBUG_AUDIOREC("file... fstwrite?%i",fastWrite?1:0)
				if (currFilename[0]==0)
				{
					// procura um seguinte novo ficheiro com o mesmo nome 
					// mas numero mais avançado
					if (fs_createFileNodeExt(currFilename, "/sound%i.wav", &fileNode)!=RES_OK)
						goto file_error;
				}
				else
				{
					fs_delete(currFilename);
					//								  partição  abrir existente
					if (fs_createFileNode(currFilename, &fileNode, 0, false)!=RES_OK)
						goto file_error;
				}
				DEBUG_AUDIOREC("start for %s", currFilename)

				// gravar espaço temporario para header
				if (fs_write(&fileNode, &currFilename, sizeof(WAVEFORM_), bytesWr) !=RES_OK)
				{
					DEBUG_AUDIOREC("cannot seek", 0)
					return 1;
				}
				if (fastWrite)
				{
					if (fs_fastWriteBegin(&fileNode, fastWrite->beginCluster, fastWrite->currCluster, fastWrite->lastCluster, fastWrite->bytesExtra)!=FS_OK)
					{
						DEBUG_AUDIOREC("cannot fast write begin!", 0)
						return 1;
					}
				}
				DEBUG_AUDIOREC("file created",0)
			}
			break;
			case audio::AUDIOREC_TARGET_ESP:
			{
				#ifdef APP_USES_WIFI
					ESP_PacketHeader packetheader;
					packetheader.type = ESP_PacketType_Voice;
					esp_send(&packetheader, sizeof(ESP_PacketHeader));
		
					ESP_PacketVoiceHeader packetVoiceHeader;
					packetVoiceHeader.channels = AUDIO_CHANS;
					packetVoiceHeader.sampleRate = AUDIO_SAMPLE_RATE;
					packetVoiceHeader.bitsPerSample = AUDIO_SAMPLE_BITS; 
					esp_send(&packetVoiceHeader, sizeof(ESP_PacketVoiceHeader));
				#endif	
			}
			break;	
		}
		return 0;
	file_error:
		DEBUG_AUDIOREC("'%s' fail to create", currFilename)
		return 1;
	}
	W1 core0_start_(W1 waitForButton_, W1 multipleFiles_=false)
	{  
		#ifdef AUDIOREC_USES_BLINK
			W32 lastSampleBlock=0, N=0;
			W1 ledValue = true;
		#endif

		AUDIOREC_SET_WRITE_DONE
		recording	    = false;
		writtenBlocks   = 0;
		sampleBlocks	= 0;

		adc_init_();
		if  (waitForButton_)
		{
			if (recordingCallbackFx)
				recordingCallbackFx(AUDIOREC_CAUSE_WAIT, recordingCallbackFxArgs);
		
			while (!recording)
			{
				tight_loop_contents(); 
			}
		}
		
		recording = true; 
		
		
		#ifdef AUDIOREC_PIN_START_STOP_BUTTON
			PIN_UNINSTALL_IRQ_ONFALL(AUDIOREC_PIN_START_STOP_BUTTON)
		#endif		

		// core1 (gravar) começa mas espera por "readyToWrite" 
		if (target == audio::AUDIOREC_TARGET_MEASURE)
		{
			DEBUG_AUDIOREC("start sampling",0)
		}
		else
		{
			#ifdef AUDIOREC_TEST_IMMEDIATE_SAVING
				printf("\nimm saving");
				maxSamplesBocks = 8192;
			#else
				DELAY_MS(200);
				multicore_reset_core1();
				multicore_launch_core1(&core1_saveTask_);
			#endif
		}

		DEBUG_AUDIOREC("start rec",0)
		#ifdef AUDIOREC_PIN_START_STOP_BUTTON
			PIN_SET_PULLUP(AUDIOREC_PIN_START_STOP_BUTTON)
			PIN_INSTALL_IRQ_ONFALL(AUDIOREC_PIN_START_STOP_BUTTON, core0_button_irqhandler_)
		#endif

		// abrir ficheiro ou ESP
		if (core0_readyStream_()) return 1;
		
		if (recordingCallbackFx)
			recordingCallbackFx(AUDIOREC_CAUSE_START, recordingCallbackFxArgs);

		// iniciar as interrupções do ADC (core0)
		AUDIOREC_STATS(reset)
		adc_run(true);
		DELAY_MS(50);
		dma_channel_start(SAMPLE_DMA_CHANNEL); 
	
		// aguardar um pouco pelo o ADC
		DELAY_MS(50);
		DEBUG_AUDIOREC("c0: start",0)
		
		while(recording)
		{
			tight_loop_contents(); 
			#ifdef AUDIOREC_USES_BLINK
				if (lastSampleBlock!=sampleBlocks)
				{
					N++;
					lastSampleBlock=sampleBlocks;
				}

				if (N>4)
				{
					N=0;
					//if (callbackFx)
					//	callbackFx(callbackFxArgs);
					#ifdef AUDIOREC_USES_BLINK
						PIN_WRITE(PIN_LED, ledValue);
						ledValue = !ledValue;
					#endif
					//DELAY_MS(10)
				}
			#endif
		}
		core0_stop_();

		//if (multipleFiles_) goto repeat;
		return 0;
	}

	W0 recordToNextFile(W1 waitForButton_=false)
	{ 
		target = audio::AUDIOREC_TARGET_FILE;
		currFilename[0]=0;
		core0_start_(waitForButton_, true);
	}
	W0 recordToFile(const SW8 *filename_, W1 waitForButton_=false)
	{
		target = audio::AUDIOREC_TARGET_FILE;
		strcpy(currFilename, filename_);
		core0_start_(waitForButton_);
	}
	W0 adjustSilenceThresholds(const W32 seconds_, SW16 addOffset_ = 50 )
	{
		silence->measure.minTh = ((1<<12))-1;
		silence->measure.maxTh = 0;
		silence->measure.minTh -=addOffset_;
		silence->measure.maxTh +=addOffset_;

		silence->counter = 0;
		silence->measure.maxSamples = seconds_*AUDIO_SAMPLE_RATE;
		target = audio::AUDIOREC_TARGET_MEASURE;
		core0_start_(false);
		silence->print();
	}
	W0 espTransmit(W1 waitForButton_=false)
	{
		target = audio::AUDIOREC_TARGET_ESP;
		core0_start_(waitForButton_);
	}
	
	W1 init(W1 fastDiskWrite_, 
			W16 AUDIO_SAMPLE_BITS_=16, 
			W16 AUDIO_SAMPLE_RATE_=8000, 
			W8 PIN_ADC_LEFT_=0xFF, 
			W8 PIN_ADC_RIGHT_=0xFF, 
			SW8 stopOnSilenceSecs_=-1,
			W16 silenceThMin_ = AUDIOREC_DEFAULT_SIL_MIN_TH,
			W16 silenceThMax_ = AUDIOREC_DEFAULT_SIL_MAX_TH,
			W16 LBA_CLUSTERS_=4)
	{
		DEBUG_AUDIOREC("init..",0)

		if (fastDiskWrite_)
			fastWrite = new FastWrite();

		audio::audioRecInstance = this;
		
		PIN_ADC_LEFT      = PIN_ADC_LEFT_;
		PIN_ADC_RIGHT     = PIN_ADC_RIGHT_;
		AUDIO_CHANS       = (PIN_ADC_RIGHT==0xFF)?1:2;
		AUDIO_SAMPLE_BITS = AUDIO_SAMPLE_BITS_;
		AUDIO_SAMPLE_RATE = AUDIO_SAMPLE_RATE_;
		
		if (stopOnSilenceSecs_!=-1)
		{
			silence = new Silence();
			silence->counter= 0;
			silence->maxSamplesOnSilenceToStart = stopOnSilenceSecs_*AUDIO_SAMPLE_RATE;
			silence->measure.minTh = silenceThMin_;
			silence->measure.maxTh = silenceThMax_;
			//AUDIOREC_LOG("Samples to silence: %i", silence->maxSamplesOnSilence ) 
		}
		 
		bytesToWrite = 512*(W32)LBA_CLUSTERS_; 
		AUDIO_NUM_SAMPLES = (bytesToWrite/(AUDIO_SAMPLE_BITS/8))/AUDIO_CHANS;
	
		AUDIOREC_LOG("Fst?%1i Clusters:%u Samples:%u Bytes:%u", fastWrite?1:0, LBA_CLUSTERS_, AUDIO_NUM_SAMPLES, bytesToWrite) 

		#ifdef AUDIOREC_USES_EXTERNAL_BUFFER
			if (bytesToWrite == AUDIOREC_USES_EXTERNAL_BUFFERSZ)
			{
				AUDIOREC_LOG("\nbuf-ext",0); 
				diskBuffer = (volatile W8*)AUDIOREC_USES_EXTERNAL_BUFFERSZ;
			}
			else
			{
				AUDIOREC_LOG("\nbuf-new",0); 
				diskBuffer = new volatile W8[bytesToWrite]; 
			}
		#else
			diskBuffer = new volatile W8[bytesToWrite]; 
		#endif
		dmaBuffer  = new volatile W16[AUDIO_NUM_SAMPLES];

		if (!(diskBuffer && dmaBuffer))
		{
			if (diskBuffer) delete diskBuffer;
			if (dmaBuffer) delete dmaBuffer;
			DEBUG_AUDIOREC("alloc!", 0)
			return 1;
		}
	 
		////////////////////////////////////////////////////////////////////
	
		////////////////////////////////////////////////////////////////////
		#ifdef AUDIOREC_PIN_START_STOP_BUTTON
			PIN_UNINSTALL_IRQ_ONFALL(AUDIOREC_PIN_START_STOP_BUTTON)
			PIN_INIT(AUDIOREC_PIN_START_STOP_BUTTON)
			PIN_MODE(AUDIOREC_PIN_START_STOP_BUTTON,   PIN_MODE_INPUT)
			PIN_SET_PULLUP(AUDIOREC_PIN_START_STOP_BUTTON)
			PIN_INSTALL_IRQ_ONFALL(AUDIOREC_PIN_START_STOP_BUTTON, core0_button_irqhandler_)
		#endif
		return 0;
	}
	
	~AudioRec()
	{

		if (silence)
			delete silence;
		if (fastWrite)
			delete fastWrite;

		if (diskBuffer)
		{
			#ifdef AUDIOREC_USES_EXTERNAL_BUFFER
				if (bytesToWrite != AUDIOREC_USES_EXTERNAL_BUFFERSZ)
					delete[] (W8*)diskBuffer;
			#else
				delete[] (W8*)diskBuffer;
			#endif

		}
		if (dmaBuffer)
			delete[] dmaBuffer;
	}

};

};

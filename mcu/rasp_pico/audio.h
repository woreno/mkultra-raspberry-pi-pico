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
 
////////////////////////////////////////////////////////////////////////////////
// debug
#define DEBUG_AUDIOREC_AUDIO_TX					 1
#define DEBUG_AUDIOREC_INTERFACE				 2
#define DEBUG_AUDIOREC_ADC_MEASURE				 3
#define DEBUG_AUDIOREC_ADC_MEASURE_GRAPH		 4
#define DEBUG_AUDIOREC_WRITEDISK_SPEED			 5
#define DEBUG_AUDIOREC_FAST_WRITEDISK_SPEED		 6
	//#define DEBUG_AUDIOREC		DEBUG_AUDIOREC_ADC_MEASURE_GRAPH	 

#define DEBUG_LOG(X_)	
//#define DEBUG_LOG(X_) {  printf(X_);}

namespace audio {

class AudioRec;
class AudioPlayer;
AudioRec		*audioRecInstance;
AudioPlayer		*audioPlayerInstance;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

	enum		AUDIOREC_TARGET { AUDIOREC_TARGET_FILE, AUDIOREC_TARGET_ESP };
	#define		AUDIO_ENCODE(X_)		(((SW16)(X_)-2048)*8)
	
	enum STATE   { STATE_IDLE, STATE_BUSY, STATE_RECORDING, STATE_STOPPING };
	enum COMMAND { COMMAND_BEGIN_APP       = 0,
				   COMMAND_START_RECORDING = 1, 
				   COMMAND_BEEP			   = 2, 
				   COMMAND_STOP_RECORDING  = 3, 
				   COMMAND_NONE };
class AudioRec
{
public:
	// definições do audio
	// 12-bit unsigned para 16-bit signed
	//#define AUDIO_ENCODE(X_)		(((SW16)( ((W16)(X_))<<3 ))-32767)
	W16 PIN_ADC_LEFT;
	W16 PIN_ADC_RIGHT;
	W16 SAMPLE_DMA_CHANNEL;
	W16 AUDIO_CHANS;
	W16 AUDIO_SAMPLE_BITS;
	W16 AUDIO_SAMPLE_RATE;
	W16 LBA_CLUSTERS;
	W16 AUDIO_NUM_SAMPLES;
	SW8 currFilename[32]; 
	AUDIOREC_TARGET	target;
	
	volatile COMMAND	command;
	volatile STATE		state;

	dma_channel_config  dma_cfg;
	volatile SW16		*buffer; 
	volatile W16		*DMAbuffer;
	MutexType			recordMutex;
	MutexType			cmdMutex;
	FSNode				fileNode;
	WAVEFORM_			waveHeader;
	W32					bytesWr;
	W32 				t0;
	FP    				idleTimeSum;
	FP    				recordingTimeSum;
	FP    				samplingTimeSum;
	W32					sampleBlocks;
	W32					fastWriteBeginCluster, 
						fastWriteCurrCluster, 
						fastWriteLastCluster, 
						fastWriteBytesExtra;
	W32					writtenBlocks;
	
	AudioRec()
	{
		target				= audio::AUDIOREC_TARGET_FILE;
		PIN_ADC_LEFT		= IO_PIN_ADC0;
		PIN_ADC_RIGHT		= IO_PIN_ADC1; 
		SAMPLE_DMA_CHANNEL	= 2;
		LBA_CLUSTERS		= 4;
		buffer				= 0;
		DMAbuffer			= 0;
		currFilename[0]     = 0;
	}


W0 alert_(COMMAND cmd_)
{
	sound_emit(cmd_);
	blink(5,100);
}
W0 log_time_()
{
	const float sampleBlkSpeed = (W32)(samplingTimeSum/(float)sampleBlocks);
	const W32 sampleSpeed      = sampleBlkSpeed/AUDIO_NUM_SAMPLES;
	const W32 freq             = 1e6/(float)sampleSpeed;
	const W32 writeSpeed       = (W32)(recordingTimeSum/(float)sampleBlocks);
	const W32 idleTime         = (W32)(idleTimeSum/(float)sampleBlocks);
	const W32 totalTime        = (W32)(((float)sampleBlocks * AUDIO_NUM_SAMPLES)/(float)(AUDIO_SAMPLE_RATE));


	datetime_t  timeT1;
	rtc_get_datetime(&timeT1);
	
	time_t time_difference = mktime(&timeT1) - mktime(&timeT0);
    struct tm time_difference_tm;
    gmtime_r(&time_difference, &time_difference_tm);


	printf("\n time: %uh %umin %usec",
			(W32)tm.hour , (W32)tm.min, (W32)tm.sec);
	printf("\n computed: %i sec", totalTime);
	
	printf("\n %i blks each with %i samples / %i blks written",	//2
			sampleBlocks, AUDIO_NUM_SAMPLES, writtenBlocks); //2
	
	#ifdef DEBUG_AUDIOREC_ENABLE_STATS
		printf("\n %i usec/block"	//3
			   "\n %i usec/sample (freq: %iHz)" //4
			   "\n write:%i usec/block"	//5
			   "\n idle:%i usec/block", //6
				(W32)sampleBlkSpeed,   //3
				sampleSpeed,  freq,    //4
				writeSpeed,            //5
				idleTime);			   //6

	#endif
}
W0 log_reset_()
{
	idleTimeSum      = 0;
	recordingTimeSum = 0;
	samplingTimeSum  = 0;
	sampleBlocks     = 0;
	writtenBlocks    = 0;
	
	datetime_t timeT0;
    timeT0.year  = 2020;
    timeT0.month = 06;
    timeT0.day   = 05;
    timeT0.dotw  = 5;
	timeT0.sec   = 0;
	timeT0.min   = 0;
	timeT0.hour  = 0;
	rtc_init();
	rtc_set_datetime(&timeT0);
	t0 = time_us_32();  
}
W0 core1()
{ 
    stdio_init_all();
    while (1)
    {
		#ifdef DEBUG_AUDIOREC_ENABLE_STATS
			W32 core1_t0 = time_us_32();
		#endif
		
		DEBUG_LOG("\ncore1: waiting")
		const W32 option = multicore_fifo_pop_blocking();
		
		#ifdef DEBUG_AUDIOREC_ENABLE_STATS
			const W32 core1_t1 = time_us_32();
			idleTimeSum += core1_t1 - core1_t0;
		#endif

		DEBUG_LOG("\ncore1: writing buffer")
		//printf("%X/%X", fastWriteCurrCluster, fastWriteLastCluster);

		if (option==2)
		{
			printf("\ncore1: core0 mandou parar");
			MutexUnlock(recordMutex);
			return;
		}
		else
		{
			if (target  == audio::AUDIOREC_TARGET_FILE)
			{
				if (fs_fastWrite((SW8*)buffer, fastWriteCurrCluster, fastWriteLastCluster)!=RES_OK)
				{
					MutexLock(cmdMutex)
					printf("\ncore1: nao consigo escrever mais a parar...");
					command=COMMAND_STOP_RECORDING;
					MutexUnlock(cmdMutex)
					return;
				}
			}
			else
			{
				ESP_PacketVoice pack;	
				pack.size = AUDIO_NUM_SAMPLES*(AUDIO_SAMPLE_BITS/8);
				esp_send((W0*)&pack, sizeof(ESP_PacketVoice));
				esp_send((W0*)buffer, pack.size);
			}
		}
		writtenBlocks++;
		#ifdef DEBUG_AUDIOREC_ENABLE_STATS
			recordingTimeSum += time_us_32() - core1_t1;
		#endif
	}
}
static W0 core1_()
{
	audioRecInstance->core1_();
}
W0 dma_irqhandler()
{
	MutexLock(cmdMutex)
	if (command==COMMAND_STOP_RECORDING) 
	{
		 state = STATE_STOPPING;
	}
	else
	{
		// retirar do buffer DMA apenas a amostragem e escalá-la 
		// atencao a tirar os ultimos bits do adc com FFF
		volatile SW16 *b = buffer;
		for (SW32 i=0; i < AUDIO_NUM_SAMPLES; i++) 
			b[i] = AUDIO_ENCODE(DMAbuffer[i]&0xFFF);

		while (!multicore_fifo_wready()) {}
		multicore_fifo_push_blocking(1);
		dma_channel_set_write_addr(SAMPLE_DMA_CHANNEL, DMAbuffer, true);

		#ifdef DEBUG_AUDIOREC_ENABLE_STATS
			const W32 t1 = time_us_32();
			samplingTimeSum +=  t1 - t0;
			t0 = t1;
		#endif
		sampleBlocks++;
	}
	MutexUnlock(cmdMutex)
	dma_hw->ints0 = 1u << SAMPLE_DMA_CHANNEL;
}
static W0 dma_irqhandler_()
{
	audioRecInstance->dma_irqhandler();
}
W0 core0_button_irqhandler(uint GPIO_, uint32_t EVENTMASK_)
{
	MutexLock(cmdMutex)
	if (state  == STATE_IDLE)
	{
		state   = STATE_BUSY;
		command = COMMAND_START_RECORDING;
	}
	else if (state  == STATE_RECORDING)
	{
		state   = STATE_BUSY;
		command = COMMAND_STOP_RECORDING;
	}
	MutexUnlock(cmdMutex)

	#ifdef PIN_RECORD_BUTTON
		gpio_acknowledge_irq(PIN_RECORD_BUTTON, EVENTMASK_);
	#endif
}
static  W0 core0_button_irqhandler_(uint GPIO_, uint32_t EVENTMASK_)
{
	audio::audioRecInstance->core0_button_irqhandler(GPIO_, EVENTMASK_);
}
W0 core0_stop_()
{
	DEBUG_LOG("core0: terminando...")
	fflush(stdout);
	// aguardar que core1 acabe de escrever
	MutexCreate(recordMutex)
	MutexUnlock(recordMutex);
	MutexLock(recordMutex);
	multicore_fifo_push_blocking(2);
	MutexLock(recordMutex); 
	
	log_time_();
	
	if (target == audio::AUDIOREC_TARGET_FILE)
	{
		if (fs_fastWriteEnd(&fileNode, fastWriteBeginCluster, fastWriteCurrCluster)!=FS_OK)
		{
			printf("\nCannot fast write close!");
		}
		// escrever o header do ficheiro
		// definindo o tamanho total
		waveHeader.lDataSize  = fastWriteBytesExtra + writtenBlocks*AUDIO_NUM_SAMPLES*sizeof(SW16);
		fs_seek(&fileNode, 0);
		fs_write(&fileNode, &waveHeader, sizeof(WAVEFORM_), bytesWr);
	}
	else
	{
		ESP_PacketVoice pack;	
		pack.size = 0;
		esp_send(&pack, sizeof(ESP_PacketVoice));
	}

	alert_(COMMAND_STOP_RECORDING);
	DEBUG_LOG("core0: terminado")
	adc_run(false);

}
W1 core0_start_()
{ 
	adc_run(true);
	alert_(COMMAND_BEEP);  

	if (target  == audio::AUDIOREC_TARGET_FILE)
	{
		printf("\nnew file wait...");
		if (currFilename[0]==0)
		{
			if (fs_createFileNodeExt(currFilename, "/sound%i.wav", &fileNode)!=RES_OK)
			{
				printf("\nfs:file %s fail to create", currFilename);
				return 1;
			}
		}
		else
		{
			// <!> falta apagar o ficheiro!!
			if (fs_createFileNode(currFilename, &fileNode)!=RES_OK)
			{
				printf("\nfs:file %s fail to create", currFilename);
				return 1;
			}
		}
		printf("\nstart for %s", currFilename);
		fflush(stdout);

		if (fs_write(&fileNode, &waveHeader, sizeof(WAVEFORM_), bytesWr)==RES_FAIL)
		{
			printf("\ncannot write");
			return 1;
		}
		if (fs_fastWriteBegin(&fileNode, fastWriteBeginCluster, fastWriteCurrCluster, fastWriteLastCluster, fastWriteBytesExtra)!=FS_OK)
		{
			printf("\nCannot fast write begin!");
			return 1;
		}
		printf("\nfile created");
	}
	else
	{
		ESP_PacketHeader packetheader;
		packetheader.type = ESP_PacketType_Voice;
		esp_send(&packetheader, sizeof(ESP_PacketHeader));
		
		ESP_PacketVoiceHeader packetVoiceHeader;
		packetVoiceHeader.channels = AUDIO_CHANS;
		packetVoiceHeader.sampleRate = AUDIO_SAMPLE_RATE;
		packetVoiceHeader.bitsPerSample = AUDIO_SAMPLE_BITS; 
		esp_send(&packetVoiceHeader, sizeof(ESP_PacketVoiceHeader));
		
	}

	alert_(COMMAND_START_RECORDING); 
	state   = STATE_RECORDING;
	
	multicore_reset_core1();
	multicore_launch_core1(&core1_);

	// aguardar um pouco pelo o ADC e CORE1
	DELAY_MS(800);
	
	log_reset_();

	dma_channel_start(SAMPLE_DMA_CHANNEL); 
	while(state != STATE_STOPPING)
	{
		DELAY_US(100);
		if (sampleBlocks%32==0)
		{
			blink(1,10);
		}
	}
	core0_stop_();
	return 0;
}
//
W0 adc_dma_init_()
{
	// ADC poem na queue um sample de 4bytes a 44.1KHz
	// ADC emite DREQ a DMA2
	// DMA copia todos os samples na queue (que vai ser só 1)
	// ADC shifta 1 byte (pelo que só fica 1) esta função é especifica para DMAs de 1byte
	adc_gpio_init(PIN_ADC_LEFT);
	if (AUDIO_CHANS==2)	 adc_gpio_init(PIN_ADC_RIGHT);
	adc_init();
	adc_set_temp_sensor_enabled(false);
	adc_select_input(0) ;
	adc_set_round_robin(1+(AUDIO_CHANS-1)*2);	//<<- aqui se especifica quantos adc
	adc_fifo_setup(
		true,    // Write each completed conversion to the sample FIFO
		true,    // Enable DMA request (DREQ)
		1,       // DREQ (and IRQ) asserted when at least 1 sample present
		false,   // We won't see the ERR bit because of 8 bit reads; disable.
		false	 // shift the sample 8 bits (12->20bits)
		);
	adc_run(false);

	W32 divisor = 48000000.0f/(AUDIO_SAMPLE_RATE); 
	adc_set_clkdiv(divisor);
	printf("\nADC clkdiv: %i", divisor); 

 // configuração canal 2 amostragens
	dma_cfg = dma_channel_get_default_config(SAMPLE_DMA_CHANNEL);
    uint32_t freq = clock_get_hz(clk_sys);

	channel_config_set_read_increment(&dma_cfg, false);  // no read incrementing
	channel_config_set_write_increment(&dma_cfg, true);  // yes write incrementing
	channel_config_set_dreq(&dma_cfg, DREQ_ADC);         // paced by DREQ_ADC
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);

	dma_channel_configure(SAMPLE_DMA_CHANNEL, 
						  &dma_cfg,
						  DMAbuffer,			// destination buffer
						  &adc_hw->fifo,		// ADC data register
						  AUDIO_NUM_SAMPLES,	// transfer count
						  false);				// start immediately
	

	dma_channel_set_irq0_enabled(SAMPLE_DMA_CHANNEL, true);
	irq_set_exclusive_handler(DMA_IRQ_0, dma_irqhandler_);
	irq_set_enabled(DMA_IRQ_0, true);

}
W0 initWaveHeader_()
{
	FSNode pathNode;
	fs_getRootDirNode(&pathNode);
	printf("\nrtdir:");
	fs_iterateDir(&pathNode, fs_itfx_prnEntry, 0);
	waveHeader.fill(0, AUDIO_SAMPLE_RATE, AUDIO_SAMPLE_BITS, AUDIO_CHANS);
}
//
W0 init(W16 AUDIO_SAMPLE_BITS_=16, W16 AUDIO_SAMPLE_RATE_=44100/16, W8 AUDIO_CHANS_=1)
{
	audio::audioRecInstance = this;

	if (target == audio::AUDIOREC_TARGET_FILE)
		AUDIO_NUM_SAMPLES = (((LBA_CLUSTERS*512)/(sizeof(SW16)))/AUDIO_CHANS);
	else
		AUDIO_NUM_SAMPLES = 512;
	
	buffer    = new volatile SW16[AUDIO_NUM_SAMPLES]; 
	DMAbuffer = new volatile W16[AUDIO_NUM_SAMPLES];
	
	COMMAND command = COMMAND_NONE;
	STATE   state   = STATE_IDLE;

	adc_dma_init_();
	
	#ifdef PIN_RECORD_BUTTON
		PIN_INIT(PIN_RECORD_BUTTON)
		PIN_MODE(PIN_RECORD_BUTTON,   PIN_MODE_INPUT)
		PIN_SET_PULLUP(PIN_RECORD_BUTTON)
		PIN_INSTALL_IRQ_ONFALL(PIN_RECORD_BUTTON, core0_button_irqhandler_)
	#endif
		
	MutexUnlock(recordMutex)
	MutexUnlock(cmdMutex)
}
W0 record_(W1 waitForButton_)
{ 
	alert_(COMMAND_BEGIN_APP); 
	if  (!waitForButton_)
	{
		core0_start_();
		return;
	}
		
	while(1)
	{
		MutexUnlock(recordMutex)
		MutexUnlock(cmdMutex)
		command = COMMAND_NONE;
		state = STATE_IDLE;
		printf("\nWaiting\n");
		fflush(stdout);
		
		while (command == COMMAND_NONE){ DELAY_MS(10); }
		switch(command)
		{	
			case COMMAND_START_RECORDING:
				core0_start_(); 
			break;
		}  
	}
}
W0 recordToFiles()
{
	target = audio::AUDIOREC_TARGET_FILE;
	currFilename[0]=0;
	initWaveHeader_();
	record_(true);
}
W0 recordToFile(SW8 *filename_=0, W1 waitForButton_=false)
{
	target = audio::AUDIOREC_TARGET_FILE;
	if (filename_)
		strcpy(currFilename, filename_);
	else
		currFilename[0]=0;
	initWaveHeader_();
	record_(waitForButton_);
}
W0 espTransmit(W1 waitForButton_=false)
{
	target = audio::AUDIOREC_TARGET_ESP;
	record_(waitForButton_);
}
W0 tests()
{
	#ifdef DEBUG_APP
		test_adc_init();
		#if (DEBUG_AUDIOREC==DEBUG_INTERFACE)
			test_interface();
		#endif
		#if (DEBUG_AUDIOREC==DEBUG_AUDIO_TX)
			test_audio_tx();
		#endif	
		#if ((DEBUG_AUDIOREC==DEBUG_WRITEDISK_SPEED)||(DEBUG_APP==DEBUG_FAST_WRITEDISK_SPEED))
			test_measure_writedisk_speed();
		#endif
		#if (DEBUG_AUDIOREC==DEBUG_ADC_MEASURE)
			test_measure_adc_read();
		#endif
		#if (DEBUG_AUDIOREC==DEBUG_ADC_MEASURE_GRAPH)
			test_measure_adc_read_graph();
		#endif
			STOP 
	#endif 
}
W0 destroy()
{
	if (buffer)
		delete[] buffer;
	if (DMAbuffer)
		delete[] DMAbuffer;
}
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class AudioPlayer
{
	public:
		W32				audioPin;
		W0				*playBuffer;
		WAVEFORM_		waveHeader;
		FSNode			node;
		FS_FLAGS		res;
		W32				halfSamples, halfSamplesInBytes;
		W32             delay_us;
		volatile		W1 waitFirst;
		volatile		W1 waitNext;
		volatile		W1 threadActive;
	
	template<typename DT>
	W0 digital_core1_play()
	{
		//printf("\ncore1");
		DT* buffer = (DT*) playBuffer;
		W32    readAt = 0;
		while(waitFirst){}
		//printf("\ncore1 st");
		while(threadActive)
		{
			//const SW32 readAt = multicore_fifo_pop_blocking();
			//printf("\nplay %u", readAt);
			const SW32 end = readAt + halfSamples;
			for (SW32 i=readAt; i< end; i++)
			{ 
				if (sizeof(DT)==1)
				{
					const W8 val = (buffer[i]>127)?1:0; 
					PIN_WRITE(audioPin, val);
					PIN_WRITE(PIN_BUZZER, val);
				}
				else
				{
					const SW16 val = (buffer[i]>0)?1:0; 
					PIN_WRITE(audioPin, val);
					PIN_WRITE(PIN_BUZZER, val);
				}
				 
				DELAY_US(delay_us+audio_adj)
			} 
			if (readAt==0) 
			{
				waitNext = false;
				readAt = halfSamples;
			}
			else
				readAt = 0;
		}
		waitNext = false;
	} 
	
	template<typename DT>
	static W0 digital_core1_play_()
	{
		audio::audioPlayerInstance->digital_core1_play<DT>();
	}

	template<typename DT>
	W0 digital_core0_read_()
	{
		audioPlayerInstance = this;
		waitFirst = true;
		threadActive = true;
		multicore_reset_core1();
		multicore_launch_core1(&digital_core1_play_<DT>);

		DT* buffer = (DT*) playBuffer;
		W32 bytesRW;
		W32 fileSize = node.file.fileSize;
		W32 pos = 0;
		halfSamplesInBytes = halfSamples*sizeof(DT);
		while(fileSize)
		{
			DT* bf = buffer + pos;

			const W32 toReadInBytes   = MIN(fileSize, halfSamplesInBytes);
			const W32 toReadInSamples = toReadInBytes/sizeof(DT);

			res = fs_read(&node, bf, toReadInBytes, bytesRW);

			fileSize-=toReadInBytes;

			if (fileSize==0)
			{
				const W32 remainSize = halfSamplesInBytes - toReadInBytes;
				memset(bf + toReadInSamples, 0, remainSize);
			} 


			waitFirst = false;
			if (pos==0)
			{
				waitNext = true;
				while(waitNext){}
				pos = halfSamples;
			}
			else
			{
				pos=0;
			}
		}
		threadActive = false;
	}

	W1 digital_play(const SW8* filename_, const W32 audioPin_)
	{ 
		W32 samples, bytesRW;
		audioPin = audioPin_;
		PIN_INIT(audioPin)
		PIN_MODE(audioPin,	PIN_MODE_OUTPUT)
		PIN_INIT(PIN_BUZZER)
		PIN_MODE(PIN_BUZZER,PIN_MODE_OUTPUT)
		
		res = fs_createFileNode(filename_,&node,0,true);
		if (res!=FS_OK)
		{
			printf("\ncannot open file");
			return 1;
		}
		
		res = fs_read(&node, &waveHeader, sizeof(WAVEFORM_), bytesRW);
		if (res!=FS_OK)
		{
			printf("\ncannot read header");
			return 1;
		}

		printf("\nsample rate:% u bits:%u", waveHeader.wfex.nSamplesPerSec, waveHeader.wfex.wBitsPerSample);
		delay_us = ((1.0f/(FP32)waveHeader.wfex.nSamplesPerSec)*1e6)-55;
		printf("\ndelay em us: %u", delay_us);
		
		if (waveHeader.wfex.wBitsPerSample==8)
		{
			samples    =  4096; // 1~cluster
			playBuffer = new W8[samples];
		}
		else
		{
			samples    =  4096/2;
			playBuffer = new SW16[samples];
		}
		halfSamples = samples>>1;

		if (!playBuffer)
		{
			printf("\n cannot alloc %u samples", samples);
			return 1;
		}
		multicore_reset_core1();
		 
		if (waveHeader.wfex.wBitsPerSample==8)
		{
			digital_core0_read_<W8>();
			delete[] (W8*)playBuffer;
		}
		else
		{
			digital_core0_read_<SW16>();
			delete[] (SW16*)playBuffer;
		}
		
		return 0;
	}


	};


};


//#define PWM_USE_TAB
//#define PWM_USE_DY
#define AUDIOPLAYER_PWM_USES_IRQ	// melhor
#ifdef AUDIOPLAYER_PWM_USES_IRQ
	#define AUDIOPLAYER_PWM_OVERSAMPLE 2
	#define AUDIO_PLAYER_DEF_PWM_WRAP  255
#else
	#define AUDIOPLAYER_PWM_OVERSAMPLE 16
	#define AUDIO_PLAYER_DEF_PWM_WRAP  32
#endif

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

#define AUDIOPLAYER_BUFFERSZ	1024

#define AUDIOPLAYER_SWAPBUFFERS \
	volatile W0* temp = playBuffers[1]; \
	playBuffers[1] = playBuffers[0]; \
	playBuffers[0] = temp;

//std::swap(playBuffers[0], playBuffers[1]);
//#define DEBUG_AUDIOPLAYER_VERBOSE
#define DEBUG_AUDIOPLAYER(X_,...)       { printf("\n"); printf(X_,__VA_ARGS__);fflush(stdout);}
#define DEBUG_AUDIOPLAYER_ESP(X_,...)   // { printf("\n"); printf(X_,__VA_ARGS__); fflush(stdout);}
//#define AUDIOPLAYER_AUXPIN PIN_BUZZER 


namespace audio {

class AudioPlayer;
AudioPlayer		*audioPlayerInstance;
 

class AudioPlayer
{
	public:
		typedef W0(*COMPLETION_FUNCTION)(W32, W32);
		
		repeating_timer		timer;

		#ifdef APP_USES_SD
			FSNode			node;
			FS_FLAGS		res;
		#endif
		WAVEFORM_		waveHeader;
		W1				isFile;
		W8				audioPin;
		W16             delay_us;
		volatile		W1  waitFirst;
		volatile		W1  dataIsReady, core1finished; 
		//
		volatile		W32	totalSamples, currPosSample;
		//
		volatile		W0 *playBuffers[2]; 
		volatile        W16 availableSamples;
		volatile		W16	samplesInBytes, samples;
		volatile		W16 currSampleID;
						W8	currPlayBit;
		union
		{
			SW16			lastSampleValue16;
			W8				lastSampleValue8;
		};
		W8				pwmBits,  
						pwmRepeatCounter, 
						pwmFrameID, 
						pwmWaveON;
		W8				pwmVal;
		W1				pwmHaveRLE;
		W8              *pwmBuff;
		COMPLETION_FUNCTION completionFunction;
		W16				SAMPLE_VALUE_MIN;

		
// CORE 0: tocar temporizado PWM
	W0 core0_pwmPlay_timer()
	{
		// *** não fazer nada se chegou ao fim
		if (currPosSample>=totalSamples)
		{
			//pwm_set_gpio_level(audioPin, 0);
			dataIsReady = false;
			return;
		}
		if (!dataIsReady) return;

		if (pwmBits==0) // <-default
		{
			const W8* buffer = (const W8*) playBuffers[1];	
			const W8 val = buffer[currSampleID]>>4; 
			{
				// tornar val simetrico
				//const W8 newval = val; //((val>=128)? (val-128): (128-val))<<1;
				//if (val<128)
					pwm_set_gpio_level(audioPin, val);
				pwmVal = val;
			}
			++currPosSample;
			if (++currSampleID != availableSamples) return;
		}
		else
		{
			if (currSampleID==0) 
			{
				pwmBuff = (W8*)playBuffers[1];
				currPlayBit = 0;
				getBitsInit<W8>(pwmBuff,  lastSampleValue8,  currPlayBit);
			}
			const W8 val = getBitsDyn(&pwmBuff, lastSampleValue8, currPlayBit, pwmBits);	
			if (pwmVal!=val)
			{
				pwm_set_gpio_level(audioPin, val);
				pwmVal = val;
			}
			//printf(" %i/%i", currSampleID,  ((availableSamples*8)/pwmBits), val);
			++currPosSample;
			if (++currSampleID <=  (((availableSamples*8)/pwmBits)+1) ) return;
		}
		currSampleID = 0;		
		AUDIOPLAYER_SWAPBUFFERS	 
		dataIsReady = false;
	
	}
	W0 core0_pwmPlay_int();

// CORE 0: tocar temporizado DIGITAL
	template<int BITSZ_,typename DT>
	W0 core0_digitalPlay_timer()
	{
		// *** não fazer nada se chegou ao fim
		if (currPosSample>=totalSamples)
		{
			//pwm_set_gpio_level(audioPin, 0);
			dataIsReady = false;
			return;
		}
		//if (!dataIsReady) return;

		const DT* buffer = (const DT*) playBuffers[1];
				
		if (BITSZ_==1)
		{
			lastSampleValue8>>=1;
			const W1     val = lastSampleValue8&1;
			PIN_WRITE(audioPin, val);						
			#ifdef AUDIOPLAYER_AUXPIN
				PIN_WRITE(AUDIOPLAYER_AUXPIN, val);	
			#endif
			++currPosSample;	
			if (++currPlayBit==8)
			{
				currPlayBit=0;
				if (++currSampleID==samplesInBytes) 
				{
					currSampleID = 0;			
					dataIsReady = false;
				}
				lastSampleValue8 = buffer[currSampleID];
			}
			return;
		}

		const W1 val =  (sizeof(DT)==1)?
			(buffer[currSampleID]>127):
			(buffer[currSampleID]>-1);
			#ifdef DEBUG_AUDIOPLAYER_VERBOSE
				DEBUG_AUDIOPLAYER("%i/%i %03i %i", currPosSample, totalSamples, buffer[currSampleID], val);
			#endif
		PIN_WRITE(audioPin, val);	
		#ifdef AUDIOPLAYER_AUXPIN
			PIN_WRITE(AUDIOPLAYER_AUXPIN, val);	
		#endif				

		++currPosSample;	
		//printf("%i",val?1:0);
		if (++currSampleID==samples) 
		{
			//printf("\nnext");
			currSampleID = 0;			
			AUDIOPLAYER_SWAPBUFFERS	 
			dataIsReady = false;
		}
	}
// CORE 1: ler ESP
	W0 core1_espRead()
	{
		#ifdef APP_USES_WIFI
		
		ESP_PacketVoice voicePacket;
		W32 it=0, size;
		RXTX_DUMMY(AUDIO_CHUNK_START_AUDIODETAILS)
		
		// receber pacote com o tamanho
		esp_receive(&voicePacket, sizeof(ESP_PacketVoice));
		totalSamples = voicePacket.size;
		size = totalSamples;
		RXTX_DUMMY(AUDIO_CHUNK_START_SENDING)

		while(size)
		{
			W8* bf = (W8*) playBuffers[0];
			
			availableSamples = VL_MIN( size, samples );
			 
			DEBUG_AUDIOPLAYER_ESP("\nESP: %i, %i %i/%i", it, availableSamples, voicePacket.size-size  , voicePacket.size  );
			it++;
			 
			RXTX_DUMMY(SEGMENT)
			if (esp_receive(bf, availableSamples)) goto error;

			//if (size < samples)
			//{
			//	if (pwmBits)
			//		memset(bf+size, 0, samples-size);
			//	else
			//		memset(bf+size, 128, samples-size);
			//}

			size-=availableSamples;
			waitFirst = false;
			dataIsReady = true;
			if (completionFunction)
				completionFunction(voicePacket.size-size, voicePacket.size);
			while(dataIsReady)
			{
				tight_loop_contents(); 
			}
		}
		goto end;
		error:
			DEBUG_AUDIOPLAYER_ESP("\nerror rd", 0);
		end:
			if (completionFunction)
				completionFunction(voicePacket.size, voicePacket.size);
			DEBUG_AUDIOPLAYER_ESP("\nfinish rd", 0);
			waitFirst = false;
			dataIsReady = true;
			core1finished = true;
		#endif

	} 
// CORE 1: ler FS
	template<int BITSZ_, typename DT>
	W0 core1_fileRead()
	{
		#ifdef APP_USES_SD
		W32 bytesRW;
		W32 totalRead, fileSize = node.file.fileSize;
		while(fileSize)
		{
			DT* bf = (DT*) playBuffers[0];

			const W32 toReadInBytes   = VL_MIN(fileSize, samplesInBytes);
			totalRead+=toReadInBytes;

			res =  fs_rw_(&node,  (W8*)bf, toReadInBytes, bytesRW, FS_OP_READ);
			fileSize -= toReadInBytes;

			//if (toReadInBytes<samplesInBytes)
			//{
			//	memset(bf + toReadInBytes, 0, samplesInBytes - toReadInBytes);
			//}
			#ifdef DEBUG_AUDIOPLAYER_VERBOSE
				printf("\nrd %i/%i", totalRead,  node.file.fileSize);
			#endif
			if (fileSize==0)
			{
				const W32 remainSize = samplesInBytes - toReadInBytes;
				memset((W8*)bf + toReadInBytes, 0, remainSize);
				break;
			} 
			
			availableSamples = toReadInBytes/sizeof(DT);
			waitFirst = false;
			dataIsReady = true;
			if (completionFunction)
				completionFunction(node.file.fileSize-fileSize, node.file.fileSize);
			while(dataIsReady){}
		} 
		if (completionFunction) completionFunction(node.file.fileSize, node.file.fileSize);
		DEBUG_AUDIOPLAYER("\nfinish rd", 0);
		waitFirst = false;
		dataIsReady = true;
		core1finished = true;
		#endif 
	} 

// trampolins ////////////////////////////////////////
	template<int BITSZ_,typename DT>
	static W1 core0_digitalPlay_timer_(repeating_timer_t* rt_)
	{
		//printf("%X",audio::audioPlayerInstance);
		audio::audioPlayerInstance->core0_digitalPlay_timer< BITSZ_,DT>();
		return true;
	}
	static W1 core0_pwmPlay_timer_(repeating_timer_t* rt_)
	{
		audio::audioPlayerInstance->core0_pwmPlay_timer();
		return true;
	}
	static W0 core0_pwmPlay_int_()
	{
		audio::audioPlayerInstance->core0_pwmPlay_int();
	}
	
	static W0 core1_espRead_()
	{
		audio::audioPlayerInstance->core1_espRead();
	}
	template<int BITSZ_, typename DT>
	static W0 core1_fileRead_()
	{
		audio::audioPlayerInstance->core1_fileRead<BITSZ_,DT>();
	}
//////////////////////////////////////////////////////
	
	W0 initVars_()
	{
		pwmRepeatCounter=0;
		lastSampleValue16	= 0;
		core1finished		= false;
		waitFirst			= true; 
		currSampleID		= 0;
		currPosSample		= 0;
		currPlayBit			= 0;
		pwmFrameID = 0; 
	}
	template<int BITSZ_, typename DT>
	W1 init_(W1 isPWM_=false)
	{
		if (!isFile && waveHeader.wfex.wBitsPerSample!=8)
		{
			DEBUG_AUDIOPLAYER("cannot if >8bits",0);
			return 0;
		}
		DEBUG_AUDIOPLAYER("sample rate:% u bits:%u | encode bits:%i rle:%i ", 
				waveHeader.wfex.nSamplesPerSec, 
				waveHeader.wfex.wBitsPerSample,
				pwmBits, pwmHaveRLE);
		delay_us = ((1.0f/(FP32)waveHeader.wfex.nSamplesPerSec)*1e6);

		DEBUG_AUDIOPLAYER("delay %u us", delay_us);
		

		audioPlayerInstance = this;
		if (isFile)
		{
			if (BITSZ_==1)
			{
				samplesInBytes = AUDIOPLAYER_BUFFERSZ;
				samples        = samplesInBytes*8;
				totalSamples   = waveHeader.lDataSize*8;
				playBuffers[0] = new DT[samplesInBytes];
				playBuffers[1] = new DT[samplesInBytes];
			}
			else
			{
				samplesInBytes = AUDIOPLAYER_BUFFERSZ;
				samples        = samplesInBytes/sizeof(DT); // 1~cluster
				totalSamples   = waveHeader.lDataSize/sizeof(DT);
				playBuffers[0] = new DT[samples];
				playBuffers[1] = new DT[samples];
			}
		}
		else
		{
			totalSamples   = 0xffffffff;
			samplesInBytes = samples;
			playBuffers[0] = new DT[samples];
			playBuffers[1] = new DT[samples];
		}

		if (!(playBuffers[0] && playBuffers[1]))
		{
			DEBUG_AUDIOPLAYER("cannot alloc %u samples", samples);
			return 1;
		}
		memset((W8*)playBuffers[1], 0, samplesInBytes); 

		DEBUG_AUDIOPLAYER("bufsz:%u total:%u", samples, totalSamples);
		if (isPWM_)
		{
			gpio_set_function(audioPin, GPIO_FUNC_PWM);
			float clockfreq = (float)clock_get_hz(clk_sys);
			int slice = pwm_gpio_to_slice_num(audioPin);
			pwm_set_enabled(slice, true);
			pwm_config config = pwm_get_default_config();
			

			float div = 
			#ifdef AUDIOPLAYER_PWM_USES_IRQ
				clockfreq / ((FP32)waveHeader.wfex.nSamplesPerSec * (AUDIO_PLAYER_DEF_PWM_WRAP+1)* (FP32)(AUDIOPLAYER_PWM_OVERSAMPLE));
				pwm_config_set_clkdiv(&config, div);  
				printf("\nclk freq: %3.2fHz pwm div: %3.2f", clockfreq, div);
			#else
				clockfreq / ((FP32)waveHeader.wfex.nSamplesPerSec * AUDIOPLAYER_PWM_OVERSAMPLE);
				pwm_config_set_clkdiv(&config, div);
			#endif


			if (pwmBits==0)
				pwm_config_set_wrap(&config, AUDIO_PLAYER_DEF_PWM_WRAP);
			else
				pwm_config_set_wrap(&config, (1<<pwmBits)-1);

			pwm_init(slice, &config, true);
			pwm_set_gpio_level(audioPin, 0);

			#ifdef AUDIOPLAYER_PWM_USES_IRQ
				pwm_clear_irq(slice);
				irq_set_exclusive_handler(PWM_IRQ_WRAP, core0_pwmPlay_int_); 
			#endif

			//printf("div %3.2f", div);
		}
		return 0;
	}
	template<typename DT>
	W0 destroy_()
	{
		if (playBuffers[0])
			delete[] (DT*)playBuffers[0];
		if (playBuffers[1])
			delete[] (DT*)playBuffers[1];
	}
	W0 stop()
	{ 
		multicore_reset_core1();
		DELAY_MS(500);
		cancel_repeating_timer(&timer);
		DELAY_MS(500);
		core1finished = true;
		currPosSample = totalSamples;
	}

// CORE 0: tocar
	template<int BITSZ_, typename DT>
	W1 digitalPlay_()
	{ 
		initVars_();
		multicore_reset_core1();
		if (isFile)
			multicore_launch_core1(&core1_fileRead_<BITSZ_,DT>);
		else
			multicore_launch_core1(&core1_espRead_);
		
		while(waitFirst)
		{
			tight_loop_contents(); 
		}		

		const DT* buffer = (const DT*) playBuffers[0];	
		lastSampleValue16 = buffer[0];
		add_repeating_timer_us(delay_us, core0_digitalPlay_timer_<BITSZ_,DT>,
								NULL, &timer);
		// aqui requereu que currPosSample e totalSamples fossem
		// volatile?
		
		while(currPosSample<totalSamples)
		{
			tight_loop_contents(); 
		}
		cancel_repeating_timer(&timer); 

		while(!core1finished)
		{
			dataIsReady = false;
		}

		return 0;
	}
	W1 pwmPlay(SW16 usAdjust_=0)
	{ 
		initVars_();

		//const W32 delay = (FP32)usAdjust_+ (delay_us/(FP32)AUDIOPLAYER_PWM_FRAMES);
		const W32 delay = delay_us;
		
		multicore_reset_core1();
		if (isFile)
			multicore_launch_core1(&core1_fileRead_<8,W8>);
		else
			multicore_launch_core1(&core1_espRead_);
		 
		
		while(waitFirst)
		{
			tight_loop_contents(); 
		}		
		AUDIOPLAYER_SWAPBUFFERS
		

		#ifdef AUDIOPLAYER_PWM_USES_IRQ
			irq_set_enabled(PWM_IRQ_WRAP, true);
			pwm_set_irq_enabled(pwm_gpio_to_slice_num(audioPin), true);
		#else
			add_repeating_timer_us(delay, core0_pwmPlay_timer_, NULL, &timer);
		#endif

	
		while(currPosSample<totalSamples)
		{
			tight_loop_contents(); 
		}
		
		#ifdef AUDIOPLAYER_PWM_USES_IRQ
			irq_set_enabled(PWM_IRQ_WRAP, false);
			pwm_set_irq_enabled(pwm_gpio_to_slice_num(audioPin), false);
		#else
			cancel_repeating_timer(&timer); 
		#endif
		
		while(!core1finished)
		{
			dataIsReady = false;
		}

		 

		return 0;
	}

//
	W1 init(const W32 audioPin_, SW8 bitsPerSample_=0, W16 sampleRate_=0)
	{
		audioPin = audioPin_;
		PIN_INIT(audioPin)
		PIN_MODE(audioPin,	PIN_MODE_OUTPUT)
		#ifdef AUDIOPLAYER_AUXPIN
			PIN_INIT(AUDIOPLAYER_AUXPIN)
			PIN_MODE(AUDIOPLAYER_AUXPIN,PIN_MODE_OUTPUT)
		#endif
		waveHeader.fill(0, sampleRate_, bitsPerSample_);
		return 0;
	}
	W1 open(const SW8* filename_, W1 isPWM_=false)
	{
		isFile = 1;
		#ifdef APP_USES_SD
		W32 bytesRW;
		res = fs_createFileNode(filename_,&node,0,true);
		if (res!=FS_OK)
		{
			DEBUG_AUDIOPLAYER("open file",0);
			return 1;
		}
		
		res = fs_read(&node, &waveHeader, sizeof(WAVEFORM_), bytesRW);
		if (res!=FS_OK)
		{
			DEBUG_AUDIOPLAYER("read header",0);
			return 1;
		}
		#endif
		if (init_<8, W8>()) return 1;
		return 0;
	}

	#ifdef APP_USES_WIFI
	W1 initEsp(const W32 audioPin_, ESP_PacketVoiceHeader& voiceHeader, W1 isPWM_=false)
	{
			isFile = 0;
			samples = voiceHeader.bytesPerBlock;
			pwmBits = voiceHeader.pwmBits;
			
			/*
			if (pwmBits)
				AUDIOPLAYER_PWM_FRAMES = 1<<voiceHeader.pwmBits;
			else
				AUDIOPLAYER_PWM_FRAMES = AUDIOPLAYER_DEFAULT_PWM_FRAMES;
			*/

			pwmHaveRLE = voiceHeader.pwmHaveRLE?1:0; 
			init(audioPin_,voiceHeader.bitsPerSample, voiceHeader.sampleRate);
		if (init_<8, W8>(isPWM_)) return 1;
		return 0;
		error:
		DEBUG_AUDIOPLAYER_ESP("ERR",0);
		return 1;
	}
	#endif

	W1 digitalPlay()
	{ 
		switch (waveHeader.wfex.wBitsPerSample) 
		{
			case 1:return digitalPlay_<1,W8>();
			case 8:return digitalPlay_<8,W8>();
			case 16:return digitalPlay_<16,SW16>();
		}
		return 0;
	}

	AudioPlayer()
	{
		SAMPLE_VALUE_MIN  = 120;
		completionFunction = 0;
	}
	~AudioPlayer()
	{
		if (waveHeader.wfex.wBitsPerSample==8)
			destroy_<W8>();
		else
			destroy_<SW16>();
	}
	FP32 pwmDy;
};

        
#ifdef PWM_USE_TAB
	const W8 pwmTab[] VL_STORE_IN_FLASH = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11,
	11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 29, 29, 30, 31, 31, 
	32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 62, 63, 64, 65, 66, 68
	, 69, 70, 71, 73, 74, 75, 77, 78, 79, 81, 82, 84, 85, 86, 88, 89, 91, 92, 94, 95, 97, 99, 100, 102, 103, 105, 107, 108, 110, 112, 114, 115, 117, 119,
	 121, 123, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 157, 159, 161, 163, 165, 168, 170, 172, 174, 177, 179, 182
	, 184, 186, 189, 191, 194, 196, 199, 201, 204, 207, 209, 212, 214, 217, 220, 223, 225, 228, 231, 234, 236, 239, 242, 245, 248, 251, 254, 254,254};

	#define PWM_TAB(X) pwmTab[(W8)(X)]

#else
	#define PWM_TAB(X) X
#endif

W0 AudioPlayer::core0_pwmPlay_int()
{
	// *** não fazer nada se chegou ao fim
	if (currPosSample>=totalSamples)
	{
		//pwm_set_gpio_level(audioPin, 0);
		dataIsReady = false;
		goto reti;
	}
	if (!dataIsReady) goto reti;
		
	if (pwmFrameID == 0)
	{ 
		const W8* buffer = (const W8*) playBuffers[1];	
		const W8 val = buffer[currSampleID]; //W8SYM(buffer[currSampleID]);

		#ifdef PWM_USE_DY
			lastSampleValue8 = val;
			pwmDy = ((FP32)buffer[currSampleID+1]-(FP32)val) / (FP32)AUDIOPLAYER_PWM_OVERSAMPLE;
		#else
			if (val != lastSampleValue8)
			{
				pwm_set_gpio_level(audioPin, PWM_TAB(val));
				lastSampleValue8 = val;
			}
		#endif
		//if (val != lastSampleValue8)
		//{
		//	//if (val>=128)
		//	//	pwm_set_gpio_level(audioPin, val-128);
		//	//else
		//	//	pwm_set_gpio_level(audioPin, 128-val);
		//	pwm_set_gpio_level(audioPin, val);
		//	lastSampleValue8 = val;
		//}

		++currPosSample;
		++currSampleID;
		
		if (currSampleID == availableSamples) 
		{
			#ifdef PWM_USE_DY
				lastSampleValue8 =  PWM_TAB( (FP32)lastSampleValue8 + (FP32)pwmFrameID*pwmDy ) ;
			#endif
			currSampleID = 0;	
			pwmFrameID = 0;
			AUDIOPLAYER_SWAPBUFFERS	 
			dataIsReady = false;
			pwm_clear_irq(pwm_gpio_to_slice_num(audioPin));
			goto reti;
		}
	}
	
	#ifdef PWM_USE_DY
		pwm_set_gpio_level(audioPin, PWM_TAB( (FP32)lastSampleValue8 + (FP32)pwmFrameID*pwmDy )  );
	#endif

	if (++pwmFrameID == AUDIOPLAYER_PWM_OVERSAMPLE) pwmFrameID=0;

	pwm_clear_irq(pwm_gpio_to_slice_num(audioPin));
	return;

reti:
	//pwm_set_gpio_level(audioPin, lastSampleValue8);
	//lastSampleValue8=(FP32)lastSampleValue8/2;
	pwm_clear_irq(pwm_gpio_to_slice_num(audioPin));
}


};

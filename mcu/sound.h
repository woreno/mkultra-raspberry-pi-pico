#ifndef MCU_SOUND_H
#define MCU_SOUND_H
#define BUZZER_SAMPLE_RATE 44100

#include "hardware/clocks.h"
#include "hardware/pwm.h"

// subchunk de formato PCM do contentor WAVE
VL_PACK_BEGIN
struct VL_PACK_MIDDLE WAVEFORM_PCM_  
{
	W16 wFormatTag;     // formato: 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	W16 nChannels;		// nº de canais
	W32 nSamplesPerSec; // amostras por segundo
	W32 nAvgBytesPerSec;
	W16 nBlockAlign;	// 2=16-bit mono, 4=16-bit stereo 
	W16 wBitsPerSample; // bits por amostra
}; 
VL_PACK_END

// header de contentor WAVE
VL_PACK_BEGIN
struct VL_PACK_MIDDLE WAVEFORM_ 
{
	W8			szRIFF[4]; //B.E.  assinatura "RIFF"
	W32			lRIFFSize; //      tamanho do ficheiro -8 bytes (excluindo até aqui)
	W8			szWave[4]; //B.E.  assinatura "WAVE"
	//-- subchunk de formato ------------------------------------------
	W8			wfexFmt[4];  //B.E.  assintura "fmt "
	W32			wfexSz;          // é o tamanho de wfex; 16 para PCM
	WAVEFORM_PCM_	 wfex;	    //        	
	//-- subchunk de dados --------------------------------------------
	W8  		szData[4];    // assinatura "data"
	W32			lDataSize;    //o tamanho do som

	W1 isSigned() const 
	{
		return (!(wfex.wBitsPerSample==8)); // por padrao Wav
	}
	W32 getNSamples() const 
	{
		return (lDataSize/(wfex.wBitsPerSample>>3))/wfex.nChannels;
	}
	W0 fill(W32 dataByteSize_, W32 sampleRate_, W32 bitsPerSample_, W32 chans_=1 )	
	{
		VL_ARR4(szRIFF,'R','I','F','F')
		VL_ARR4(szWave,'W','A','V','E')
		lRIFFSize = dataByteSize_ + sizeof(WAVEFORM_) - 8;
		VL_ARR4(wfexFmt,'f','m','t',32)
		wfexSz = sizeof(wfex);
		VL_ARR4(szData,'d','a','t','a')
		lDataSize = dataByteSize_ ;
		wfex.nChannels = chans_;
		wfex.nSamplesPerSec = sampleRate_;
		wfex.wBitsPerSample = bitsPerSample_;
		wfex.wFormatTag = 1; //PCM
		wfex.nBlockAlign = 0; //(chans_==2)?4:2; 
	}

};
VL_PACK_END

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
	

W8  sound_welcome_m[]  = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
W8  sound_welcome_d[]  = {4, 5, 5, 5, 0xAA};
W8    sound_start_m[]  = {NOTE_G5, 0, NOTE_G5, 0, NOTE_G5};
W8   sound_start_d[]   = {5, 2, 5, 2, 3, 0xAA};
W8     sound_beep_m[]  = {NOTE_G5};
W8    sound_beep_d[]   = {5, 0xAA};
W8     sound_stop_m[]  = {NOTE_G4, 0, NOTE_G4};
W8    sound_stop_d[]   = {5, 1, 5, 0xAA};
W8    sound_error_m[]  = {NOTE_FS1, 0, NOTE_FS1, 0, NOTE_FS1};
W8   sound_error_d[]   = {2, 1, 2, 1, 3, 0xAA};


enum SOUNDS {	SOUND_WELCOME=0, SOUND_START,  SOUND_BEEP, SOUND_STOP,  SOUND_ERROR };
W8  *sounds_melodies[] = {sound_welcome_m, sound_start_m, sound_beep_m, sound_stop_m, sound_error_m};
W8  *sounds_duration[] = {sound_welcome_d, sound_start_d, sound_beep_d, sound_stop_d, sound_error_d};


#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)

	uint buzzer_slice;
	pwm_config buzzer_cfg;

	W0   sound_init()
	{
		PIN_INIT(PIN_BUZZER)
		PIN_MODE(PIN_BUZZER,		  PIN_MODE_OUTPUT)
		gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
		buzzer_slice = pwm_gpio_to_slice_num(PIN_BUZZER);
		buzzer_cfg = pwm_get_default_config();
	}
	W0   sound_set_freq(uint freq, uint duty_c=5000 )
	{
		float div = (float)clock_get_hz(clk_sys) / (freq * 10000);
		pwm_config_set_clkdiv(&buzzer_cfg, div);
		pwm_config_set_wrap(&buzzer_cfg, 10000); 
		pwm_init(buzzer_slice, &buzzer_cfg, true);
		pwm_set_gpio_level(PIN_BUZZER, duty_c); 
	};
	W0   tone(W32 freq_)
	{
		pwm_set_enabled(buzzer_slice, true);
		sound_set_freq(freq_);
	}
	W0   noTone()
	{
		pwm_set_enabled(buzzer_slice, false);
	}
	W0   sound_emit(int SOUND_)
	{
	  W8 *melody = sounds_melodies[SOUND_];
	  W8 *durations = sounds_duration[SOUND_];
 
	  while(*durations!=0xAA)
	  {
		if (*melody)
			tone(*melody);
		else
			noTone();
	
		sleep_ms(*durations*40);
		durations++;
		melody++;
	  }
	  noTone();
	}
	W0   sound_alert(int cmd_)
	{
		sound_emit(cmd_);
		blink(2,100);
	}


#endif
      

#endif
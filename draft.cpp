

			//#define UI_ROTATE_SAMPLES 5
			//W8 data[UI_ROTATE_SAMPLES];
			//W8 clk[UI_ROTATE_SAMPLES];
			////             data
			//// clock down: X X X
			//// clock up:   X X X
			//W32 up = 0, down=0;
			//for (W32 i=0; i< UI_ROTATE_SAMPLES; i++)
			//{
			//	const W8 clkNow = PIN_READ(PIN_ROTARY_ENCODER_CLK);
			//	while(clkNow == PIN_READ(PIN_ROTARY_ENCODER_CLK)) { }
			//
			//	const W8   data = PIN_READ(PIN_ROTARY_ENCODER_DATA);
			//	
			//		if ( (clkNow==data) )
			//			up++;
			//		else
			//			down++;
			//	
			//	while(clkNow == PIN_READ(PIN_ROTARY_ENCODER_CLK)) { }
			//} 
			//
			//// esperar que passem os restantes clocks
			//
			//
			//printf("\nUp:%i Down:%i ? %i", up , down, (up>=down)?1:0);
			//
			//
			//lastData = data;

W0 app_core0_periodic_sample()
{
	#if (AUDIO_CHANS == 1)
		buffer[sampleI]=ENCODE(adc_read());
		sampleI++;
	#else
		adc_select_input(MICRO_LEFT);
		buffer[sampleI] = ENCODE(adc_read());
		adc_select_input(MICRO_RIGTH);
		buffer[sampleI] = ENCODE(adc_read());
		sampleI+=2;
		DELAY_US(AUDIO_MICROSEC_DELAY_2CHANS); 
	#endif
	if (sampleI == ARR_SZ)
	{
		multicore_fifo_push_blocking(arrayI);
		arrayI++;
		if (arrayI==ARR_BUFFERING)
		{
			sampleI = 0;
			arrayI = 0;
		}
	}
	if (command==COMMAND_STOP_RECORDING) { app_core0_stop(); return; }
}
	
//periph_clock_enable();
//uart_disable(); 
//uart_init(uart0, 9600); 
//uart_set_baudrate(uart0, 9600);
//uart_set_hw_flow(uart0, false, false);
//uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
//uart_set_fifo_enabled(uart0, false);
//sleep_ms(4000); 
//while(1)
//{
//	uart_puts(uart0, "\nOK\n");
//	sleep_ms(1000);
//}
	
//sleep_ms(3000);
//printf("\nhello1");
//while(1)
//{
//	SW8 str[] = "\nhello";
//	fwrite(str,1,sizeof(str),stdout);
//	sleep_ms(1000);
//}
BOOL CALLBACK EnumerateHandlesCallback2(HANDLE handle, PVOID lParam) 
{
	if (GetFileType(handle)==FILE_TYPE_DISK)
	{
		printf("");
	}
    return TRUE;
}
W1 sd_check_procs(W64 devNumber_)
{
	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnapshot == INVALID_HANDLE_VALUE)
		return RES_FAIL;

	PROCESSENTRY32 processEntry = { 0 };
	processEntry.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnapshot, &processEntry))
		return RES_FAIL;
	do
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processEntry.th32ProcessID);
		if (hProcess != NULL)
		{
			DWORD processDriveMask = 0;
			GetProcessHandleCount(hProcess, &processDriveMask);
			dev_handleCount = 0;
			EnumProcessHandles(hProcess, &EnumerateHandlesCallback, 0);
			if (dev_handleCount)
			{
				printf("\n  Process %d: '%s'", processEntry.th32ProcessID, processEntry.szExeFile);
				EnumProcessHandles(hProcess, &EnumerateHandlesCallback2, 0);
			}
			
			CloseHandle(hProcess);
		}
	} while (Process32Next(hProcessSnapshot, &processEntry));
	
    CloseHandle(hProcessSnapshot);
	return RES_OK;
}


	/*
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!pSD)
    {
       SD_INIT_FAIL("PSECURITY_DESCRIPTOR LocalAlloc")
    }
	
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        LocalFree(pSD);
        SD_INIT_FAIL("InitializeSecurityDescriptor")
    }
	PSID pAdminSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid))
    {
        LocalFree(pSD);
        SD_INIT_FAIL("AllocateAndInitializeSid")
    }
	
    if (!SetSecurityDescriptorOwner(pSD, pAdminSid, TRUE))
    {
        FreeSid(pAdminSid);
        LocalFree(pSD);
        SD_INIT_FAIL("SetSecurityDescriptorOwner")
    }

	// Convert the security descriptor to a string for debugging purposes
    LPTSTR pSDString = NULL;
    if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSD, SDDL_REVISION_1, OWNER_SECURITY_INFORMATION, &pSDString, NULL))
    {
        FreeSid(pAdminSid);
        LocalFree(pSD);
        SD_INIT_FAIL("ConvertSecurityDescriptorToStringSecurityDescriptor")
    }

    // Print the security descriptor for debugging purposes
    _tprintf(_T("Security Descriptor: %s\n"), pSDString);
	
	// Define the security attributes for the file
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &pSD; // Use default security descriptor
    sa.bInheritHandle = FALSE;		// Do not inherit handle
	*/



	#else
	#ifndef USE_TX_ENCODING
		for (W32 i=0; i< size_; i++)
		{
			tud_cdc_write_char(buf[i]);
		}
	#else
		W8 SPECIAL_CHAR = 0xFF;
		for (W32 i=0; i< size_; i++)
		{
			W8 ch = buf[i];
			if (ch==SPECIAL_CHAR||ch==0x11||ch==0x13||ch==0xF6||ch==0x0C)
			{
				W8 code;
				switch(ch)
				{
					case 0xFF: code=0; break;
					case 0x11: code=1; break;
					case 0x13: code=2; break;
					case 0xF6: code=3; break;
					case 0x0C: code=4; break;
				}
				USE_TX_WAIT
				tud_cdc_write_char(SPECIAL_CHAR);
				tud_cdc_write_flush();
				USE_TX_WAIT
				tud_cdc_write_char(code);
				tud_cdc_write_flush();
				continue;
			}
			USE_TX_WAIT
			tud_cdc_write_char(ch);
			tud_cdc_write_flush();
		}

	#endif
	#endif


		
W0 tx_xonxoff()
{
	W8 byte;
	if (tud_cdc_available())
	{
		byte = tud_cdc_read_char();
		if (byte&0xFF==0x13) // XOFF
		{
			while(1)
			{
				if (tud_cdc_available())
				{
					byte = tud_cdc_read_char();
					if (byte&0xFF==0x11) // XON
						break;
				}
			}
		}
	}
}
#if (DEBUG_AUDIO==DEBUG_AUDIO_ADC)

W0 audio_adc_test()
{
	SW16 L, R;
	while(1)
	{
		#define TEST_BUF_SZ 32
		SW16* buf = buffer;
		W32 i = TEST_BUF_SZ;
		while(i)
		{
			i--;
			#if (AUDIO_CHANS == 1)
				*buf = ENCODE(adc_read());
				printf("\nL %i", buf[0]);
				buf++;
				sleep_us(AUDIO_MICROSEC_DELAY); 
			#else
				adc_select_input(MICRO_LEFT);
				buf[0] = ENCODE(adc_read());
				adc_select_input(MICRO_RIGTH);
				buf[1] = ENCODE(adc_read());
				printf("\nL %i \t\tR %i", buf[0], buf[1]);
				buf+=2;
				sleep_us(AUDIO_MICROSEC_DELAY_2CHANS); 
			#endif
			sleep_ms(100); 
		}
		

		/*
		float av[] = {0,0};
		i = TEST_BUF_SZ;
		buf = buffer;
		while(i)
		{
			i--;
			#if (AUDIO_CHANS == 1)
				av[0]+=*buf;
				buf++;
			#else
				av[0]+=buf[0];
				av[1]+=buf[1];
				buf+=2;
			#endif
		}
		av[0]/=(float)TEST_BUF_SZ;
		av[1]/=(float)TEST_BUF_SZ;
		#if (AUDIO_CHANS==1)
			printf("\nL %2.3f", av[0]);
		#else
			printf("\nL %2.3f \t\tR %2.3f", av[0], av[1]);
		#endif
		*/
	}
}
#endif	



		// as N arrays sao preenchida
		SW16 *buf = buffer;
		SW16 test = 0;
		for(W32 j=0; j< ARR_BUFFERING; j++)
		{
			DEBUG_LOG("\ncore0: sample %j", j)
			W32 i = (ARR_SZ/AUDIO_CHANS);
			while(i)
			{
				i--;
				#if (AUDIO_CHANS == 1)
					*buf = test++; //
					//ENCODE(adc_read());
					buf++;
					DELAY_US(AUDIO_MICROSEC_DELAY); 
				#else
				#endif
			}
			DEBUG_LOG("\ncore0: sample %j", j)

			multicore_fifo_push_blocking(j);

			if (command==COMMAND_STOP_RECORDING) { app_core0_stop(); return; }

		}
	}#include "audio_provider.h"
#include "micro_features/micro_model_settings.h"

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include <limits>

// set this to determine sample rate
// 0     = 500,000 Hz
// 960   = 50,000 Hz
// 9600  = 5,000 Hz
// -> 3000 = 16,000 Hz
#define CLOCK_DIV 3000 //16Khz

// Channel 0 is GPIO26
#define ADC_PIN 26
#define CAPTURE_CHANNEL 0

#define NSAMP 1024
#define BUFFER_SIZE 16000

namespace {
// dma settings
dma_channel_config cfg;
uint dma_chan;

uint16_t g_audio_sample_buffer[NSAMP];
// tflite micro settings
bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
constexpr int kAudioCaptureBufferSize = NSAMP * 16;
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];
// A buffer that holds our output
int16_t g_audio_output_buffer[kMaxAudioSampleSize];
// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
volatile int32_t g_latest_audio_timestamp = 0;

} // namespace

//this next function is the dma interupt
void CaptureSamples() {
  // data processing
  const int number_of_samples = NSAMP;
  // Calculate what timestamp the last audio sample represents
  const int32_t time_in_ms = g_latest_audio_timestamp + (number_of_samples / (kAudioSampleFrequency / 1000));
  // Determine the index, in the history of all samples, of the last sample
  const int32_t start_sample_offset = g_latest_audio_timestamp * (kAudioSampleFrequency / 1000);
  // Determine the index of this sample in our ring buffer
  const int capture_index = start_sample_offset % kAudioCaptureBufferSize;
  // Read the data to the correct place in our buffer
  memcpy(g_audio_capture_buffer + capture_index, (void *)g_audio_sample_buffer, sizeof(int16_t)*number_of_samples);

  // Clear the interrupt request.
  dma_hw->ints0 = 1u << dma_chan;
  // Give the channel a new wave table entry to read from, and re-trigger it
  dma_channel_set_write_addr(dma_chan, g_audio_sample_buffer, true);

  g_latest_audio_timestamp = time_in_ms;
}

void setup() {
  adc_gpio_init(ADC_PIN + CAPTURE_CHANNEL);

  adc_init();
  adc_select_input(CAPTURE_CHANNEL);
  adc_fifo_setup(
		 true,    // Write each completed conversion to the sample FIFO
		 true,    // Enable DMA data request (DREQ)
		 1,       // DREQ (and IRQ) asserted when at least 1 sample present
		 false,   // We won't see the ERR bit because of 8 bit reads; disable.
		 false     // Shift each sample to 8 bits when pushing to FIFO
		 );

  // set sample rate
  adc_set_clkdiv(CLOCK_DIV);

  sleep_ms(1000);
  // Set up the DMA to start transferring data as soon as it appears in FIFO
  uint dma_chan = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dma_chan);

  // Reading from constant address, writing to incrementing byte addresses
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);

  // Pace transfers based on availability of ADC samples
  channel_config_set_dreq(&cfg, DREQ_ADC);

  dma_channel_configure(dma_chan, &cfg,
			NULL,    // dst
			&adc_hw->fifo,  // src
			NSAMP,          // transfer count
			false            // start immediately
	);

  // Tell the DMA to raise IRQ line 0 when the channel finishes a block
  dma_channel_set_irq0_enabled(dma_chan, true);
  // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
  irq_set_exclusive_handler(DMA_IRQ_0, CaptureSamples);
  irq_set_enabled(DMA_IRQ_0, true);

  adc_run(true); //start running the adc
}

TfLiteStatus InitAudioRecording(tflite::ErrorReporter* error_reporter) {
  // Hook up the callback that will be called with each sample
  setup();
  // Manually call the handler once, to trigger the first transfer
  CaptureSamples();
  // let first samples roll in
  sleep_ms(1000);
  // Block until we have our first audio sample
  while (!g_latest_audio_timestamp) {
  }

  return kTfLiteOk;
}

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter,
                             int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
  // Set everything up to start receiving audio
  if (!g_is_audio_initialized) {
    TfLiteStatus init_status = InitAudioRecording(error_reporter);
    if (init_status != kTfLiteOk) {
      return init_status;
    }
    g_is_audio_initialized = true;
  }
  // This next part should only be called when the main thread notices that the
  // latest audio sample data timestamp has changed, so that there's new data
  // in the capture ring buffer. The ring buffer will eventually wrap around and
  // overwrite the data, but the assumption is that the main thread is checking
  // often enough and the buffer is large enough that this call will be made
  // before that happens.

  const int16_t kAdcSampleDC = 2048;
  const int16_t kAdcSampleGain = 20;

  // Determine the index, in the history of all samples, of the first
  // sample we want
  //printf("getting audio samples\n");
  const int start_offset = start_ms * (kAudioSampleFrequency / 1000);
  // Determine how many samples we want in total
  const int duration_sample_count = duration_ms * (kAudioSampleFrequency / 1000);
  for (int i = 0; i < duration_sample_count; ++i) {
    // For each sample, transform its index in the history of all samples into
    // its index in g_audio_capture_buffer
    const int capture_index = (start_offset + i) % kAudioCaptureBufferSize;
    const int32_t capture_value = g_audio_capture_buffer[capture_index];
    int32_t output_value = capture_value - kAdcSampleDC;
    //
    output_value *= kAdcSampleGain;
    //printf("%d, \n", output_value);
    if (output_value < std::numeric_limits<int16_t>::min()) {
      output_value = std::numeric_limits<int16_t>::min();
    }
    if (output_value > std::numeric_limits<int16_t>::max()) {
      output_value = std::numeric_limits<int16_t>::max();
    }
    // Write the sample to the output buffer
    g_audio_output_buffer[i] = output_value;
    //printf("%d\n", output_value);
    //g_audio_output_buffer[i] = capture_value;
  }
  // Set pointers to provide access to the audio
  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_audio_output_buffer;

  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() { return g_latest_audio_timestamp; }
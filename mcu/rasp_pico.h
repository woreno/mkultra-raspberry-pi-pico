//#define TEST_RASPPICO
#define APP_TARGET APP_TARGET_RASPBERRY_PICO

#ifndef RASP_PICO_SYS_FREQ 
	#define RASP_PICO_SYS_FREQ_HZ 125000000	//KHz, 125MHz
#endif

#define RASP_PICO_ADC_BASE_FREQ_HZ 48000000.0f //Hz, 48Mhz

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"


#ifdef RASPBERRY_PICO_USES_PIO
	#include "hardware/pio.h"
	#include "hardware/clocks.h"
#endif

#include <atomic>


// lock 
#define MutexLock(X_)				 while (X_.test_and_set(std::memory_order_acquire));
#define MutexUnlock(X_)				 X_.clear(std::memory_order_release);
#define MutexCreate(X_)				 MutexUnlock(X_)
#define MutexType                    std::atomic_flag 

#define W0							void
#define W1							bool
#define W8							unsigned char
#define SW8							char
#define FP							float
#define W32							unsigned int 
#define SW32						signed int
#define W16							unsigned short
#define SW16						signed short 
#define FP32						float
#define VL_PACK_BEGIN 
#define VL_PACK_MIDDLE              __attribute__((packed))
#define VL_PACK_END					
#define VL_FLASH_READ_BYTE(X_,IDX_)	 (*((W8*)((X_)+((W32)IDX_))))  
#define VL_FLASH_READ_WORD(X_,IDX_)	 (*((W16*)((X_)+((W32)IDX_))))  
#define VL_ALIGN(X_)				__attribute__((aligned(X_))) 
#define VL_STORE_IN_FLASH			__in_flash()
#define VL_STORE_FLASH_GROUP(X_)	__in_flash(X_)
#define DELAY_US(X_)				busy_wait_us(X_); //busy_wait_at_least_cycles((((FP)X_)/0.008f)-3.0);
#define DELAY_MS(X_)				busy_wait_ms(X_);
#define DELAY_IDLE(X_)				pico_delay_(X_); 
#define DELAY_ATOMIC  __asm__ __volatile__("nop");	//__volatile__ obriga o optimizador a nao optimizar

inline W0 pico_delay_(W32 X_)
{
	while(X_--)
	{
		__asm volatile ("nop\n"); // 8ns
	}
}


#define PIN_LED						PICO_DEFAULT_LED_PIN // quinto pino a contar da direita/abaixo
				     // n. no pico
				     // |  
				     // v	
#define IO_PIN_0	0
#define IO_PIN_1	1
#define IO_PIN_2	2
#define IO_PIN_3	3
#define IO_PIN_4	4
#define IO_PIN_5	5
#define IO_PIN_6	6
#define IO_PIN_7	7
#define IO_PIN_8	8
#define IO_PIN_9	9
#define IO_PIN_10	10
#define IO_PIN_11	11
#define IO_PIN_12	12				
#define IO_PIN_13	13	
#define IO_PIN_14	14	
#define IO_PIN_15	15	
#define IO_PIN_16	16
#define IO_PIN_17	17	
#define IO_PIN_18	18
#define IO_PIN_19	19	
#define IO_PIN_20	20
#define IO_PIN_21	21
#define IO_PIN_22	22
#define IO_PIN_23	23
#define IO_PIN_24	24
#define IO_PIN_25	25
#define IO_PIN_26	26
#define IO_PIN_27	27
#define IO_PIN_28	28 	

#define IO_PIN_ADC0	IO_PIN_26
#define IO_PIN_ADC1	IO_PIN_27
#define IO_PIN_ADC2	IO_PIN_28

#define IO_PIN_ADC_ID(PIN_) \
			(((PIN_)==IO_PIN_ADC0)?0: \
			 (((PIN_)==IO_PIN_ADC1)?1:2))



#define HIGH 1
#define LOW 0


#define PICO_GPIO_SET  ((W32*)0xD0000014)
#define PICO_GPIO_CLR  ((W32*)0xD0000018)


//GPIO_IRQ_LEVEL... , GPIO_IRQ_EDGE_...
#define PIN_INSTALL_IRQ_ONRISE_AND_FALL(PIN_, CALLBACK_) gpio_set_irq_enabled_with_callback(PIN_, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, CALLBACK_);
#define PIN_INSTALL_IRQ_ONRISE(PIN_, CALLBACK_) gpio_set_irq_enabled_with_callback(PIN_, GPIO_IRQ_EDGE_RISE, true, CALLBACK_);
#define PIN_INSTALL_IRQ_ONFALL(PIN_, CALLBACK_) gpio_set_irq_enabled_with_callback(PIN_, GPIO_IRQ_EDGE_FALL, true, CALLBACK_);
#define	PIN_UNINSTALL_IRQ_ONFALL(PIN_)			gpio_set_irq_enabled_with_callback(PIN_, GPIO_IRQ_EDGE_FALL, false, 0);
#define	PIN_UNINSTALL_IRQ_ONRISE(PIN_)			gpio_set_irq_enabled_with_callback(PIN_, GPIO_IRQ_EDGE_RISE, false, 0);
#define PIN_IRQ_HANDLER(NAME_)					W0 NAME_(uint GPIO_, uint32_t EVENTMASK_)
#define PIN_IRQ_HANDLER_ACK						gpio_acknowledge_irq(GPIO_, EVENTMASK_);





// arduino compativel
#define LOW	 0
#define HIGH 1
#define F(X_)						X_

#define PIN_MODE_OUTPUT				GPIO_OUT
#define PIN_MODE_INPUT				GPIO_IN
#define PIN_SET_PULLUP(NUM_)		gpio_pull_up(NUM_);	
#define PIN_SET_PULLDOWN(NUM_)		gpio_pull_down(NUM_);	
#define PIN_SET_PWM(NUM_)			gpio_set_function(NUM_, GPIO_FUNC_PWM);


#define _PIN_INIT(X_,DIR_)\
		PIN_INIT(X_)\
		PIN_MODE(X_,DIR_)
#define PIN_INIT(NUM_)				gpio_init(NUM_);
#define PIN_MODE(NUM_,VAL_)			gpio_set_dir(NUM_, VAL_);
#define PIN_WRITE(NUM_,VAL_)		gpio_put(NUM_, VAL_)
#define PIN_READ(NUM_)				gpio_get(NUM_)
#define PIN_NUMBER(NUM_)			NUM_
#define PIN_LO(NUM_)				(*PICO_GPIO_CLR)|=(1<<NUM_); //mais lento!
#define PIN_HI(NUM_)				(*PICO_GPIO_SET)|=(1<<NUM_); //mais lento!
#define PIN_LO1(NUM_)				PIN_WRITE(NUM_,0);
#define PIN_HI1(NUM_)				PIN_WRITE(NUM_,1);

#define PIN_SETHI_RAW(X_)			*PICO_GPIO_SET = ((W32)(X_));
#define PIN_SETLO_RAW(X_)			*PICO_GPIO_CLR = ((W32)(X_));	

#define PIN_GETIO_RAW				(*PICO_GPIO_SET)

				
#define TEST_A_PIN(PIN_DEF_,INFO_)	test_pin(PIN_DEF_, #PIN_DEF_ , "",  INFO_ );
#define TEST_PINS(PINS_DEF_)		test_pin_init(); PINS_DEF_(TEST_A_PIN) printf("\n");// while(1){ } 


/// PWM
/*
#ifdef PIN_PWM_DEBUG
	PIN_PWM_CONFIG_DEBUG 	printf("\nClk:%3.2fHz | PWMFreq:%3.2fHz div:%3.2f", clockfreq, irqFreq, div);
#else
	PIN_PWM_CONFIG_DEBUG
#endif
PIN_PWM_CONFIG(PIN_, FREQ_, WRAP_SIZE_) \
	const FP32 CLK_FREQ = (float)clock_get_hz(clk_sys); \
	const FP32 CLK_DIV  = CLK_FREQ/(FREQ_*WRAP_SIZE_); \
	PIN_PWM_SLICE1	= pwm_gpio_to_slice_num(PIN_ULTRASPEAKER); \
	gpio_set_function(PIN_, GPIO_FUNC_PWM); \
	pwm_set_enabled(PIN_PWM_SLICE1, true); \
	pwm_config pconfig = pwm_get_default_config(); \
	pwm_config_set_clkdiv(&pconfig, CLK_DIV);   \
	pwm_config_set_wrap(&pconfig, PIN_); \
	pwm_init(slice, &pconfig, true); \
	PIN_PWM_CONFIG_DEBUG

	
PIN_PWM_CONFIG(PIN_, FREQ_, WRAP_SIZE_)

*/

W0 mcu_init(W16 mcuSharedBufferSz_=512)
{
	mcu_init_(mcuSharedBufferSz_);
	//set_sys_clock_khz(RASP_PICO_SYS_FREQ_HZ/1000,true); //,false);
	stdio_init_all();
	irq_set_mask_enabled(0,true);
	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);
	
	#ifdef TEST_RASPPICO
		const uint LED_PIN = PICO_DEFAULT_LED_PIN;
		gpio_init(LED_PIN);
		gpio_set_dir(LED_PIN, GPIO_OUT);
		while (true)
		{
			printf("Hello, world!\n");
			gpio_put(LED_PIN, 1);
			sleep_ms(250);
			gpio_put(LED_PIN, 0);
			sleep_ms(250);
		}
	#endif

}
 


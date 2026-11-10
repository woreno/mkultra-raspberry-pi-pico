
#define ULTRAWAVE_FRAMES	10
#define ULTRAWAVE_BITS		8
#define ULTRAWAVE_WRAP_SIZE ((1<<ULTRAWAVE_BITS))
#define ULTRAWAVE_WRAP_AT	((1<<ULTRAWAVE_BITS)-1)
W8 sinTable[ULTRAWAVE_FRAMES];
W8 ultraWaveFrameId;


W0 play_ultra_wave_irq()
{
	pwm_set_gpio_level(PIN_ULTRASPEAKER, sinTable[config.ultraWaveFrameId]);
	//printf("\n%i",sinTable[config.ultraWaveFrameId]);

	if (++ultraWaveFrameId == ULTRAWAVE_FRAMES)
		ultraWaveFrameId = 0;
	
	pwm_clear_irq(pwm_gpio_to_slice_num(PIN_ULTRASPEAKER));
} 

W0 play_ultra_wave(W0* args_)
{
	for (W16 i=0; i< ULTRAWAVE_FRAMES; i++)
	{
		const FP32 x01   = (FP32)i/((FP32)ULTRAWAVE_FRAMES-1);
		const FP32 angle = 2.0f*VL_PI*x01;
		const FP32 s     = 0.5f + std::sin(angle) * 0.5f ;
		sinTable[i] = s * (FP32)(ULTRAWAVE_WRAP_AT);
		//printf("\nX:%3.2f A:%3.2f N:%3.2f %i",x01, angle, s, sinTable[i]);
	}
	//ui_uninstall_buttons();
	
	FP32 clockfreq	= (float)clock_get_hz(clk_sys);
	SW32 slice		= pwm_gpio_to_slice_num(PIN_ULTRASPEAKER);
	pwm_clear_irq(slice);

	if (!config.ultraWaveOn)
	{
		irq_set_enabled(PWM_IRQ_WRAP, false);
		pwm_set_irq_enabled(slice, false);
		//ui_display_str(ui_strs_off, ui_strs_playultrawave);
		ui_menu_draw();
		ui_install_buttons();
		return;
	}
	ultraWaveFrameId = 0;

	PIN_INIT(PIN_ULTRASPEAKER)
	gpio_set_function(PIN_ULTRASPEAKER, GPIO_FUNC_PWM);
	pwm_set_enabled(slice, true);
	pwm_config pconfig = pwm_get_default_config();

	const FP32 irqFreq = (config.ultraFreq * (ULTRA_FREQ_UNIT) *
						 (FP32)ULTRAWAVE_FRAMES )/
						 ((FP32)ULTRAWAVE_WRAP_SIZE);

	// cpuFreq               cpuFreq                 
	// ------- = irqFreq <=> ------- =  div
	//   div				 irqFreq
	const FP32 div = clockfreq / irqFreq;
	
	printf("\nClk:%3.2fHz Ultra: IrqFreq:%3.2fHz div:%3.2f", clockfreq, irqFreq, div);

	// config
	pwm_config_set_clkdiv(&pconfig, div);  
	pwm_config_set_wrap(&pconfig, ULTRAWAVE_WRAP_AT);
	pwm_init(slice, &pconfig, true);
	
	irq_set_exclusive_handler(PWM_IRQ_WRAP, play_ultra_wave_irq); 
	irq_set_enabled(PWM_IRQ_WRAP, true);
	pwm_set_irq_enabled(slice, true);
	
	//ui_display_str(ui_strs_on, ui_strs_playultrawave);
	//ui_menu_draw();

	
	ui_install_buttons();
	//multicore_reset_core1();
	//multicore_launch_core1(&core1_fileRead_<8,W8>);
}

W0 play_ultra_soundfile(W0* args_)
{

}
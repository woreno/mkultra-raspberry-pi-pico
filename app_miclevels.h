
// MIC LEVELS

volatile W1 mic_levels_on;
PIN_IRQ_HANDLER(mic_levels_intr)
{
	mic_levels_on = false;
	PIN_IRQ_HANDLER_ACK
} 
W0 mic_levels(W0* args_)
{
	//
	#define LR_DX			10
	#define LR_DY			DISP_HEIGHT/5
	#define LR_WIDTH		(DISP_WIDTH-LR_DX*2)
	#define LR_SAMPLE_NUM	50

	W1 first = true;
	draw_initBuffer();
	draw_clear(0xFFFF);
	draw_text_centered(-1, LR_DY+4, ui_strs_miclevels, 0x0);
	DRAW_UPDATE
	draw_disposeBuffer();


	mic_levels_on = true;
	const W8 pins[] = { PIN_MIC_IN_LEFT,    PIN_MIC_IN_RIGHT   }, 
		      ids[] = { IO_PIN_ADC_ID(PIN_MIC_IN_LEFT), 
						IO_PIN_ADC_ID(PIN_MIC_IN_RIGHT) };
	
	adc_run(false);
	adc_gpio_init(PIN_MIC_IN_LEFT);
	adc_gpio_init(PIN_MIC_IN_RIGHT);
	adc_init();
	adc_set_temp_sensor_enabled(false);
	//adc_run(true);

	struct Sampling
	{
		FP32 vmax;
		FP32 vmin;
		FP32 vmean;
		FP32 samples[LR_SAMPLE_NUM];
		Sampling()
		{
			vmax=0;
			vmin=0;
			vmean=0;
		}
		W0 compute()
		{
			vmean = 0;
			for (W16 j=0; j< 2; j++)
			{
				for (W16 i=0; i< LR_SAMPLE_NUM; i++)
					vmean += samples[i];
				vmean /= (FP32)LR_SAMPLE_NUM;
			}
		}

	} sampling[2];

	PIN_UNINSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT) 
	PIN_INSTALL_IRQ_ONFALL(PIN_BUTTON_RIGHT, mic_levels_intr) 


	W16 it = 0;
	while(mic_levels_on)
	{
		for (W8 channel=0; channel< 2; channel++)
		{
			adc_select_input( ids[channel] );
			// o nivel é de 0..4095, mas tratar aqui como simetrico
			const FP32 sample =  ((FP32) (adc_read()&0xFFF) /4095.0f )-0.5f;
			sampling[channel].samples[it] = VL_ABS(sample)*2.0f;
		}

		if (++it==LR_SAMPLE_NUM)
		{
			it = 0;
			// calcular a média para cada canal
			FP32 means[2]; 

			for (W8 channel=0, 
					dy=2*LR_DY+6 ; 
					channel < 2; 
					channel++, dy+=LR_DY+4)
			{
				
				W16 y0 = dy, 
					y1 = dy+LR_DY-1;

				sampling[channel].compute();

				// calcular tamanho de cada retangulo 
				FP32 mean = VL_MIN(sampling[channel].vmean, 1.0f);
				means[channel] = mean;

				const W16 width = mean * ((FP32)LR_WIDTH-1);
			 
				W16 x0 = LR_DX;
				W16 x1 = LR_DX + width;
				if (x0!=x1)
					draw_rect( x0, 
							   y0, 
							   x1, 
							   y1,  
							   0, true);

				x0 = x1;
				x1 = LR_DX + LR_WIDTH - 1;
				if (x0!=x1)
					draw_rect( x0, 
							   y0, 
							   x1, 
							   y1,  
							   0xCCCC, true);
			}
			
			#if (APP_DEBUG_MIC_LEVELS==1)
				printf("\n%3.2f\t%3.2f", means[0], means[1]);
			#endif
			//printf("\n%3.2f %3.2f", sampling[0].vmean, sampling[1].vmean );
		} 
	}
	ui_install_buttons();
	ui_menu_draw();
}
 
// atrasos ---------------------------------------------------------------------------------

//#define DISP_UNROLL_LOOPS


// é necessário definir ST7735_PARTNUMBER_IS = ST7735_NFP096H_01AY ou ST7735_NFP096H_01AF
#define  ST7735_NFP096H_01AY 1		
#define  ST7735_NFP096H_01AF 2


#define DISP_COLOR_MK_W16(R_,G_,B_) ( ((W16)((B_&(0xF)))<<(8))| ((G_&(0xF))<<4) | (R_&(0xF)) )



#define PIN_DISP_CLK_HI		PIN_HI(PIN_DISP_CLK)
#define PIN_DISP_CLK_LOW	PIN_LO(PIN_DISP_CLK)

#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
	//#define DISP_CLK_DELAY1     DELAY_IDLE(37) // DELAY_IDLE(37)
	//#define DISP_CLK_DELAY2		DELAY_IDLE(5)  //DELAY_IDLE(7) // obrigatório para NFP096H-01AF

#ifndef DISP_CLK_DELAY1     
	#define DISP_CLK_DELAY1     DELAY_IDLE(38) // DELAY_IDLE(37)
#endif
#ifndef DISP_CLK_DELAY2     
	#define DISP_CLK_DELAY2		DELAY_IDLE(8)  //DELAY_IDLE(7) // obrigatório para NFP096H-01AF
#endif	
	// ao receber um lote diferente destes ecrãs ST7735
	// foi preciso adicionar no minimo DISP_CLK_DELAY2
	// porque não funcionavam
	// antigo lote (que funcionava sem DISP_CLK_DELAY2: NDB01QF ....)
	// novo lote (que nao funcionava sem DISP_CLK_DELAY2, e teve-se
	// de adicionar NFP096H-01AF) 


	// tenho um novo lote de adicionar NFP096H-01AY) 

#else
	#define DISP_CLK_DELAY1     DELAY_US(1.2)
	#define DISP_CLK_DELAY2		
#endif

// pinos -----------------------------------------------------------------------------------

#define DISP_PINS_DEF(X_)\
	X_(PIN_DISP_CLK,			 "p3")\
	X_(PIN_DISP_OUT,			 "p4")\
	X_(PIN_DISP_DC,				 "p6")\
	X_(PIN_DISP_CS,				 "p7")


//	X_(PIN_DISP_RES,			 "p5")\

// dimensões do ecrã -----------------------------------------------------------------------


#if (ST7735_PARTNUMBER_IS == ST7735_NFP096H_01AY)
	#define DISP_XSTART			((W8)0)
	#define DISP_YSTART			((W8)24)
#else
	#define DISP_XSTART			((W8)1)
	#define DISP_YSTART			((W8)25)
#endif

#define DISP_WIDTH			((W8)160)
#define DISP_HEIGHT			((W8)80)
#define DISP_TOTALPX		((W16)DISP_WIDTH*(W16)DISP_HEIGHT)

// rx/tx -----------------------------------------------------------------------------------

#define DISP_TXRAM(X0_, X1_, Y0_, Y1_)\
	const W8 ras[] = { DISP_CMD_RASET,  0, (W8)(Y0_)+DISP_YSTART, 0, (W8)(Y1_)+DISP_YSTART };\
	const W8 cas[] = { DISP_CMD_CASET,  0, (W8)(X0_)+DISP_XSTART, 0, (W8)(X1_)+DISP_XSTART };\
	const W8 ram[] = { DISP_CMD_RAMWR };\
	disp_txCmdData(sizeof(cas), (W0*)cas);\
	disp_txCmdData(sizeof(ras), (W0*)ras);\
	disp_txCmd(1, (W0*)ram);

#define DISP_TX	\
	PIN_DISP_CLK_HI			 \
	DISP_CLK_DELAY1			 \
	PIN_DISP_CLK_LOW		 \
	DISP_CLK_DELAY2			 
 
#define DISP_TX_DATA(DATA_,X_)\
if ((DATA_)&(1<<(X_)))		 \
	PIN_HI(PIN_DISP_OUT)	 \
else						 \
	PIN_LO(PIN_DISP_OUT)	 \
PIN_DISP_CLK_HI				 \
DISP_CLK_DELAY1				 \
PIN_DISP_CLK_LOW			 \
DISP_CLK_DELAY2			 

#define DISP_TX_DATA_SLOW(DATA_,X_)\
if ((DATA_)&(1<<(X_)))		 \
	PIN_HI(PIN_DISP_OUT)	 \
else						 \
	PIN_LO(PIN_DISP_OUT)	 \
PIN_DISP_CLK_HI				 \
DELAY_IDLE(37)				 \
PIN_DISP_CLK_LOW			 \
DELAY_IDLE(7)			

// Codificacao em W16:
// LSB							     MSB
//  0  1  2  3  4  5  6  7    0  1  2  3
// R0 R1 R2 R3 G0 G1 G2 G3 | B0 B1 B2 B3 12bits
//  |_______.  |_________.   |________
//     <----V       <----V       <----V
// Transmissao:
// R3 R2 R1 R0 G3 G2 G1 G0 | B3 B2 B1 B0 12bits
//  |_ primeiro a transmitir
//
//W16 len_:numero de repeticoes
W0 disp_txColor(const W16 data)
{
  if (data==0xFFF)
  {
    PIN_HI(PIN_DISP_OUT)
	goto txData;
  }
  else if (data==0)
  {
    PIN_LO(PIN_DISP_OUT)
	goto txData;
  }
  else
  {
	#ifdef DISP_UNROLL_LOOPS
		DISP_TX_DATA(data,3) // R3
		DISP_TX_DATA(data,2) // R2
		DISP_TX_DATA(data,1) // R1
		DISP_TX_DATA(data,0) // R0
		DISP_TX_DATA(data,7) // G3
		DISP_TX_DATA(data,6) // G2
		DISP_TX_DATA(data,5) // G1
		DISP_TX_DATA(data,4) // G0
		DISP_TX_DATA(data,8+3) // B3
		DISP_TX_DATA(data,8+2) // B2
		DISP_TX_DATA(data,8+1) // B1
		DISP_TX_DATA(data,8+0) // B0
	#else
		const W8 bitOrder[]={3,2,1,0, 7,6,5,4,8+3,8+2,8+1,8+0};
		for (W8 i=0; i< 12; i++)
		{
			DISP_TX_DATA(data,bitOrder[i]) 
		}
	#endif  
	  
	return;
  }
  txData:
	#ifdef DISP_UNROLL_LOOPS
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
	#else
		for (W8 i=0; i< 12; i++)
		{
			DISP_TX
		}
	#endif
}


W0 disp_txRepColorWhite(W16 len_, W0* buf_)
{
  PIN_HI(PIN_DISP_DC)
  PIN_LO(PIN_DISP_CS)
  PIN_HI(PIN_DISP_OUT)
  while(len_)
  {
	#ifdef DISP_UNROLL_LOOPS
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
	#else
	  
		for (W8 i=0; i< 12; i++)
		{
			DISP_TX
		}
	#endif
	len_--;
  }
  PIN_HI(PIN_DISP_CS)
}
W0 disp_txRepColorBlack(W16 len_, W0* buf_)
{
  PIN_HI(PIN_DISP_DC)
  PIN_LO(PIN_DISP_CS)
  PIN_LO(PIN_DISP_OUT)
  while(len_)
  {
	#ifdef DISP_UNROLL_LOOPS
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
		DISP_TX
	#else
		for (W8 i=0; i< 12; i++)
		{
			DISP_TX
		}
	#endif
	len_--;
  }
  PIN_HI(PIN_DISP_CS)
}

//#define disp_txRepColorBlack disp_txRepColor
//#define disp_txRepColorWhite disp_txRepColor

//W16 len_:numero de repeticoes
W0 disp_txRepColor(W16 len_, W0* buf_)
{
  const W8 data0 = ((W8*)(buf_))[0];
  const W8 data1 = ((W8*)(buf_))[1];
  PIN_HI(PIN_DISP_DC)
  PIN_LO(PIN_DISP_CS)
  while(len_)
  {
	#ifdef DISP_UNROLL_LOOPS


			DISP_TX_DATA(data0,3) // R3
			DISP_TX_DATA(data0,2) // R2
			DISP_TX_DATA(data0,1) // R1
			DISP_TX_DATA(data0,0) // R0

			DISP_TX_DATA(data0,7) // G3
			DISP_TX_DATA(data0,6) // G2
			DISP_TX_DATA(data0,5) // G1
			DISP_TX_DATA(data0,4) // G0

			DISP_TX_DATA(data1,3) // B3
			DISP_TX_DATA(data1,2) // B2
			DISP_TX_DATA(data1,1) // B1
			DISP_TX_DATA(data1,0) // B0


	#else
	  
		const W8 bitOrder[]={3,2,1,0,7,6,5,4};	
		for (W8 i=0; i< 8; i++)
		{
			DISP_TX_DATA(data0,bitOrder[i]) 
		}
		for (SW8 i=3; i>-1; i--)
		{
			DISP_TX_DATA(data1,i) 
		}
	#endif

	len_--;
  }
  PIN_HI(PIN_DISP_CS)
}

typedef W0(*DISP_TX_REP_FUNCTION)(W16, W0*);
DISP_TX_REP_FUNCTION disp_rep_function = disp_txRepColor;


W0 disp_tx_slow(W8 len_, W0* buf__){

  W8* buf_ = (W8*) buf__;
  PIN_LO(PIN_DISP_CS)
  while(len_)
  {
     const W8 data = *buf_;
	// #ifdef DISP_UNROLL_LOOPS
		 DISP_TX_DATA_SLOW(data,7)
		 DISP_TX_DATA_SLOW(data,6)
		 DISP_TX_DATA_SLOW(data,5)
		 DISP_TX_DATA_SLOW(data,4)
		 DISP_TX_DATA_SLOW(data,3)
		 DISP_TX_DATA_SLOW(data,2)
		 DISP_TX_DATA_SLOW(data,1)
		 DISP_TX_DATA_SLOW(data,0)
	//else
	//	for (SW8 i=7; i>-1; i--) <--- NAO FUNCIONA
	//	{
	//		 DISP_TX_DATA(data,i)
	//	}
	//endif
     buf_++;
     len_--;
  }
  PIN_HI(PIN_DISP_CS)
}
W0 disp_tx(W8 len_, W0* buf__){

  W8* buf_ = (W8*) buf__;
  PIN_LO(PIN_DISP_CS)
  while(len_)
  {
     const W8 data = *buf_;
	// #ifdef DISP_UNROLL_LOOPS
		 DISP_TX_DATA(data,7)
		 DISP_TX_DATA(data,6)
		 DISP_TX_DATA(data,5)
		 DISP_TX_DATA(data,4)
		 DISP_TX_DATA(data,3)
		 DISP_TX_DATA(data,2)
		 DISP_TX_DATA(data,1)
		 DISP_TX_DATA(data,0)
	//else
	//	for (SW8 i=7; i>-1; i--) <--- NAO FUNCIONA
	//	{
	//		 DISP_TX_DATA(data,i)
	//	}
	//endif
     buf_++;
     len_--;
  }
  PIN_HI(PIN_DISP_CS)
}

W0 disp_txCmd(W8 len_, W0* buf_)
{
  PIN_LO(PIN_DISP_DC)
  disp_tx(1,buf_);
}

W0 disp_txData(W8 len_, W0* buf_)
{
  PIN_HI(PIN_DISP_DC)
  disp_tx(len_,buf_);
}
W0 disp_txCmdData(W8 len_, W0* buf_)
{
  PIN_LO(PIN_DISP_DC)
  disp_tx(1,buf_);
  PIN_HI(PIN_DISP_DC)
  if (--len_)
    disp_tx(len_,((W8*)buf_)+1);
}
W0 disp_txCmdData_slow(W8 len_, W0* buf_)
{
  PIN_LO(PIN_DISP_DC)
  disp_tx_slow(1,buf_);
  PIN_HI(PIN_DISP_DC)
  if (--len_)
    disp_tx_slow(len_,((W8*)buf_)+1);
}

// tabela do github - autor "bersch" 
typedef enum {
    DISP_CMD_NOP       = 0x00,
    DISP_CMD_SWRESET   = 0x01, /* Software Reset */
    DISP_CMD_RDDID     = 0x04, /* Read Display ID */
    DISP_CMD_RDDST     = 0x09, /* Read Display Status */
    DISP_CMD_RDDPM     = 0x0a, /* Read Display Power Mode */
    DISP_CMD_RDDMADCTL = 0x0b, /* Read Display MADCTL */
    DISP_CMD_RDDCOLMOD = 0x0c, /* Read Display Pixel Format */
    DISP_CMD_RDDIM     = 0x0d, /* Read Display Image Mode */
    DISP_CMD_RDDSM     = 0x0e, /* Read Display Signal Mode */
    DISP_CMD_RDDSDR    = 0x0f, /* Read Display Self-Diagnostic Result */
    DISP_CMD_SLPIN     = 0x10, /* Sleep In */
    DISP_CMD_SLPOUT    = 0x11, /* Sleep Out */
    DISP_CMD_PTLON     = 0x12, /* Partial Display Mode On */
    DISP_CMD_NORON     = 0x13, /* Normal Display Mode On */
    DISP_CMD_INVOFF    = 0x20, /* Display Inversion Off */
    DISP_CMD_INVON     = 0x21, /* Display Inversion On */
    DISP_CMD_GAMSET    = 0x26, /* Gamma Set */
    DISP_CMD_DISPOFF   = 0x28, /* Display Off */
    DISP_CMD_DISPON    = 0x29, /* Display On */
    DISP_CMD_CASET     = 0x2a, /* Column Address Set */
    DISP_CMD_RASET     = 0x2b, /* Row Address Set */
    DISP_CMD_RAMWR     = 0x2c, /* Memory Write */
    DISP_CMD_RGBSET    = 0x2d, /* Color Setting 4k, 65k, 262k */
    DISP_CMD_RAMRD     = 0x2e, /* Memory Read */
    DISP_CMD_PTLAR     = 0x30, /* Partial Area */
    DISP_CMD_SCRLAR    = 0x33, /* Scroll Area Set */
    DISP_CMD_TEOFF     = 0x34, /* Tearing Effect Line OFF */
    DISP_CMD_TEON      = 0x35, /* Tearing Effect Line ON */
    DISP_CMD_MADCTL    = 0x36, /* Memory Data Access Control */
    DISP_CMD_VSCSAD    = 0x37, /* Vertical Scroll Start Address of RAM */
    DISP_CMD_IDMOFF    = 0x38, /* Idle Mode Off */
    DISP_CMD_IDMON     = 0x39, /* Idle Mode On */
    DISP_CMD_COLMOD    = 0x3a, /* Interface Pixel Format */
    DISP_CMD_RDID1     = 0xda, /* Read ID1 Value */
    DISP_CMD_RDID2     = 0xdb, /* Read ID2 Value */
    DISP_CMD_RDID3     = 0xdc, /* Read ID3 Value */
    DISP_CMD_FRMCTR1   = 0xb1, /* Frame Rate Control in normal mode, full colors */
    DISP_CMD_FRMCTR2   = 0xb2, /* Frame Rate Control in idle mode, 8 colors */
    DISP_CMD_FRMCTR3   = 0xb3, /* Frame Rate Control in partial mode, full colors */
    DISP_CMD_INVCTR    = 0xb4, /* Display Inversion Control */
    DISP_CMD_PWCTR1    = 0xc0, /* Power Control 1 */
    DISP_CMD_PWCTR2    = 0xc1, /* Power Control 2 */
    DISP_CMD_PWCTR3    = 0xc2, /* Power Control 3 in normal mode, full colors */
    DISP_CMD_PWCTR4    = 0xc3, /* Power Control 4 in idle mode 8colors */
    DISP_CMD_PWCTR5    = 0xc4, /* Power Control 5 in partial mode, full colors */
    DISP_CMD_VMCTR1    = 0xc5, /* VCOM Control 1 */
    DISP_CMD_VMOFCTR   = 0xc7, /* VCOM Offset Control */
    DISP_CMD_WRID2     = 0xd1, /* Write ID2 Value */
    DISP_CMD_WRID3     = 0xd2, /* Write ID3 Value */
    DISP_CMD_NVFCTR1   = 0xd9, /* NVM Control Status */
    DISP_CMD_NVFCTR2   = 0xde, /* NVM Read Command */
    DISP_CMD_NVFCTR3   = 0xdf, /* NVM Write Command */
    DISP_CMD_GMCTRP1   = 0xe0, /* Gamma '+'Polarity Correction Characteristics Setting */
    DISP_CMD_GMCTRN1   = 0xe1, /* Gamma '-'Polarity Correction Characteristics Setting */
    DISP_CMD_GCV       = 0xfc, /* Gate Pump Clock Frequency Variable */
} ST7735S_Command;



	

//usar draw_init em vez de display_init!
W0 display_init(){ 
	
	// ligar o pino de BACKLIGHT;  RES  a -[1K]--VCC !!!!
	//PIN_INIT(PIN_DISP_RES)
	PIN_INIT(PIN_DISP_CS)
	PIN_INIT(PIN_DISP_OUT)
	PIN_INIT(PIN_DISP_CLK)
	PIN_INIT(PIN_DISP_DC)

	//PIN_MODE(PIN_DISP_RES,   PIN_MODE_OUTPUT)
	PIN_MODE(PIN_DISP_CS,    PIN_MODE_OUTPUT)
	PIN_MODE(PIN_DISP_OUT,   PIN_MODE_OUTPUT)
	PIN_MODE(PIN_DISP_CLK,   PIN_MODE_OUTPUT)
	PIN_MODE(PIN_DISP_DC,    PIN_MODE_OUTPUT)
	DELAY_MS(2);
	
    PIN_DISP_CLK_LOW
    PIN_WRITE(PIN_DISP_CS,  LOW);
	//PIN_WRITE(PIN_DISP_RES, LOW);

	#ifdef DEBUG_PINS_DISP
		TEST_PINS(DISP_PINS_DEF)
	#endif

									// Pin_BLK_Pct(100);
								    //   hard reset 	
	//PIN_WRITE(PIN_DISP_RES, HIGH); //  Pin_RES_High();
	PIN_WRITE(PIN_DISP_CS,  LOW);  //  Pin_CS_Low();
	PIN_WRITE(PIN_DISP_DC,  LOW);  //  Pin_DC_Low();
    DELAY_MS(20);					   //  Delay(2); 10µs min 
	//PIN_WRITE(PIN_DISP_RES, LOW);  //  Pin_RES_Low();
    DELAY_MS(20);						 //  Delay(2);  10µs min 
	//PIN_WRITE(PIN_DISP_RES, HIGH); //  Pin_RES_High();
    DELAY_MS(2);					   //  Delay(2);
	PIN_WRITE(PIN_DISP_CS,  HIGH); //


	PIN_WRITE(PIN_DISP_CS,  HIGH);
	//PIN_WRITE(PIN_DISP_RES, HIGH);
	DELAY_MS(5);
	
	//PIN_WRITE(PIN_DISP_RES, LOW);
	DELAY_MS(5);
	//PIN_WRITE(PIN_DISP_RES, HIGH);
	
	PIN_WRITE(PIN_DISP_CS,  HIGH);
	DELAY_MS(5);
	PIN_WRITE(PIN_DISP_CS,  LOW);


/* columns: 1 = # of params, 2 = command, 3 .. = params */
W8 disp_init_cmds[] = {
    1, DISP_CMD_SWRESET, /* software reset */
    1,  DISP_CMD_SLPOUT, /* sleep out, turn off sleep mode */
	1, DISP_CMD_DISPOFF,  /*  output from frame mem disabled */
    4, DISP_CMD_FRMCTR1, 0x00, 0b111111, 0b111111, /* frame frequency normal mode (highest frame rate in normal mode) */
    4, DISP_CMD_FRMCTR2, 0b1111, 0x01, 0x01, /* frame frequency idle mode */
    7, DISP_CMD_FRMCTR3, 0x05, 0x3c, 0x3c, 0x05, 0x3c, 0x3c,  /* frame freq partial mode: 1-3 dot inv, 4-6 col inv */
    2,  DISP_CMD_INVCTR, 0x03, /* display inversion control: 3-bit 0=dot, 1=col */
    4,  DISP_CMD_PWCTR1, 0b11111100, 0x08, 0b10, /* power control */
    2,  DISP_CMD_PWCTR2, 0xc0,
    3,  DISP_CMD_PWCTR3, 0x0d, 0x00,
    3,  DISP_CMD_PWCTR4, 0x8d, 0x2a,
    3,  DISP_CMD_PWCTR5, 0x8d, 0xee, /* partial */
	/* display brightness and gamma */
    2,     DISP_CMD_GCV, 0b11011000, /* auto gate pump freq, max power save */
    2, DISP_CMD_NVFCTR1, 0b01000000, /* automatic adjust gate pumping clock for saving power consumption */
	2,  DISP_CMD_VMCTR1, 0b001111,  /* VCOM voltage setting */
    2, DISP_CMD_VMOFCTR, 0b0000, /* ligthness of black color 0-0x1f */
    2,  DISP_CMD_GAMSET, 0x01, /* gamma 1, 2, 4, 8 */
    2,  DISP_CMD_MADCTL, 0b01101000, 
			/* row oder, col order, row colum xchange, vert refr order, 
			   [bit 3: 1:BGR/0:RGB], hor refr order, 0, 0 */

    2,  DISP_CMD_COLMOD, 0x03, /* 3=12bit, 5=16-bit, 6=18-bit  pixel color mode */
	17, DISP_CMD_GMCTRP1,0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2c,
						 0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10,
    17, DISP_CMD_GMCTRN1,0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2c,
						 0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10,
    5,   DISP_CMD_CASET, 0, 0, 0, DISP_WIDTH-1,
    5,   DISP_CMD_RASET, 0, 0, 0, DISP_HEIGHT-1,
    1,   DISP_CMD_INVOFF, /* display inversion on/off */
    1,  DISP_CMD_IDMOFF, /* idle mode off */
    1,   DISP_CMD_NORON,  /* normal display mode on */
    1,  DISP_CMD_DISPON   /* recover from display off, output from frame mem enabled */
};
	for(W16 i = 0, cmdLen; i < sizeof(disp_init_cmds); i+=cmdLen+1) 
	{
		cmdLen = disp_init_cmds[i];
		disp_txCmdData_slow(cmdLen, disp_init_cmds+i+1);
	}
	

	
}





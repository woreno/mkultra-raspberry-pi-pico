

// <!> falta confirmar CRC16
//    - alguns comandos podiam estar optimizados
//      porque CRC7 é gerado para comandos
//      quando os comandos são iguais e CRC7 
//      não muda

// 04/10/2026
// <!>  EXISTEM CARTÕES MAIS ANTIGOS QUE PODEM
//      NÃO FUNCIONAR ATENDER AO ENDEREÇAMENTO
//      QUE PODE MUDAR COM O TIPO DE CARTÃO!!!!!


//#define  SD_OPTIMIZE_O2

#define  SD_DEBUG  DEBUG
//#define  SD_USE_REVBIT_TABLE
#include "crc7.h"

// --------------------------------------------------------------------------
// SE NAO DEFINIDO SE USAR CRC16, ATIVAR POR DEFAULT
#ifndef SD_ENABLE_DATA_CRC
	#define SD_ENABLE_DATA_CRC 1
#endif

#if (SD_ENABLE_DATA_CRC==1)
	#include "crc16.h"
#endif
	
// --------------------------------------------------------------------------
// RX/TX SPI
// CLK deve oscilar a 100KHz-400KHz  2.5us-10us 
#ifndef SD_REPLY_CLK_TIMEOUT
#define SD_REPLY_CLK_TIMEOUT	 32
#endif


#ifndef SD_USES_CUSTOM_CLOCK
	#ifdef SD_USES_FAST_CLOCK
	//7
		#define SD_CLK_DELAY1			__asm volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n");
		#define SD_CLK_DELAY2			__asm volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n");
	//#define SD_CLK_DELAY1			__asm volatile ("nop\n nop\n nop\n ");
	//#define SD_CLK_DELAY2			__asm volatile ("nop\n nop\n nop\n ");

	#else
		#define SD_CLK_DELAY1			DELAY_US(2) 
		#define SD_CLK_DELAY2			DELAY_US(2) 
	#endif
#else
	#define SD_CLK_DELAY1 SD_USES_CUSTOM_CLOCK
	#define SD_CLK_DELAY2 SD_USES_CUSTOM_CLOCK
#endif
// --------------------------------------------------------------------------
// DEBUG 
// POSSIVEL DE ATIVAR LÁ FORA:
	//#define SD_DEBUG_R2
	//#define SD_DEBUG_R3
	//#define SD_DEBUG_R7
	//#define SD_DEBUG_CRC16
	//#define SD_DEBUG_CMD_STRS
	//#define SD_DEBUG_ERROR_STRS
	//#define SD_DEBUG_CMDS

// --------------------------------------------------------------------------
// INICIALIZACAO: PADRAO PARA O SDCARD FAZER ECO (VERIFICACAO)
#define SD_INIT_PATTERN 0xAA

// --------------------------------------------------------------------------
// DEFINIÇÕES DE ERRO
#ifndef ERRORS_DEF
#define ERRORS_DEF_SD(X_)\
  X_(SD,      INIT			    ,   0 )\
  X_(SD,      SOME_ERROR	    ,   1 )\
  X_(SD,      TIMEOUT			,   2 )\
  X_(SD,      CMD_APP_NOTDONE   ,   3 )\
  X_(SD,      CMD_APP_SEND_OP   ,   4 )\
  X_(SD,      TIMEOUT_CMD_APP   ,   5 )\
  X_(SD,      3_3V_NOT_SUPPORTED,   6 )\
  X_(SD,      INIT_PATTERN      ,   7 )\
  X_(SD,      READ_INVALID_TOKEN,   8 )\
  X_(SD,      READ_CRC16		,   9 )\
  X_(SD,      WRITE_CRC16		,   10 )\
  X_(SD,      WRITE_CARD		,   11 )\
  X_(SD,      WRITE_UNKNOWN		,   12 )
  enum SD_ERRORS { ERRORS_DEF_SD(ERR_MKENUM_)   };
  ERRORS_DEF_SD(ERR_MKSTRS1_)
  #ifdef DEBUG_SD_ERROR_STRS
	 const SW8 *const SD_ERROR_STRINGS[] VL_STORE_IN_FLASH = {  ERRORS_DEF_SD(ERR_MKSTRS2_) };  
  #endif
#endif
  
// --------------------------------------------------------------------------
// ESTADO DA RESPOSTA 1 E ESTADO NA INICIALIZACAO  
union
{
	struct { W8 pattern; };
	struct { W8 supports3_3V; };
} sd_init_checks;
W8 sd_lastR1reply;
 
// --------------------------------------------------------------------------
// CONSTANTES 
#define SD_DATA_TOKEN			0xFE
#define SD_R1_IS_READY(X_)		((X_&1)==1)			// = Idle (preparado)
#define SD_R1_IS_OK(X_)			(0==((X_)&0xFE))	// = Sem erros [1111'1110]

 
// --------------------------------------------------------------------------
// MACROS DE ERRO 
#define SD_EXEC_FAIL_IF(COND_, CODE_)\
	if (COND_) {  sd_unselect(); SD_PRINT_ERROR_CODE(CODE_, "\n "# COND_ "") return(CODE_); }

#define SD_INIT_FAIL_IF(COND_, CODE_)\
	SD_EXEC_FAIL_IF(COND_!=RES_OK, CODE_)



// --------------------------------------------------------------------------
// COMANDOS SD --------------------------------------------------------
//								 CMD  SHIFT	REPLYTYPE
//                                |   |      |
//                                V   V      V
#define SD_CMDS_DEF(X_)\
  X_(SD_CMD_GO_IDLE          , ((( 0<<4) |(0x1)))  )\
  X_(SD_CMD_SEND_OP_COND     , ((( 1<<4) |(0x1)))  )\
  X_(SD_CMD_SEND_IF_COND     , ((( 8<<4) |(0x7)))  )\
  X_(SD_CMD_SEND_CSD         , ((( 9<<4) |(0x1)))  )\
  X_(SD_CMD_SEND_CID         , (((10<<4) |(0x1)))  )\
  X_(SD_CMD_STOP_TX          , (((12<<4) |(0x2)))  )\
  X_(SD_CMD_STATUS           , (((13<<4) |(0x2)))  )\
  X_(SD_CMD_SET_BLKLEN       , (((16<<4) |(0x1)))  )\
  X_(SD_CMD_RD_BLK           , (((17<<4) |(0x1)))  )\
  X_(SD_CMD_RD_MBLK          , (((18<<4) |(0x1)))  )\
  X_(SD_CMD_WR_BLK           , (((24<<4) |(0x1)))  )\
  X_(SD_CMD_WR_MBLK          , (((25<<4) |(0x1)))  )\
  X_(SD_CMD_WR_CSD           , (((27<<4) |(0x1)))  )\
  X_(SD_CMD_SET_WR_PROT      , (((28<<4) |(0xB)))  )\
  X_(SD_CMD_SET_CLR_PROT     , (((29<<4) |(0x1)))  )\
  X_(SD_CMD_SEND_WR_PROT     , (((30<<4) |(0x1)))  )\
  X_(SD_CMD_LOCK             , (((42<<4) |(0x1)))  )\
  X_(SD_CMD_APP              , (((55<<4) |(0x1)))  )\
  X_(SD_CMD_APP_SEND_OP_COND , (((41<<4) |(0x3)))  )\
  X_(SD_CMD_GEN_CMD          , (((56<<4) |(0x1)))  )\
  X_(SD_CMD_RD_OCR           , (((58<<4) |(0x3)))  )\
  X_(SD_CMD_CRC_EN           , (((59<<4) |(0x1)))  )

// enums SD_CMD
VL_DEF_MKENUM(SD_CMD     , SD_CMDS_DEF) 
// strings
#ifdef SD_DEBUG_CMD_STRS
  VL_DEF_MKSTRS(SD_CMDStrs , SD_CMDS_DEF) 
#endif

#ifdef SD_DEBUG_REGISTERS
	const SW8* R1_bits_strs[] =  {"Idle", "EraseCancel", "Ilegal", "CRC", "EraseSeq", "Addr", "Param", 0};
	const SW8* R2_bits_strs[] =  {"Locked", "WrProtSkip/Unlock", "Err", "CRC", "ECC", "WrProt", "EraseParam", "OutRange/CSD" };
#endif
	
	// --------------------------------------------------------------------------
// DEBUG 
#ifdef SD_DEBUG_ERROR_STRS
	#define SD_PRINT_ERROR_CODE(CODE_, STRING_) sd_printError(CODE_, STRING_);
	W0 sd_printError(W8 code_, const SW8* str_=0)
	{
		printf("\n*SD** <!><!><!> "); 
		if (str_) printf("%s", str_);
		printf(" | %s", SD_ERROR_STRINGS[code_]); 
		printf("\nR1:");
		sprnBits(mcuSharedBuffer, R1_bits_strs, sd_lastR1reply, true);
		DEBUG_PRINT(mcuSharedBuffer);
	}
#else
	#define SD_PRINT_ERROR_CODE(CODE_, STRING_)
#endif



W8 sd_cmd_execute(SD_CMD cmdr_, W32 args_=0, W8* buffer_=0);


W1 sd_rxBytes(W16 bytesIn_, W8* buf_, W1 wait_=0){

  PIN_WRITE(PIN_SD_OUT, 1);   
  #ifdef SD_DEBUG_DATA  
   SD_DEBUG("\nRecebo %uB\n", bytesIn_)
  #endif
 
  bytesIn_--;
  W8 bitsIn_ = 8;
  SW16 its = SD_REPLY_CLK_TIMEOUT;
  if (wait_)
  {
      W1 biti;
      for (;its>-1;its--)
      {
        PIN_HI1(PIN_SD_CLK)
        SD_CLK_DELAY1
        biti = PIN_READ(PIN_SD_IN)==HIGH;  
        PIN_LO1(PIN_SD_CLK)
        SD_CLK_DELAY2
        if (!biti) break;
      }
      if (biti)
      {
        SD_DEBUG("\n***No reply aft %u clks",SD_REPLY_CLK_TIMEOUT)
        return RES_FAIL;
      }
      #ifdef SD_DEBUG_DATABITS
        SD_DEBUG("0",0)
      #endif
      bitsIn_--;
    }
  for (SW16 i=bytesIn_; i>-1; i--)
  {
    W8 data = 0;
    for (SW16 j=bitsIn_-1; j>-1; j--)
    {
      PIN_HI1(PIN_SD_CLK)
      SD_CLK_DELAY1
      
      const W1 biti = PIN_READ(PIN_SD_IN)==HIGH;  
      data|= biti<<j;
      PIN_LO1(PIN_SD_CLK)
      SD_CLK_DELAY2
    }
    buf_[i]=data;
    #ifdef SD_DEBUG_DATABITS
      SD_DEBUG("`",0)
    #endif
    bitsIn_ = 8;
  }

  #ifdef SD_DEBUG_REPLY_TIME
    if (wait_)
    {
      SD_DEBUG("\n demorou %u clks", SD_REPLY_CLK_TIMEOUT-its);
    }
  #endif
  return RES_OK;
}

#ifdef SD_USE_REVBIT_TABLE
	#define SD_SEND_BIT(X_)						\
		  PIN_WRITE(PIN_SD_OUT,data&1);			\
		  PIN_HI1(PIN_SD_CLK)					\
		  SD_CLK_DELAY1							\
		  PIN_LO1(PIN_SD_CLK)					\
		  SD_CLK_DELAY2							\
		  data>>=1;
#else

#if (APP_TARGET==APP_TARGET_ARDUINO_NANO)
	#define SD_SEND_BIT(X_)	\
		PIN_WRITE(PIN_SD_OUT,  (data>>(7-X_))&1 ); \
		PIN_HI1(PIN_SD_CLK)						\
		SD_CLK_DELAY1							\
		PIN_LO1(PIN_SD_CLK)						\
		SD_CLK_DELAY2						
#else

	#define SD_SEND_BIT(X_)	\
		PIN_WRITE(PIN_SD_OUT,  (data>>(7-X_))&1 ); \
		PIN_HI1(PIN_SD_CLK)						   \
		PIN_LO1(PIN_SD_CLK)						

#endif

#endif

W0 sd_txBytes(W16 bytesOut_, W8* buf_=0)
{ 
  while(bytesOut_)
  {
	#ifdef SD_USE_REVBIT_TABLE
		W8 data = revBits[*buf_];
	#else
		const W8 data = *buf_;
	#endif
	SD_SEND_BIT(0)
	SD_SEND_BIT(1)
	SD_SEND_BIT(2)
	SD_SEND_BIT(3)
	SD_SEND_BIT(4)
	SD_SEND_BIT(5)
	SD_SEND_BIT(6)
	SD_SEND_BIT(7)
	buf_++;
	bytesOut_--;
  }
}

W0 sd_txBytes0(W16 bytesOut_)
{ 
	W8 b=0;
	while(bytesOut_)
	{
		sd_txBytes(1,&b);
		bytesOut_--;
	}
}

W0 sd_select()
{
	PIN_LO1(PIN_SD_CS)
}

W0 sd_unselect()
{
	W8 sd_fillByte=0xFF;
	sd_txBytes(1,&sd_fillByte); 
	PIN_HI1(PIN_SD_CS)
	sd_txBytes(1,&sd_fillByte); 
}
W0 sd_wait()
{
	W8 sd_fillByte=0xFF;
	W8 byte;
	do
	{
		sd_txBytes(1,&sd_fillByte); 
		sd_rxBytes(1,&byte);
		if (byte==0xFF) break;
	}while(1);
}
W8 sd_rxDataToken(){
  
  W16 tries = 0xFFFF;
  W8  rxbyte;
  do 
  {
    sd_rxBytes(1,&rxbyte);
	if (rxbyte==SD_DATA_TOKEN) goto skip;
    if (--tries==0) break;
  }while(rxbyte==0xFF);
  
	skip:
  return rxbyte;
} 

W8 sd_cmd_execute(SD_CMD cmdr_, W32 args_, W8* buffer_){
	
	W8 buf[6];
	const W8 cmd_       =  cmdr_>>4;
	const W8 replyType  =  cmdr_&0xF;
	const W8 *args      = (W8*)&args_;
  
	sd_select();
	sd_wait();

	// calcular o CRC16 do buffer de antemao caso escrita
	#if (SD_ENABLE_DATA_CRC==1)
		W16 CRC16_computed=  
			(cmdr_==SD_CMD_WR_BLK)? crc16(buffer_, 512) : (0); 
	#endif

	#ifdef SD_DEBUG_CMD_STRS
		const SW8 *cmdStr = searchString(cmdr_>>4, SD_CMDStrs, SD_CMDStrsArr);
	#endif
   
	buf[0] =  (W8)(1<<6) | (W8)cmd_; 
	if (args_==0)
	{
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
	}
	else
	{
		buf[4] = args[0];
		buf[3] = args[1];
		buf[2] = args[2];
		buf[1] = args[3];
	}
	buf[5] = (crc7(buf, 5)<<1) | 1;

	#ifdef SD_DEBUG_CMDS
	if (cmdr_!=SD_CMD_STATUS)
	{
		SD_DEBUG("\n\n*** %02u %s [CRC:0x%02X 0x%02X%02X%02X%02X%02X%02X R%X]",
				cmd_,   cmdStr, buf[5]>>1, buf[5], buf[4], 
				buf[3], buf[2], buf[1], buf[0], replyType)
	}
	#endif

	// enviar o comando
	sd_txBytes(6,buf);

	// tentar receber consoante o tipo de resposta
	// falhar logo se existir algum erro
	if (replyType==0xB)
	{}
	else
	{ 
		SD_EXEC_FAIL_IF(sd_rxBytes(1, buf, 1), ERROR_SD_TIMEOUT) 
		sd_lastR1reply = buf[0];

		// 
		//  |
		//  V 
		// 
		SD_EXEC_FAIL_IF(!SD_R1_IS_OK(sd_lastR1reply), ERROR_SD_SOME_ERROR)
		
		//if (!((0==((sd_lastR1reply)&0xFE))))
		//{
		//	printf("\nERR R1:%X", sd_lastR1reply);
		//	return	ERROR_SD_SOME_ERROR;
		//}
		
		const W16 extraBytes = replyType==0x2? 1: 4;
		SD_EXEC_FAIL_IF(sd_rxBytes(extraBytes,buf), ERROR_SD_TIMEOUT) 
	}

	// se o comando tiver mais, continuar
	switch (cmdr_)
	{
		case SD_CMD_RD_BLK:
		{
			// 1. data token (RX) -> 2. data block (RX) -> 3. CRC (RX)
			W8 dataToken = sd_rxDataToken();
			if (dataToken != SD_DATA_TOKEN) // 1. data token
			{
				DEBUG("\nRX: token %X!",dataToken);
			}    
			
			for (W16 i=0; i<512; i++) // 2. data block
				sd_rxBytes(1,buffer_+i);   
       
			sd_rxBytes(2,buf); // 3.  CRC
		
			#if (SD_ENABLE_DATA_CRC==1)
				const W16 CRC16_fromSD = *((W16*)buf);
				CRC16_computed = crc16(buffer_, 512);		
				#ifdef SD_DEBUG_CRC16
					printf("\nCRC: 0x%04X obtive: 0x%04X", CRC16_fromSD, CRC16_computed);
				#endif
				SD_EXEC_FAIL_IF(CRC16_fromSD!=CRC16_computed, ERROR_SD_READ_CRC16)
			#endif
		}
		break;
		case SD_CMD_WR_BLK:
		{
			// 1. data token (TX) -> 2. data block (TX) -> 3. CRC (TX) -> 4. R1 (RX)
			W1 rxStatus;
			W8 dataResponse, dataToken = SD_DATA_TOKEN;

			sd_txBytes(1, &dataToken); // 1. data token

			if (buffer_)
				sd_txBytes(512, buffer_);   // 2. data block
			else
				sd_txBytes0(512);
		
			#if (SD_ENABLE_DATA_CRC==1)
				CRC16_computed = (CRC16_computed<<8) | (CRC16_computed>>8);
				sd_txBytes(2, (W8*)(&CRC16_computed)); // 3. CRC
			#endif

			rxStatus = sd_rxBytes(1,&dataResponse); // 4. data response
			
			#if (SD_ENABLE_DATA_CRC==1)
				#ifdef SD_DEBUG_CRC16
					printf("\nCRC: 0x%04X accepted:%X", crc16_toTx, byte);
				#endif
				dataResponse&=0x1F;
				if (dataResponse!=5)
				{
					SD_EXEC_FAIL_IF(dataResponse==0xB, ERROR_SD_WRITE_CRC16)
					SD_EXEC_FAIL_IF(dataResponse==0xD, ERROR_SD_WRITE_CARD)
					SD_EXEC_FAIL_IF(dataResponse!=5,   ERROR_SD_WRITE_UNKNOWN)
				}
			#endif

			//sd_wait();
		}
		break;
	}

	SD_EXEC_FAIL_IF(!SD_R1_IS_OK(sd_lastR1reply), ERROR_SD_SOME_ERROR)
	
	// imprimir restante resposta
	switch(replyType)
	{
		case 0x2:  
		{
			#ifdef SD_DEBUG_R2
				sprnBits(mcuSharedBuffer, R2_bits_strs, buf[0]);
				DEBUG_PRINT(mcuSharedBuffer);
			#endif
		}
		break;
		case 0x3:  
		{
			const W32 b = ((W32*)buf)[0];
			sd_init_checks.supports3_3V =  GET_BIT(b,21);
			#ifdef SD_DEBUG_R3
			DEBUG_PRINT("\nTensoes: ");
              
			for (W16 i=15; i<24; i++)
				if (GET_BIT(b,i)) { DEBUG("%i ", 27+i-15); }
              
			DEBUG_PRINT("\nOutros: ");
			if (GET_BIT(b,24)) { DEBUG_PRINT("1.8V aceite|"); }
			if (GET_BIT(b,29)) { DEBUG_PRINT("UHS-II Status|"); }
			if (GET_BIT(b,30)) { DEBUG_PRINT("SDHC (hi cap)|");}
			else  { DEBUG_PRINT("SDSC (std cap)|"); }
			if (GET_BIT(b,31)) { DEBUG_PRINT("PowerUp Stats|"); }
			if (sd_init_checks.supports3_3V) { DEBUG_PRINT("3.3 aceite"); }
			#endif
		}
		break;
		case 0x7:  
		{
			const W32 b = ((W32*)buf)[0];
			sd_init_checks.pattern = GET_BITS(b, 0, 7);
			#ifdef SD_DEBUG_R7
				DEBUG("\necho: 0x%02X",             sd_init_checks.pattern );
				DEBUG("\ntensao aceite: 0x%02X",    GET_BITS(b, 8, 11));
				DEBUG("\nversao: 0x%02X",			GET_BITS(b, 28, 31));
			#endif
		}
		break;
	}
  
  sd_unselect(); 
  return RES_OK;
}
W1 sd_rd_block(W32 blk_, W0* buffer_)
{
	//printf("\n READ BLOCK :%i (BYTE: 0x%X)", blk_, blk_*512);
	return sd_cmd_execute(SD_CMD_RD_BLK, blk_, (W8*)buffer_);
}
W1 sd_wr_block(W32 blk_, W0* buffer_)
{
	return sd_cmd_execute(SD_CMD_WR_BLK, blk_, (W8*)buffer_);
}

W0 sd_init_pins_()
{
	INIT_SECTION(SD)
	PIN_INIT(PIN_SD_CS)
	PIN_INIT(PIN_SD_OUT)
	PIN_INIT(PIN_SD_CLK)
	PIN_INIT(PIN_SD_IN)

	PIN_MODE(PIN_SD_CS,   PIN_MODE_OUTPUT)
	PIN_MODE(PIN_SD_OUT,  PIN_MODE_OUTPUT)
	PIN_MODE(PIN_SD_CLK,  PIN_MODE_OUTPUT)
	PIN_MODE(PIN_SD_IN,   PIN_MODE_INPUT)
  
	PIN_LO1(PIN_SD_OUT)
	PIN_HI1(PIN_SD_CS)
	PIN_LO1(PIN_SD_CLK)

	#ifdef DEBUG_PINS_SD
		TEST_PINS(SD_PINS_DEF)
	#endif
	
	DELAY_MS(200);
	  
	PIN_HI1(PIN_SD_CS)					
	for (W16 i=0; i< 80; i++)		
	{								
		PIN_HI1(PIN_SD_CLK)			
		SD_CLK_DELAY1					
		PIN_LO1(PIN_SD_CLK)			
		SD_CLK_DELAY2					
	}
	
	PIN_LO1(PIN_SD_OUT)
	PIN_LO1(PIN_SD_CS)
	PIN_LO1(PIN_SD_CLK)
}
W8 sd_init(){ 
	 
	
	sd_init_pins_();
	
	W16 tries=3;
	while(tries)
	{
		if (RES_OK == sd_cmd_execute(SD_CMD_GO_IDLE)) goto sd_init_continue;
		DELAY_MS(200);
		tries--;
	}
	return RES_FAIL;
	
	sd_init_continue:

	SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_SEND_IF_COND, (1<<8)|SD_INIT_PATTERN), ERROR_SD_INIT)
		SD_EXEC_FAIL_IF(sd_init_checks.pattern!=SD_INIT_PATTERN, ERROR_SD_INIT_PATTERN) 
	
	SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_CRC_EN, SD_ENABLE_DATA_CRC), ERROR_SD_INIT)
	SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_RD_OCR), ERROR_SD_INIT)
	
	tries=0xFFFF;
	do
	{
		W16 tries2=0xFFFF;
		SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_APP), ERROR_SD_CMD_APP_NOTDONE)
		SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_APP_SEND_OP_COND, 0x40000000), ERROR_SD_CMD_APP_SEND_OP)
		if (sd_lastR1reply == 0 ) break;
		if (tries%1024==0) DEBUG_PRINT(F("."));
	} while(--tries);
	
	SD_EXEC_FAIL_IF((tries==0), ERROR_SD_TIMEOUT_CMD_APP);

	SD_EXEC_FAIL_IF(sd_init_checks.supports3_3V!=1, ERROR_SD_3_3V_NOT_SUPPORTED) 

	//(SD_CMD_SEND_CSD)
	//(SD_CMD_SEND_CID)
	SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_SET_BLKLEN, 512), ERROR_SD_INIT)
	SD_INIT_FAIL_IF(sd_cmd_execute(SD_CMD_CRC_EN, SD_ENABLE_DATA_CRC), ERROR_SD_INIT)

	
	return RES_OK;  
}


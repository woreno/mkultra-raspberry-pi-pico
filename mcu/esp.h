#include "esp_config.h"
// atenção a velocidade max. 115200 bauds/s
// 14.0625Kbps (sem compressao)
//
// se a baudrate for 115207 ou assim estão mal ligados!
// !!! ATENÇÃO LIGAR... O PINO DO MCU "RX" ao "TX" do ESP
//                      O PINO DO MCU "TX" ao "RX" do ESP

#define ESP_USE_EXTERNAL_BUFFER				mcuSharedBuffer
#define ESP_USES_EXTERNAL_RXTXBUFFER		mcuSharedBuffer
SW8	*esp_LOCAL_IP = 0;
W1  espRes;

//#define DEBUG_ESP_CMD
//#define DEBUG_ESP_INCOMING
//#define DEBUG_ESP_OKMSGS
//#define DEBUG_ESP_RX
//#define DEBUG_ESP_FRX
//#define ESP_USE_PASSIVEMODE
//#define DEBUG_RECEIVING
//#define DEBUG_SENDING
#define DEBUG_ESP(X)				   // {printf("\n");printf(X);}
#define DEBUG_ESP_STATUS(X_, ...)		//{printf("\nESP>"); printf(X_,__VA_ARGS__);}

#define ESP_SENDCMD(OK_STR_, ...)	\
	espRes = esp_cmd(OK_STR_, __VA_ARGS__); \
	if (espRes) goto error;

#define ESP_EXEC(X_) if (X_) { goto error; }

const SW8 esp_okConfirmStr[]		VL_STORE_IN_FLASH = "OK\r\n";
const SW8 esp_readyConfirmStr[]		VL_STORE_IN_FLASH = "ready\r\n";

enum ESP_STATUS { 
	ESP_STATUS_NOT_INIT							= 0, 
	ESP_STATUS_INIT_NO_WIFI						= 1, 
	ESP_STATUS_WIFI_AND_IP						= 2	, 
	ESP_STATUS_TCP								= 3	, 
	ESP_STATUS_ALL_DISCONNECTED					= 4	, 
	ESP_STATUS_STARTED_WIFI_BUT_NOT_CONNECTED	= 5 ,
	ESP_STATUS_UNKNOWN							= 0xFF
};
ESP_STATUS  espStatus = ESP_STATUS_UNKNOWN;

enum ESP_CMDTYPE { ESP_CMDTYPE_NORMAL , ESP_CMDTYPE_IP,  ESP_CMDTYPE_STATUS, ESP_CMDTYPE_SSIDS, ESP_CMDTYPE_CIPSTART };

W1 esp_cmd( const SW8*, const SW8*, ESP_CMDTYPE);

W0 esp_clearRx()
{
	while (uart_is_readable(ESP_UART))
	{
		const W8 b = uart_getc(ESP_UART);
		DELAY_MS(1);
	}
}
W1 esp_strCharCmp(W8 b, const SW8* str_, W8& it_, const W8 strSz_)
{
	if (b==str_[it_])
	{
		if (++it_==strSz_)  return 1;
	}
	else
		it_ = 0;
	return 0;
}
W1 esp_getResponse(const SW8* okStr_, ESP_CMDTYPE type_=ESP_CMDTYPE_NORMAL)
{
	const SW8 esp_errorStr[] = "ERROR\r\n";
	const SW8 esp_failStr[] = "FAIL\r\n";

	SW8 *rxBuffer, *rxb; 
	const W8  okStrSz  = strlen(okStr_);
	W8 okStrIt=0, errorStrIt = 0, failStrIt=0;
	W1 res = 0;

	const W1 responseIsLong =  (type_==ESP_CMDTYPE_STATUS) || (type_==ESP_CMDTYPE_IP);

	if (responseIsLong)
	{
		#ifdef ESP_USE_EXTERNAL_BUFFER 
			rxBuffer = (SW8*)ESP_USE_EXTERNAL_BUFFER;
		#else
			rxBuffer = new SW8[128];
		#endif
		rxb = rxBuffer;
	}
	while(1)
	{ 
		while (!uart_is_readable(ESP_UART)) { tight_loop_contents(); }
		const W8  b = uart_getc(ESP_UART);
		#ifdef DEBUG_ESP_INCOMING
			printf("%c", b );
		#endif
		if (esp_strCharCmp(b, okStr_, okStrIt, okStrSz))
			goto skip;
		if (esp_strCharCmp(b, esp_errorStr, errorStrIt, 5))
			goto error;
		if (esp_strCharCmp(b, esp_failStr, failStrIt, 4))
			goto error;
		if (responseIsLong) 
		{
			*rxb = b; 
			rxb++;
		}
	}
	goto error;
	skip:
	if (responseIsLong)
	{
		DEBUG_ESP_STATUS("RESP:%ibytes", rxb-rxBuffer);
		*rxb=0;
		SW8 *loc, *loc2;
		switch (type_)
		{	
			case ESP_CMDTYPE_STATUS:
			{
				if (0==(loc = strstr((const SW8*)rxBuffer,"STATUS:")))
				{	
					DEBUG_ESP_STATUS("stat:inv", 0);
					espStatus = ESP_STATUS_UNKNOWN;
					goto error;
				}
				espStatus = (ESP_STATUS)(loc[8]-'0');
				DEBUG_ESP_STATUS("status:%i", (W8)espStatus);
			}
			break;
			case ESP_CMDTYPE_IP:
			{ 
				if (0==(loc = strstr((const SW8*)rxBuffer,"+CIFSR:STAIP,")))
				{
					DEBUG_ESP_STATUS("ip", 0);
					goto error;
				}

				loc2 = strchr(loc+14, '"');
				if (loc2)
				{
					loc2[0]=0;
					esp_LOCAL_IP = new SW8[20];
					strcpy(esp_LOCAL_IP, loc+14);
					DEBUG_ESP_STATUS("ip:[%s]", esp_LOCAL_IP);
					goto success;
				}
			}
			break;
		}
	}
	goto success;
error:
	res = 1;
success:
	#ifdef DEBUG_ESP_OKMSGS
		if (res)
		{
			DEBUG_ESP(" <-- [FAIL]");
		}
		else
		{
			DEBUG_ESP(" <-- [OK]");
		}
	#endif
		
	#ifndef ESP_USE_EXTERNAL_BUFFER 
		if (responseIsLong) delete[] rxBuffer;
	#endif
	return res;
} 
W1 esp_cmd( const SW8* confirmStr_, const SW8* cmd_, ESP_CMDTYPE type_=ESP_CMDTYPE_NORMAL )
{ 

	#ifdef DEBUG_ESP_CMD
		printf("\n\n---------- AT%s\n", cmd_);
	#endif

	while (!uart_is_writable(ESP_UART)) { tight_loop_contents(); }
	uart_write_blocking(ESP_UART, (W8*)"AT", 2);
	uart_write_blocking(ESP_UART, (W8*)cmd_, strlen(cmd_));
	uart_write_blocking(ESP_UART, (W8*)"\r\n", 2);
	return esp_getResponse(confirmStr_, type_);
} 
//
W1 esp_send(W0* data_, W32 dataSize_)
{ 
	while (!uart_is_writable(ESP_UART)) { tight_loop_contents(); }
	
	if (dataSize_==1)
		uart_write_blocking(ESP_UART, (W8*)"AT+CIPSEND=1\r\n", 14);
	else
	{
		sprintf(mcuSharedBuffer, "AT+CIPSEND=%u\r\n", dataSize_);
		uart_write_blocking(ESP_UART, (W8*)mcuSharedBuffer, strlen(mcuSharedBuffer));
	}
	while (!uart_is_readable(ESP_UART)) { tight_loop_contents(); }
	while(1)
	{
		if (uart_getc(ESP_UART)=='>') break;
	}
	uart_write_blocking(ESP_UART, (W8*)data_, dataSize_);
	W8 okIt=0;
	while(1)
	{
		if ( esp_strCharCmp(uart_getc(ESP_UART), "SEND OK\r\n", okIt, 9)) break;
	}
	return 0;

error:
	return 1;
}
W1 esp_send1(W8 char_)
{
	return esp_send((W0*)&char_, 1);
}
W1 esp_receive(W0* data_, W32 dataSize_)
{ 
	W8* data = (W8*)data_;
	SW16	bytesDiscarded=0;
	 
	SW8 dataSzStr[8];
	W8  dataSzIndex = 0;
	W16 dataSz;
	W8 b;
	W16 dataSize = dataSize_;

	while (!uart_is_writable(ESP_UART)) {  tight_loop_contents();} //tight_loop_contents();
	//do
	//{
		//b = uart_getc(ESP_UART);
		while (uart_getc(ESP_UART)!='+') {}
		if (uart_getc(ESP_UART)!='I') goto error;
		if (uart_getc(ESP_UART)!='P') goto error;
		if (uart_getc(ESP_UART)!='D') goto error;
		if (uart_getc(ESP_UART)!=',') goto error;

	//	if (esp_strCharCmp(b, esp_rxConfirmStr, okIt, okSz)) break;
	//	bytesDiscarded++;
	//}while(1);
	
	//#ifdef DEBUG_ESP_RX
	//	bytesDiscarded-=okSz;
	//	if (bytesDiscarded) printf(">>%i discard", bytesDiscarded);
	//#endif

	//receber tamanho 
	dataSzIndex = 0;
	while(1)
	{
		b = uart_getc(ESP_UART);
		if (b == ':') break;
		dataSzStr[dataSzIndex]=b;
		dataSzIndex++;
	}
	dataSzStr[dataSzIndex]=0;
	dataSz = strToNum<W16>(dataSzStr);
	#ifdef DEBUG_ESP_RX
		printf("\n IPD '%s' %i", dataSzStr, dataSz);
	#endif 
		
	while (!uart_is_writable(ESP_UART)) { tight_loop_contents();} // 
	
	uart_read_blocking(ESP_UART, data, dataSz);
	#ifdef DEBUG_ESP_RX
		printf("*");
	#endif 
	  
	return 0; 
error:
	//#ifdef DEBUG_ESP_RX
		printf("\nIPDERR!");
	//#endif 
	return 1;
}
// 
// Pacotes
W1 esp_txPacket(ESP_PacketHeader& header)
{ 
	esp_send(&header, sizeof(ESP_PacketHeader));
	return 0;
}
ESP_PacketHeader esp_rxPacket()
{
	ESP_PacketHeader header;
	esp_receive(&header, sizeof(ESP_PacketHeader));
	return header;
}
//
W1 esp_txStr(const SW8* str_)
{
	ESP_PacketHeader packet;
	packet.type = ESP_PacketType_Str;
	packet.size = strlen(str_);
	esp_send(&packet, sizeof(ESP_PacketHeader));
	esp_send((W0*)str_, packet.size);
	return 0;
}
W1 esp_rxFile(const SW8* filename_, W1 delete_=false)
{
	W16 fsz, it=0;
	W32 bytesRead;
	W1 err = 1;
	W8* buffer;
	ESP_PacketHeader packet  = esp_rxPacket();
	#ifdef DEBUG_RECEIVING
		printf("\n\n***RXF: %i", packet.size);
	#endif
	if (packet.size==0) return 1;
	
#ifdef APP_USES_SD
	FSNode node;
	fs_delete(filename_);
	
	FS_FLAGS status = fs_createFileNode(filename_,&node, 0, false);
	if (status!=RES_OK)	
	{
		#ifdef DEBUG_RECEIVING
			printf("<!> file ~found");
		#endif
		goto error1;
	}
#else
	W0* node;
#endif
	
	W32 bytesWR;
	fsz  = packet.size;
	#ifdef ESP_USES_EXTERNAL_RXTXBUFFER
		buffer = (W8*)ESP_USES_EXTERNAL_RXTXBUFFER;
	#else
		buffer = new W8[ESP_RX_BUF_SZ];
	#endif
	if (!buffer) goto error1;
	while(packet.size)
	{
		RXTX_DUMMY(SEGMENT)
		const W16 toTransfer = VL_MIN( packet.size, ESP_RX_BUF_SZ);
		#ifdef DEBUG_ESP_FRX
			printf("\nFRX: %i, %i %i/%i", it, toTransfer, fsz-packet.size  , fsz  );
		#endif
		err = esp_receive(buffer, toTransfer);
		//DELAY_MS(130);
		//debugBuf(buffer, toTransfer);
		//fflush(stdout);	

		#ifdef APP_USES_SD			
			fs_rw_(&node, buffer, toTransfer, bytesWR, FS_OP_WRITE);
		#endif

		if (err) goto error2;
		packet.size-=toTransfer;
		it++;
	}
	err=0;
	goto success;
	error:
	error2:
		#ifdef DEBUG_ESP_FRX
			printf("ERR");
		#endif
		#ifndef ESP_USES_EXTERNAL_RXTXBUFFER
			delete[] buffer;
		#endif
	error1:
	success:
		#ifdef DEBUG_ESP_FRX
			printf("OK %i packs", it);
		#endif
#ifdef APP_USES_SD
		fs_close(&node);
#endif
	return err;
}

typedef W0(*ESP_SEND_PROGRESS_CALLBACK)(W32,W32);
W1 esp_txFile(const SW8* filename_, W1 sendFilename_=false, ESP_SEND_PROGRESS_CALLBACK call_=0)
{
#ifdef APP_USES_SD

	ESP_PacketHeader packet;
	if (sendFilename_)
	{
		packet.type = ESP_PacketType_NamedFile;
		packet.size = strlen(filename_)+1;
		printf("\npacket %s %i!", filename_, packet.size );
		esp_send(&packet, sizeof(ESP_PacketHeader));
		esp_send((W0*)filename_, packet.size);
		esp_clearRx();
		DELAY_MS(100);
	}


	//printf("\nTX f:");
	W32 bytesRead;

	W8*
	#ifdef ESP_USES_EXTERNAL_RXTXBUFFER
		buffer = (W8*)ESP_USES_EXTERNAL_RXTXBUFFER;
	#else
		buffer = new W8[ESP_RX_BUF_SZ];
	#endif

	FSNode node;
	FS_FLAGS status = fs_createFileNode(filename_,&node, 0, true);
	if (status!=RES_OK)	
	{
		#ifdef DEBUG_SENDING
			printf("<!> file ~found");
		#endif 
		return 1;
	}
	
	packet.type = ESP_PacketType_File;
	packet.size = node.file.fileSize;
	esp_send(&packet, sizeof(ESP_PacketHeader));

	W32 toSend = 	packet.size ;
	
	#ifdef DEBUG_SENDING
		printf("\nSend %i", toSend);
	#endif
	
	while(toSend)
	{
		W32 toCopy = MIN(toSend, 512);
		status = fs_rw_(&node, buffer, toCopy, bytesRead, FS_OP_READ);
		if (status!=RES_OK)	
		{
			printf("IO error");
			break;
		}
		esp_send(buffer, toCopy);
		toSend-=toCopy;
		if (call_)
		{
			call_(packet.size-toSend ,packet.size);
		}
		#ifdef DEBUG_SENDING
			printf("\n%i%/%i", packet.size-toSend, packet.size);
		#endif
	}
	#ifdef DEBUG_SENDING
		printf(ui_strs_done);
	#endif
		
	#ifndef ESP_USES_EXTERNAL_RXTXBUFFER
		delete[] buffer;
	#endif
	return toSend;
#endif
	return 0;
}
//
W8 esp_init()
{
	DEBUG_ESP("CFG...");
	 
	//esp_test();
	PIN_INIT(PIN_ESP_TX)
	PIN_INIT(PIN_ESP_RX)
	PIN_MODE(PIN_ESP_TX,   PIN_MODE_OUTPUT)
	PIN_MODE(PIN_ESP_RX,   PIN_MODE_INPUT)
	PIN_SET_PULLUP(PIN_ESP_TX) 
	PIN_SET_PULLUP(PIN_ESP_RX) 
	
	W32 bauds = uart_init(ESP_UART, 115200); 
	mcu_log("Bauds:%i\n", bauds);

	gpio_set_function(PIN_ESP_TX, GPIO_FUNC_UART);
	gpio_set_function(PIN_ESP_RX, GPIO_FUNC_UART);
	
	uart_set_translate_crlf(ESP_UART, false);
	uart_set_fifo_enabled(ESP_UART, true);

	uart_set_format(ESP_UART, 8, 1, UART_PARITY_NONE);
	uart_set_hw_flow(ESP_UART, false, false);
	DELAY_MS(100);
	
	ESP_SENDCMD(esp_okConfirmStr    , "")
	ESP_SENDCMD(esp_okConfirmStr    , "+GMR")
	ESP_SENDCMD(esp_readyConfirmStr , "+RST")
	ESP_SENDCMD(esp_okConfirmStr    , "E0")		// disable echo
	ESP_SENDCMD(esp_okConfirmStr    , "+CWMODE=1")
	ESP_SENDCMD(esp_okConfirmStr    , "+GMR")

	DEBUG_ESP(ui_strs_done);
	return 0;
error:
	return 1;
}
W8 esp_wifi_connect()
{
	DEBUG_ESP("\nWifi?\n")
	esp_clearRx();
	espRes = esp_cmd(esp_okConfirmStr  , "+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"");
	if (espRes) return 1;

	ESP_SENDCMD(esp_okConfirmStr    , "+CIFSR", ESP_CMDTYPE_IP)

	return 0;
error:
	return 1;
}
W8 esp_server_connect()
{
	DEBUG_ESP("Server?");
	ESP_SENDCMD(esp_okConfirmStr    , "+CIPMUX=0")
	ESP_SENDCMD(esp_okConfirmStr    , "+CIPSTATUS", ESP_CMDTYPE_STATUS)

	esp_clearRx();
	espRes = esp_cmd(esp_okConfirmStr, "+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_WELCOME_PORT "");
	esp_clearRx();
	if (espRes!=RES_OK) return 1;
	espRes = esp_cmd(esp_okConfirmStr, "+CIPCLOSE");
	if (espRes!=RES_OK) return 1;
	esp_clearRx();

	DELAY_MS(3000);
	espRes = esp_cmd(esp_okConfirmStr, "+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT "");
	esp_clearRx();
	if (espRes!=RES_OK) return 1;
	
	DEBUG_ESP_STATUS("connected!",0) 
	return 0;
error:
	return 1;
}


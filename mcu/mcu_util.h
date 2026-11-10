#define MCU_UTIL_INCLUDES_RXTX
 

void mcu_init_(unsigned short mcuSharedBufferSz_);

#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
	#include "rasp_pico.h"
#else
	#include "arduino.h"
#endif


#define UNIT_GIGA 1000000000
#define UNIT_MEGA 1000000
#define UNIT_KILO 1000

#ifndef ERRORS_DEF
	#define RES_OK 0 
	#define RES_FAIL 1
	#define ERR_MKENUM_(X_,Y_,Z_)     ERROR_##X_##_##Y_=Z_ , 
	#define ERR_MKSTRS1_(X_,Y_,Z_)    const SW8 ERROR_STR_##Z_ [] VL_STORE_IN_FLASH =  "" #Y_ "";
	#define ERR_MKSTRS2_(X_,Y_,Z_)    ERROR_STR_##Z_,
#endif

	#define			   VL_IS_WHITESPACE(C_) ((C_)<=32 && (C_)!=0)
	#define			   VL_IS_DIGIT(C_)		((C_)>='0' && (C_)<='9')
	#define            VL_IS_ALPHA(C_)		(((C_)>64 && (C_)<91) || ((C_)>96 && (C_)<123))
	#define            VL_TO_LOWER(C_)		(VL_IS_ALPHA(C_)?(C_|(0x20)):(C_))
	#define            VL_TO_UPPER(C_)		(VL_IS_ALPHA(C_)?(C_&(0xDF)):(C_))
	#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
		#define DEBUG_PRINT(X_)			printf(X_)
		#define DEBUG_PRINTLN(X_)		printf("\n"); printf(X_)
	#else
		#define DEBUG_PRINT(X_)			Serial.print(X_)
		#define DEBUG_PRINTLN(X_)		Serial.print("\n"); Serial.print(X_)
	#endif
		#define ATOMIC_DELAY            __asm__ __volatile__ ("nop");

	//#ifdef _MSC_VER
	//#define VL_PACK_BEGIN  __pragma( pack(push, 1) )
	//#define VL_PACK_MIDDLE
	//#define VL_PACK_END    __pragma( pack(pop) )
	//#else
	//#define VL_PACK_BEGIN  
	//#define VL_PACK_MIDDLE
	//#define VL_PACK_END    __attribute__((__packed__))
	//#endif

#ifndef APP_DEBUG
	#define DEBUG(X,...) printf( X, __VA_ARGS__);
#endif

#define STOP							stop();
#define STOP_NO_BLINK					while(1){} 
#define        VL_IS_PRINTABLE(C_)      ((C_)>31 && (C_)<127)
#define VL_PI							3.14159265359f
#define VL_CONCAT1_(X_)					X_
#define VL_CONCAT3_(X_,Y_)				X_##Y_
#define VL_CONCAT2_(X_,Y_)				VL_CONCAT3_(X_,Y_)
#define VL_CONCAT(PREF_,NAME_,SUF_)     VL_CONCAT2_(VL_CONCAT1_(PREF_)##NAME_,SUF_)

// aceitar um W16 mas A ou A,B se refere a W32 (que nao existe aqui)
// se HI_ desconta 
#define MASK_RANGE0(A_,B_)		    ((1<<((B_)-(A_)+1))-1)		// 
#define MASK_RANGE(A_,B_)		    (MASK_RANGE0(A_,B_)<<(A_))
#define ZERO_BITS(X_, A_,B_)	    ((X_) &  (~(MASK_RANGE(A_,B_))) )
#define SET_BITS(X_, A_,B_, VAL_)   (   ZERO_BITS(X_,A_,B_) | (((VAL_)&MASK_RANGE0(A_,B_))<<(A_)) ) 
#define GET_BITS(X_, A_,B_)         ((X_)>>(A_) & MASK_RANGE0(A_,B_) )
#define   GET_BIT(X_, A_)           ((X_)>>(A_) & 1)

#define VL_BIT_SET(W, N, V)			(((W) & (~(1<<(N)))) | ((V)<<(N)) )
#define W8_BIT(V,START)				(((V)>>(START))&1)			
#define W_BIT(V,START)				W8_BIT(V,START)
#define W8_BIT_SET(W, N, V)			VL_BIT_SET(W, N, V)	
#define W8_BIT_GET(V,N)				(((V)>>(N))&1)
#define W8_TAKE(V,START,N)			(  (~(0xFF<<N))   & ((V)>>START) )
#define W8_REVERSE(W)				(W8_BIT(W,7)|(W8_BIT(W,6)<<1)|(W8_BIT(W,5)<<2)|(W8_BIT(W,4)<<3)|(W8_BIT(W,3)<<4)|(W8_BIT(W,2)<<5)|(W8_BIT(W,1)<<6)|(W8_BIT(W,0)<<7))

#define W16_SET_LOW(X_,V_)	(((X_)& 0xFF00) | (V_))
#define W16_SET_HI(X_,V_)	(((X_)& 0x00FF) | ((V_)<<8))

#define W16_GET_LOW(X_)	((X_)&0xFF)
#define W16_GET_HI(X_)	(((X_)>>8)&0xFF)
#define W16_REVERSE(W)		\
						((W8_BIT(W,15)    )\
						|(W8_BIT(W,14)<< 1)\
						|(W8_BIT(W,13)<< 2)\
						|(W8_BIT(W,12)<< 3)\
						|(W8_BIT(W,11)<< 4)\
						|(W8_BIT(W,10)<< 5)\
						|(W8_BIT(W, 9)<< 6)\
						|(W8_BIT(W, 8)<< 7)\
						|(W8_BIT(W, 7)<< 8)\
						|(W8_BIT(W, 6)<< 9)\
						|(W8_BIT(W, 5)<<10)\
						|(W8_BIT(W, 4)<<11)\
						|(W8_BIT(W, 3)<<12)\
						|(W8_BIT(W, 2)<<13)\
						|(W8_BIT(W, 1)<<14)\
						|(W8_BIT(W, 0)<<15))

#define VL_VALUE_IN_INCLUSIVE_RNG(X_,A_,B_)		(((X_)>=(A_))&&((X_)<=(B_)))
#define VL_VALUE_IN_INCLUSIVE_THRESH(X_,A_,TH_)	VL_VALUE_IN_INCLUSIVE_RNG(X_, (A_)-(TH_), (A_)+(TH_))

#define VL_MIN(a_,b_)	((a_) > (b_) ? (b_) : (a_))
#define VL_MAX(a_,b_)	((a_) < (b_) ? (b_) : (a_))
#define VL_ABS(X_)      (((X_) >= 0)? (X_):(-(X_)))

// tuplos da forma X_(NOME,ID)
#define VL_DEF_MKENUM_(X_,Y_)       X_=Y_ , 
#define VL_DEF_MKSTRS_(X_,Y_)       "" #X_ "", 
#define VL_DEF_MKSTRSIDX_(X_,Y_)    Y_>>4 , 
#define VL_DEF_MKENUM(TITLE_, DEF_) enum TITLE_ { DEF_(VL_DEF_MKENUM_) TITLE_##_END };
#define VL_DEF_MKSTRS(TITLE_, DEF_)  \
    const SW8* TITLE_ []= { DEF_(VL_DEF_MKSTRS_) 0 }; \
    const W8 TITLE_##Arr []= {  DEF_(VL_DEF_MKSTRSIDX_) -1};

#ifndef VL_ARR4
	#define VL_ARR4(ARR, A0, A1, A2, A3) { ARR[0]=A0; ARR[1]=A1; ARR[2]=A2; ARR[3]=A3; }
#endif


// buffer de strings, etc
#ifndef MCU_SHAREDBUFFER_SZ
	char *mcuSharedBuffer;
#else
	#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
		char mcuSharedBuffer[MCU_SHAREDBUFFER_SZ] VL_ALIGN(4); 
	#else
		char mcuSharedBuffer[MCU_SHAREDBUFFER_SZ];
	#endif
#endif
		
SW8* getFilename(SW8* path_)
{
	SW32 lastSlash = -1;
	W32 size = strlen(path_);
	for (W32 i=0; i< size; i++)
	{
		if (path_[i]=='\\' || path_[i]=='/' )
			lastSlash = i;
	}
	if (lastSlash==-1) return path_;
	return path_ + lastSlash + 1;
}



SW8* getFilenameExt(SW8* name_)
{
	if (name_[0]=='.'&&name_[1]=='.'&&name_[2]==0) return 0;
	W32 N = strlen(name_);
	if (N==0) return 0;
	SW8 *b = name_+N-1;
	while(N)
	{
		if (*b=='.') return name_+1;
		b--;
		N--;
	}
	return 0;
}
template<typename DT1, typename DT2>
W1   trimToRange(DT1& x0_, DT1& x1_,  DT2 A_,  DT2 B_)
{
	if ((x1_<A_) || (x0_ >= B_)) return true;
	if ((x0_<=A_) && (x1_>=A_)) x0_=A_;
	if (x1_ > B_) x1_ = B_;
	return false;
}
W32  chsToLBA(const W32 c_, const W32 h_, const W32 s_, const W32 tH_, const W32 tS_)
{
	return (c_*tH_ + h_)*tS_ + (s_-1);
}
W0  bios_decode_cylinder_sectorspertrack(const W16 cx_, W16& cylinder, W16& sector)
{
	// CX = 
	//                             
	// mask =            0xFFFF        0x3    0x3F
	// bits =    15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
	//          |_____________________||_| |_________|
	//               cils (baixo)       |      | 
	//                           cils(alto)    |
	//                                        sector 

	// sector = cx & 1F
	// cils = (((cx>>5)&7)<<8) |(cx>>8) 

	sector       			=	(  cx_ & 0x3F  );

	cylinder				=	(  (( cx_ >> 6 ) & 0x3) << 8) | 
								   (  cx_ >> 8 );
}
W0  bios_encode_cylinder_sectorspertrack(const W32 totalCyls, const W32 totalSectorsPerTrack, W16& value_)
{
	// CX = 
	//                             
	// mask =            0xFFFF        0x3    0x3F
	// bits =    15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
	//          |_____________________||_| |_________|
	//               cils (baixo)       |      | 
	//                           cils(alto)    |
	//                                        sector 
	//
	const W16 maxCyl = totalCyls-1;
	// totalSectorsPerTrack é codificado como "está"
	// porque começa em 1 

	value_ =	(    (totalSectorsPerTrack) &    0x3F)         |
				(    ((maxCyl) & 0xFF)           << 8)		   |
				((   ((maxCyl)>>8)     &  0x3  )  <<6)    ;    
}


// imprime 8bits
template<typename DT>
W0 sprnBits(SW8* dst_, const SW8** strs_, DT val_, W1 prnAll_=false)
{
	W16 len=0;
	dst_[0] = 0;
	for (W8 i=0; i< sizeof(DT)*8; i++)
	{
		if (strs_[i]==0) break;
		const W1 biti =  GET_BIT(val_,i);
		if (prnAll_ || (!prnAll_ && biti) )
			sprintf(dst_+len, " \n %s:%1u", strs_[i], biti);
		len = strlen(dst_);
	}
}
// imprime 8bits
template<typename DT, int BITS_>
W0 sprnValueBits(DT val_, SW8* dst_)
{
	DT val = val_; 
	W32 i = 0; 
	for (; i< BITS_; i++)
	{
		dst_[i] = (val&1)? '1':'0';
		val >>= 1;
	}
	dst_[i] = 0;
} 
// imprime 8bits
template<typename DT, int BITS_>
W0 sprnValueBitsLowAtRight(DT val_, SW8* dst_)
{
	DT val = val_; 
	W32 j = BITS_ -1 , i = 0;
	dst_[BITS_] = 0;
	for (; i< BITS_; i++, j--)
	{
		dst_[j] = (val&1)? '1':'0';
		val >>= 1;
	}
} 
W0 prnBuff(W8* buffer_, W32 rows_=32, W32 total_=512, W32 stAddr_=0)
{
	for (W16 i=0; i<rows_; i++)
	{
		W8 toShow = VL_MIN(total_, 16);
		W8 toHide = 16-toShow;

		printf("\n %04X | ", stAddr_+i*16);
		for (W16 j=0; j<toShow; j++)
			printf("%02X ", buffer_[j]);
		for (W16 j=0; j<toHide; j++)
			printf("---");

		printf(" ");
		
		for (W16 j=0; j<toShow; j++)
			printf("%c", VL_IS_PRINTABLE(buffer_[j])?buffer_[j]:178);
		for (W16 j=0; j<toHide; j++)
			printf("-");

		buffer_+=16;
		total_-=toShow;
		if (total_==0) break;
	}
} 

/*
	printf("\n        ");
	for (W16 j=0; j< 16; j++)
	{
		printf("%02X ", j);
	}
	for (W16 i=0; i< 30; i++)
	{
		printf("\n0x%04X: ",i*16);	
		for (W16 j=0; j< 16; j++)
		{
			printf("%02X ",(W8)( *(bf+j)));
		}
		printf(" ");
		for (W16 j=0; j< 16; j++)
		{
			if (*(bf+j)>=0x20)
				printf("%c",(W8)( *(bf+j)));
			else
				printf("_");
		}
		bf+=16;
	}
*/

W0 upperStr(SW8* cstr)
{
	while (*cstr)  { *cstr = VL_TO_UPPER(*cstr); cstr++; }
} 
template<typename DT>
DT strToNum(SW8* cstr)
{
	DT num = 0;
	DT K = 1;
	SW8 n = strlen(cstr);
	if (n==0) return 0;
	n--;
	while(n>-1)
	{
		num+= ((cstr[n])-'0')*K;
		K*=10; 
		n--;
	}
	return num;
}
W0 blink(W16 times, W16 dly_){
  
	#ifndef USE_VISUAL_STUDIO
		for (W16 i=0; i< times; i++)
		{
		  PIN_WRITE(PIN_LED,HIGH);
		  DELAY_MS(dly_)
		  PIN_WRITE(PIN_LED,LOW);
		  DELAY_MS(dly_);
		}
	#endif

}

W0 integer2Str(W32 int_, SW8* str_)
{
	W32 num  = int_;
	W32 remain;
	W8 digits = 0 ; //log?
	do
	{
		num/=10;
		digits++;
	}while(num);

	SW8 * s = str_+digits;
	*s = 0;
	s--;
	num  = int_;
	
	do
	{
		remain = num%10;
		num/=10;
		*s= '0' + remain;
		s--;
	}while(num);
}

#ifdef MCU_INCLUDES_STRCVT
template <class T>
inline std::string  t2str(T  n)
{
	std::ostringstream convert;
	convert << n;
	return convert.str();
}
#endif


template<typename DT>
const SW8* searchString(W16 id_, const SW8** strs_, DT* idsArray_)
{
	W16 i=0;
	while(1)
	{
		if (idsArray_[i]==(DT)-1) break;
		if (idsArray_[i]==id_) return strs_[i];
		i++;
	}
	return 0;
}

// aumentar PTR, streamPosBits indica um bit de 0..7
template <typename T>
inline W0 getBitsInit( T* ptr,  T& stream0,  T& streamPosBits)
{
	// inicializar
	stream0 = ptr[0];
	streamPosBits = 0;
}
template <typename T, int BITS_>
inline T getBits(T** ptr, T& stream0, T& streamPosBits)							
{
	if ( (streamPosBits + BITS_) < ((sizeof(T)*8) )  ) 							
	{		
		// mascarar o que está em stream
		T data = (stream0) &  ((1<<BITS_)-1);				
		stream0>>=BITS_;								
		streamPosBits+=BITS_;		
		return data;
	}													
	const W8 N0 = (sizeof(T)*8) - streamPosBits;						
	streamPosBits = BITS_-N0;							
	++(*ptr);											
	const T stream1 = **ptr;	
	T data = ((stream1<<N0)|stream0) & ((1<<BITS_)-1);
	stream0 = stream1 >>streamPosBits;					
	return data;
}
template <typename T>
inline T getBitsDyn(T** ptr, T& stream0, T& streamPosBits, W8 BITS_)							
{
	if ( (streamPosBits + BITS_) < ((sizeof(T)*8) )  ) 							
	{		
		// mascarar o que está em stream
		T data = (stream0) &  ((1<<BITS_)-1);				
		stream0>>=BITS_;								
		streamPosBits+=BITS_;		
		return data;
	}													
	const W8 N0 = (sizeof(T)*8) - streamPosBits;						
	streamPosBits = BITS_-N0;							
	++(*ptr);											
	const T stream1 = **ptr;	
	T data = ((stream1<<N0)|stream0) & ((1<<BITS_)-1);
	stream0 = stream1 >>streamPosBits;					
	return data;
}

	/*
  
  [54321098'76543210] [54321098'76543210] [54321098'76543210]
								                      streamPos
								BITS_-N0
								  |-----|--N0---|
  [54321098'76543210] [54321098'76543210] [54321098'76543210]
								  ^             ^
								  |             |
							streamPos+BITS_	 streamPos
	
	*/
	// se os bits pedidos superarem a posição do 1º elemento
	// em ptr




#ifndef USE_VISUAL_STUDIO
#ifdef DEBUG_PINS
	W0 test_pin_init()
	{
		PIN_INIT(PIN_LED)
		PIN_MODE(PIN_LED, PIN_MODE_OUTPUT)
		DEBUG_PRINT("\ntest pins (&led)\n");
	}
	W0 test_pin(unsigned short pin_, const char* pin_name_, const char* portname_bit_, const char* info_=0)
	{
		#ifndef USE_VISUAL_STUDIO
			printf("\nPIN %02i %s %s %s", pin_, pin_name_, portname_bit_, info_);
			for (unsigned char j=0; j< 30; j++)
			{
				DEBUG_PRINT("-");
				PIN_WRITE(pin_, 1);
				//PIN_WRITE(PIN_LED, 1);
				DELAY_MS(10);
				DEBUG_PRINT("_");
				PIN_WRITE(pin_, 0);
				//PIN_WRITE(PIN_LED, 0);
				DELAY_MS(50);
			}
		#endif
	}
#endif

#endif

void stop()
{
	DEBUG_PRINT("\nSTP"); while(1){ blink(10,100); DEBUG_PRINT("."); }
}

#ifndef FATAL
void fatal(W8 section_, W16 err_)
{ 
	printf("\n!!FATAL!!");
	printf("\n@%u: %u ", section_, err_);

	#ifdef APP_UI
		ui_fatal(section_, err_);
	#endif
	while(1){}
}
#endif

inline W0 mcu_init_(W16 mcuSharedBufferSz_)
{
	#ifndef MCU_SHAREDBUFFER_SZ
		if (mcuSharedBufferSz_)
			mcuSharedBuffer = new SW8[mcuSharedBufferSz_];
	#endif
}


#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)

W0  changeBoardFreq(FP32 freq_)
{
	//W1 isOK = set_sys_clock_khz(freq_,false);
	//printf("\nFreq: %3.2f KHz ok? %i\n", freq_, isOK);

}

#endif


#ifndef USE_VISUAL_STUDIO
#ifdef MCU_UTIL_INCLUDES_RXTX
/*
#include <tusb.h>
#include <pico/stdio_usb.h>

#define USE_TX_READY_BYTE  0xAA
#define USE_TX_START_BYTE  0xBB
#define USE_TX_ERROR_BYTE  0xEE
#define USE_TX_PACK		   160
W0 rx_waitReadyAck()
{
	while(1)
	{
		if (tud_cdc_available())
		{
			const W8 byte = tud_cdc_read_char();
			if (byte==USE_TX_READY_BYTE) break;
		}
		blink(1,100);
	}
}
W0 rx_waitStartAck()
{
	while(1)
	{
		if (tud_cdc_available())
		{
			const W8 byte = tud_cdc_read_char();
			if (byte==USE_TX_START_BYTE) break;
		}
	}
}
W1 rx_waitEndAck()
{
	//tud_cdc_read_flush();
	while(1)
	{
		if (tud_cdc_available())
		{
			const W8 byte = tud_cdc_read_char();
			if (byte==USE_TX_START_BYTE) return 0; else
			if (byte==USE_TX_ERROR_BYTE) return 1;
		}
	}
}
W0 tx(W0* buf_,  W32 size_)
{
	W8* buf = (W8*)buf_;
	while(size_)
	{
		W8 xsum = 0;
		const W16 toTxSize = VL_MIN(size_, USE_TX_PACK);
		for (SW32 i=0; i< toTxSize; i++) xsum^=buf[i];
		retry:
			rx_waitStartAck();
			tud_cdc_write(&toTxSize, 2);
			tud_cdc_write_flush();
			tud_cdc_write(buf, toTxSize);
			tud_cdc_write(&xsum, 1);
			tud_cdc_write_flush();
		if (rx_waitEndAck()) goto retry;
		size_-=toTxSize;
		buf+=toTxSize;
	}
}
*/
#endif
#endif
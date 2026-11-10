// pinos
#include "charmap.h"
#include "shapes.h"


enum DRAW_ERRORS { DRAW_ERROR_FONT_RLE, DRAW_ERROR_BUFFER_ALLOC };

SW8 drawTextOutline = 0;
SW8 drawTextShadowX = 0;
SW8 drawTextShadowY = 0;
W16 drawTextOutlineColor = 0;
W16 drawTextShadowColor = 0;

#ifdef DRAW_USES_BUFFER
	SW16 draw_roi_x = 0;

	W16 *dispBuf VL_ALIGN(4);
	#define DRAW_UPDATE_RRECT(R_)				draw_updateRect(R_.x,R_.y,R_.x+R_.lx-1,R_.y+R_.ly-1);
	#define DRAW_UPDATE_RECT(X0_,Y0_,X1_,Y1_)	draw_updateRect(X0_,Y0_,X1_,Y1_);
	#define DRAW_UPDATE							draw_update();
	#define DRAW_UPDATE_FX_FILLCOLOR			*b = color_;	
	#define DRAW_UPDATE_FX_TX					disp_txColor(*b); 

	#define DRAW_RECT_TRANSVERSE(FX_)	\
		W16 *brow = dispBuf + y0_*DISP_WIDTH + x0_ ; \
		for (W8 r=y0_; r<= y1_; r++, brow+=DISP_WIDTH)\
		{\
			W16* b = brow;\
			for (SW16 c=x0_; c<=x1_; c++, b++)\
			{\
				FX_\
			}\
		}
	
	W0 draw_addColor(W16 *b, const SW8 delta_, const W1 add_, const W16 size_)
	{
		if (add_)
		{
			for (W16 r=0; r<  size_; r++, b++)
				*b= DISP_COLOR_MK_W16(
						VL_MAX(0, (SW8)DISP_COLOR_R(*b)+delta_ ),
						VL_MAX(0, (SW8)DISP_COLOR_G(*b)+delta_ ),
						VL_MAX(0, (SW8)DISP_COLOR_B(*b)+delta_ ));
		}
		else
		{
			for (W16 r=0; r< size_; r++, b++)
				*b= DISP_COLOR_MK_W16(
						VL_MAX(0, (SW8)DISP_COLOR_R(*b)-delta_ ),
						VL_MAX(0, (SW8)DISP_COLOR_G(*b)-delta_ ),
						VL_MAX(0, (SW8)DISP_COLOR_B(*b)-delta_ ));
		}
		
	}
	W0 draw_moveBufferAndCopy(SW16 dx_, W16 stx_, W16 * buf2_)
	{
		stx_<<=1;
		W8 *brow  = (W8 *)dispBuf;
		#define browStep  (DISP_WIDTH<<1)

		if (dx_<0) // <----|
		{
			
			// brow            brow2
			//  .-.                 stx 
			//  V |                 |
			// [--|-------XX]  [----XX-----]
			//   dx       ^         |  
			//            |_________| 
			// 0<---|    |--|    
			const W16 dx =  VL_ABS(dx_)<<1;
			W8 *brow2 = ((W8 *)(buf2_)) + stx_;
			for (W8 r=0; r< DISP_HEIGHT; r++, brow+=browStep, brow2+=browStep)
			{
				memmove(brow, brow + dx, browStep - dx);
				draw_addColor((W16*)(brow), 3, false, (browStep - stx_ - dx)>>1);
				memcpy(brow + browStep - dx, brow2, dx);
			}

		}
		else      // |-----> DIREITA
		{
			// brow            brow2
			//  .-.                 stx 
			//  V |                 |
			// [--|-------XX]  [----XX-----]
			//   dx       ^         |  
			//            |_________| 
			// 0<---|    |--|    
			const W16 dx =  VL_ABS(dx_)<<1;
			W8 *brow2 = ((W8 *)(buf2_)) + browStep - stx_ - dx;
			for (W8 r=0; r< DISP_HEIGHT; r++, brow+=browStep, brow2+=browStep)
			{
				memmove(brow + dx, brow, browStep - dx);
				draw_addColor((W16*)(brow + stx_ + dx), 3, false, (browStep - stx_ - dx)>>1);
				memcpy(brow, brow2, dx);
			}
		}
		#undef browStep 
	}
	W0 draw_moveBuffer(SW16 dx_)
	{
		W8 *brow  = (W8 *)dispBuf;
		#define browStep  (DISP_WIDTH<<1)

		if (dx_<0) // <----|
		{
			const W16 dx =  VL_ABS(dx_)<<1;
			for (W8 r=0; r< DISP_HEIGHT; r++, brow+=browStep)
				memmove(brow, brow + dx, browStep - dx);
		}
		else      // |-----> DIREITA
		{
			const W16 dx =  VL_ABS(dx_)<<1;
			for (W8 r=0; r< DISP_HEIGHT; r++, brow+=browStep)
				memmove(brow + dx, brow, browStep - dx);
		}
		#undef browStep  
	}

	inline W0 draw_updateRect(W8 x0_, W8 y0_, W8 x1_, W8 y1_)
	{
		DISP_TXRAM(x0_,x1_,y0_,y1_)
		PIN_HI(PIN_DISP_DC)
	    PIN_LO(PIN_DISP_CS)
		// transmitir cor-a-cor desde o buffer
		DRAW_RECT_TRANSVERSE(DRAW_UPDATE_FX_TX)
		/*
		  DISP_TX// dummy 12
		  DISP_TX// dummy 13
		  DISP_TX// dummy 14
		  DISP_TX// dummy 15
		  DISP_TX// dummy 16
		  */
		  PIN_HI(PIN_DISP_CS)
	}
	W0 draw_update()
	{
		draw_updateRect(0,0,DISP_WIDTH-1, DISP_HEIGHT-1);
	}
#else
	#define DRAW_UPDATE
	#define DRAW_UPDATE_RECT(X0_,Y0_,X1_,Y1_)
#endif

	W0 draw_initBuffer()
	{
		#ifdef DRAW_USES_BUFFER
			dispBuf = new W16[DISP_WIDTH*DISP_HEIGHT];
			if (!dispBuf) 
			{
				printf("\n!!!DISP_BUF ALLOC!!");
				fatal(DRAW, DRAW_ERROR_BUFFER_ALLOC);
			}
		#endif
	}
	W0 draw_disposeBuffer()
	{
		#ifdef DRAW_USES_BUFFER
			delete[] dispBuf;
		#endif
	}
W0  draw_rect( SW16 x0_, SW16 y0_, SW16 x1_,  SW16 y1_,  W16 color_, W1 immediate_=false)
{
	#ifdef DRAW_USES_BUFFER
		if (immediate_) goto skip;
		{
			#if (UI_ANIMATE_DIR_BUFF==0)	// se usa buffer nao é preciso ter cuidado
				x0_+=draw_roi_x;
				x1_+=draw_roi_x;
				if (trimToRange(x0_,x1_,0,DISP_WIDTH-1))  return;
				if (trimToRange(y0_,y1_,0,DISP_HEIGHT-1)) return;
			#endif

			DRAW_RECT_TRANSVERSE(DRAW_UPDATE_FX_FILLCOLOR)
		}
		return;
	#endif
	skip:
		if (trimToRange(x0_,x1_,0,DISP_WIDTH-1)) return;
		if (trimToRange(y0_,y1_,0,DISP_HEIGHT-1)) return;
		DISP_TXRAM(x0_,x1_,y0_,y1_)
		disp_rep_function((x1_-x0_+1)*(y1_-y0_+1), &color_); 

}
W0  draw_pixel(SW16 x0_, const SW16 y0_, const W16 color_)
{
	#ifdef DRAW_USES_BUFFER
		#if (UI_ANIMATE_DIR_BUFF==0)
			x0_+=draw_roi_x;
		#endif
		if (!(x0_>=0 && y0_>=0 && x0_<DISP_WIDTH && y0_<DISP_HEIGHT)) return;
		dispBuf[y0_*DISP_WIDTH + x0_]=color_;
	#else
		if (!(x0_>=0 && y0_>=0 && x0_<DISP_WIDTH && y0_<DISP_HEIGHT)) return;
		DISP_TXRAM(x0_,x0_,y0_,y0_)
		disp_txColor(color_); 
	#endif
}
inline W0  draw_pixelsRep(SW16 x0_, SW16 y0_, const W16 color_,  SW16 len_)
{
	if ((y0_<0) || (y0_>=DISP_HEIGHT)) return;
	if (len_==0) return;
	#ifdef DRAW_USES_BUFFER
		#if (UI_ANIMATE_DIR_BUFF==0)
			x0_+=draw_roi_x;
		#endif
		SW16 x1_ = x0_ + len_;
		
		if (trimToRange(x0_,x1_,0,DISP_WIDTH-1)) 
			return;
		len_ = x1_-x0_;

		W16 *b = dispBuf + y0_*DISP_WIDTH + x0_;
		while(len_)
		{
			*b = color_;
			 b++;
			 len_--;
		}
	#else
		SW16 x1_ = x0_ + len_;
		if (trimToRange(x0_,x1_,0,DISP_WIDTH-1)) 
			return;
		len_ = x1_-x0_;
		DISP_TXRAM(x0_,x0_+(len_-1),y0_,y0_)
		disp_rep_function(len_, (W0*)&color_);
	#endif
}
//---------------------------------------------------------
W0  draw_rect(const Ret ret_, const W16 color_)
{
	if (ret_.hasNoSize()) return;
	draw_rect(ret_.x, ret_.y, 
			  ret_.x + ret_.width-1, 
			  ret_.y + ret_.height-1, 
			  color_);
}
W0  draw_progressbar(const Ret ret_, const W16 colorFront_, const W32 completed_, const W32 total_)
{
	Ret r = ret_;
	r.width = (FP32)ret_.width*((FP32)completed_/(FP32)total_);
	draw_rect(r, colorFront_);
#ifdef DRAW_USES_BUFFER
	DRAW_UPDATE_RRECT(r)
#endif
}
SW16  draw_progressbar_first(const Ret ret_,  const W16 colorFront_, const W32 completed_, const W32 total_, const SW16 lastX_)
{
	SW16 dx = (FP32)ret_.lx*((FP32)completed_/(FP32)total_);
	if (dx==0) return lastX_;

	const SW8 x1 = ret_.x + dx - 1;
	dx = x1 - lastX_ + 1;
	if (dx==0) return lastX_;
	const W8 y0 = ret_.y;
	const W8 y1 = ret_.y + ret_.ly - 1;
	 

	DISP_TXRAM(lastX_, x1, y0, y1)
	disp_rep_function( (dx * ret_.ly) , (W0*)&colorFront_); 
	return lastX_+dx;
}
W0  draw_clear(const W16 c)
{
	#ifdef DRAW_USES_BUFFER
		disp_rep_function = disp_txRepColor;
	#endif
	draw_rect(0,0, DISP_WIDTH-1, DISP_HEIGHT-1, c);
}
W0  draw_displayFrameBlocks(const W8 blkLen_, const W16 color_=0, const W16 color2_=0)
{
  for (W8 i=0; i< 2; i++)
  {
	  for (W8 y=0, k=0; y< DISP_HEIGHT; y+=blkLen_, k++)
		  for (W8 x=0; x< DISP_WIDTH; x+=blkLen_, k++)
		  {
			 const W16 color = (k&1)?color2_:color_;
  		     draw_rect(x, y, MIN(x+blkLen_-1, DISP_WIDTH-1), MIN(y+blkLen_-1, DISP_HEIGHT-1), color);
		  }
  }
}


template<int SCALE_>
W0  draw_icon(const SW16 x0_, const SW16 y0_, const W32 iconId_,  const W1 dotTransparent_=false)
{
#ifdef APP_USES_ICONS
	if ((W8)iconId_ >= (W8)ICON_NONE) return;
	
	//printf("\nICON!!!");

	const W16 iconWidthBytes = DRAW_ICON_SIZE>>1;
	
	W16 parity = 0;
	W16 row_ptr  = (iconId_*DRAW_ICON_SIZE)>>1;	// situar-se no icone
	SW16 penY = y0_;

	for (W16 r=0; r < DRAW_ICON_SIZE; r++, 
		row_ptr+=DRAW_ICONS_MTX_WIDTH_IN_BYTES,			// adicionar largura em bytes
		penY+=SCALE_)
	{
		if (penY<0) continue; 
		//printf("\n");

		SW16 penX = x0_;
		W16 ptr  = row_ptr;
		for (W16 c=0; c < iconWidthBytes ; c++, ptr++, penX+=(SCALE_*2))
		{
			if (penX< 0) continue;
			//const W8 two_pixels  = VL_FLASH_READ_BYTE(iconData,ptr);
			const W8 two_pixels  = iconData[ptr];
			
			if (two_pixels == (DRAW_ICON_ALPHA_COLOR|(DRAW_ICON_ALPHA_COLOR<<4)))
			{
				//printf("%1x%1x",DRAW_ICON_ALPHA_COLOR,DRAW_ICON_ALPHA_COLOR);
				continue;
			}

			const W8 pixel0  = two_pixels&0xF;
			if (pixel0 != DRAW_ICON_ALPHA_COLOR)
			{
				for (W8 i=0; i< SCALE_; i++)
				for (W8 j=0; j< SCALE_; j++)
				{
					if (dotTransparent_)
					{
						if (parity & 1) draw_pixel(penX+i, penY+j, DRAW_ICON_PAL[pixel0]);
					}
					else
						draw_pixel(penX+i, penY+j, DRAW_ICON_PAL[pixel0]);

					//printf("%1x", pixel0);
				}
			}
			//else
			//{
			//	printf("%1x", DRAW_ICON_ALPHA_COLOR);
			//}
			const W8 pixel1  = two_pixels>>4;
			if (pixel1 != DRAW_ICON_ALPHA_COLOR)
			{
				for (W8 i=0; i< SCALE_; i++, parity++)
				for (W8 j=0; j< SCALE_; j++, parity++)
				{
					if (dotTransparent_)
					{
						if (parity & 1) draw_pixel(penX+SCALE_+i, penY+j, DRAW_ICON_PAL[pixel1]);
					}
					else
						draw_pixel(penX+SCALE_+i, penY+j, DRAW_ICON_PAL[pixel1]);
					//printf("%1x", pixel1);
				}
			}
			//else
			//{
			//	printf("%1x", DRAW_ICON_ALPHA_COLOR);
			//}
		}
		
	}
#endif
}


template<int SCALE_>
W0  draw_centered_icon(W32 iconId_, const SW16 offsetX_=0, const SW16 offsetY_=0, const W1 dotTransparent_=false)
{
	draw_icon<SCALE_>( ((DISP_WIDTH-DRAW_ICON_SIZE*SCALE_)>>1) + offsetX_, 
					   ((DISP_HEIGHT-DRAW_ICON_SIZE*SCALE_)>>1) + offsetY_, iconId_, dotTransparent_);
}


inline W0 initFontInfo(FontInfo& fi, W8 char_)
{
	#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
		fi = ((FontInfo*)fontInfos)[char_-32];
	#endif
	#if (APP_TARGET==APP_TARGET_ARDUINO_NANO)
		const W16 ascrow = (char_-32)*5;
		fi.ptrOffset = VL_FLASH_READ_WORD(fontInfos,ascrow);
		fi.ascender  = (SW8)(VL_FLASH_READ_BYTE(fontInfos,ascrow+2));
		fi.width     = VL_FLASH_READ_BYTE(fontInfos,ascrow+3);
		fi.height    = VL_FLASH_READ_BYTE(fontInfos,ascrow+4);
	#endif

}

Sz  draw_getTextSize(SW8* str_, const SW8 space_=fontDefaultSpace)
{
	FontInfo fi;
	if (space_==fontMonospaced)
		return Sz(strlen(str_)*fontBlockX, fontBlockY); // nao "\n"

	SW16 tY0 = 0;
	SW16 tY1 = 0;
	SW16 tX1 = 0;
	while(*str_)
	{
		const W8 ch = *str_;
		str_++;
		if (ch<32) 
		{
			continue;
		}
		if (ch==32)
		{
			tX1+=fontBlockX>>1;
			continue;
		}
		initFontInfo(fi, ch);
		const SW16 y0 =  - (SW16)fi.ascender;
		tY0 = VL_MIN(tY0, y0);
		tY1 = VL_MAX(tY1, y0+fi.height);
		tX1 = VL_MAX(tX1, (W8)((SW16)tX1+(SW16)fi.width+(SW16)space_));
	}
	return Sz(tX1, tY1-tY0);
}


//<!>
W0  draw_text_(const W8 x_,  W8 y_,  const SW8* str_, const W16 color_=0, 
			  const SW8 space_=fontDefaultSpace, 
			  const SW16 N_=-1, W1 wrap_=false, W8 scale_=1)
{
	FontInfo fi;
	W16 charIt=0;
	SW16 penX = x_;
	W8 width = 0;
	while( *str_ && ( (N_==-1) || (N_!=-1 && charIt<N_) ) ) 
	{
		const W8 ch = *str_; *str_++; charIt++;

		if (ch<=32)
		{
			if (ch==13)
			{
				penX=x_;
				y_+=fontBlockY;
			}
			else if (ch==32)
			{
				if (space_==fontMonospaced)
					penX+=fontBlockX;
				else
					penX+=fontBlockX>>1;
			}
			continue;
		}
			
		if (wrap_)
		{
			if ((penX+fontBlockX)>= DISP_WIDTH)
			{
				penX=x_;
				y_+=fontBlockY;
			}
		}

		#if (APP_TARGET==APP_TARGET_RASPBERRY_PICO)
			#define FONTSTREAM_DT W32
		#else
			#define FONTSTREAM_DT W16
		#endif

		initFontInfo(fi, ch);
		W8   penXX = penX;
		W8   penY = y_- (SW16)fi.ascender;
		FONTSTREAM_DT data;
		FONTSTREAM_DT streamPos;
		FONTSTREAM_DT stream0;
		FONTSTREAM_DT *ptr = ( FONTSTREAM_DT*)(fontMap + fi.ptrOffset);
		
		//DEBUG("ptr offset: %u \n", fi.ptrOffset);
		
		stream0 = ptr[0];
		streamPos = 0;
		getBitsInit<FONTSTREAM_DT>(ptr, stream0, streamPos);
		 
		while(1)
		{
			//fontGetBits(fontRLECodeBits)
			data = getBits<FONTSTREAM_DT,fontRLECodeBits>(&ptr, stream0, streamPos);
						
			switch(data)
			{
				case fontCodeIsEmpty:
				{
					goto skipNext;
				}
				break;
				case fontCodeIsCRLF: 
				{
					penXX=penX; penY++;
				}
				break;
				case fontCodeIsBack: 
				{
					data = getBits<FONTSTREAM_DT,fontRLEDataBits>(&ptr, stream0, streamPos);
					penXX+=data;
				}
				break;
				case fontCodeIsFront: 
				{
					data = getBits<FONTSTREAM_DT,fontRLEDataBits>(&ptr, stream0, streamPos);
					#ifdef DRAW_USES_BUFFER
						draw_pixelsRep(penXX,penY,color_,data);
					#else
						DISP_TXRAM(penXX,penXX+(data-1),penY,penY)
						disp_rep_function(data, (W0*)&color_);
					#endif
					penXX+=data;
				}
				break;
				default:
					fatal(DRAW, DRAW_ERROR_FONT_RLE);
				break;
				
			}
		}

	skipNext:
		if (space_==fontMonospaced)
			penX= (W8)((SW16)penX+fontBlockX);
		else
			penX= (W8)((SW16)penX+(SW16)fi.width+(SW16)space_);
	}

}

W0  draw_text(const W8 x_,  W8 y_,  const SW8* str_, const W16 color_=0, 
			  const SW8 space_=fontDefaultSpace, 
			  const SW16 N_=-1, W1 wrap_=false, W8 scale_=1)
{
	if (drawTextShadowX+drawTextShadowY)
	{
		draw_text_(x_ + drawTextShadowX, y_ + drawTextShadowY, str_, 
				  drawTextShadowColor, space_, N_, wrap_, scale_); 
	}
	if (drawTextOutline)
	{
		for (SW16 x0=(SW16)x_-drawTextOutline; x0<=(SW16)x_+drawTextOutline; x0++)
			for (SW16 y0=(SW16)y_-drawTextOutline; y0<=(SW16)y_+drawTextOutline; y0++)
				draw_text_(x0 ,y0,str_, drawTextOutlineColor, space_, N_, wrap_, scale_);
	}
	draw_text_(x_,y_,str_,color_, space_, N_, wrap_, scale_);

}
W0  draw_text_centered_with_offsets(const SW16 x_, const SW16 y_, const SW8* str_, const W16 color_=0, const SW8 space_=fontDefaultSpace)
{
	const Sz tsz = draw_getTextSize((SW8*)str_);
	const W8 x= (SW16)(DISP_WIDTH-tsz.width)/2 + x_;
	const W8 y= (SW16)(DISP_HEIGHT- tsz.height)/2 + tsz.height + y_;
		draw_text(x,y,str_,color_, space_);
}
W0  draw_text_centered(const SW16 x_, const SW16 y_, const SW8* str_, const W16 color_=0, const SW8 space_=fontDefaultSpace)
{
	const Sz tsz = draw_getTextSize((SW8*)str_);
	const W8 x= (x_==-1)?((SW16)(DISP_WIDTH-tsz.width)/2): x_;
	const W8 y= (y_==-1)?((SW16)(DISP_HEIGHT- tsz.height)/2 + tsz.height) : y_;
	draw_text(x,y,str_,color_, space_);
}

W0  draw_error_msg(SW8* str_)
{
	draw_clear(Color_Red);
#ifdef APP_USES_ICONS
	draw_icon<1>(DISP_WIDTH-18,3, ICON_ERROR);
#endif
	draw_text(2,19, str_, Color_Black, fontDefaultSpace, -1, true);
	draw_text(2,18, str_, Color_White, fontDefaultSpace, -1, true);
	DRAW_UPDATE
}


W0 draw_init(W1 initBuffers_=true)
{

		     
	//.... .... | .... .... | .... .... | .... ....

	display_init();
	#ifdef DRAW_TEST
		for (W16 i=0; i < 16; i++)
		{
			DISP_TXRAM(0,DISP_WIDTH-1,0,DISP_HEIGHT-1)
			W16 gray = DISP_COLOR_MK_W16(i,i,i);
			disp_txRepColor(DISP_WIDTH*DISP_HEIGHT, &gray);
			DELAY_IDLE(100);
		}
	#endif

	#ifdef DRAW_USES_BUFFER
		if (initBuffers_)
			draw_initBuffer();
		else
			return;
	#endif


	//DELAY_MS(20);
	draw_clear(0xFFFF);
	DRAW_UPDATE
}


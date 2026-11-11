
//	Definições de icones

//  1.  Fazer um bitmap 4-bpp (16 cores) com todos os icones 
//      um-a-um na horizontal (o bitmap pode ser mais largo
//      na vertical para marcar as posições dos icones)
//
//  2.	Gerar uiIconsData.hpp usando bmp2other.exe
//		 %DEV_TOOLS_PATH%\bmp2other chex icons.bmp uiIconsData.hpp -a=4 -l  -n=iconData -N="const W8 iconData[] VL_STORE_IN_FLASH VL_ALIGN(4) = "  


#ifdef APP_USES_ICONS
	#include "uiIconsData.hpp"
#endif

//  3. Enumerar os icones que existem no bitmap, deve
//     acabar com ICON_NUMBER

enum ICON { 
 ICON_FOLDER,
 ICON_QUESTION, 
 ICON_SD , 
 ICON_ERROR ,
 ICON_NONE,
 ICON_NUMBER};
 
//  4. Definir a paleta de cores

const W16 DRAW_ICON_PAL[]  VL_ALIGN(4) = { 
 0x0000, 
 0x000A, 
 0x00A0, 
 0x00AA, 
 0x0A00, 
 0x0B0B, 
 0x0A70, 
 0x0777, 
 0x0DDD, 
 0x077F, 
 0x07F7, 
 0x07FF, 
 0x0F77, 
 0x0F7F, 
 0x0FF7, 
 0x0FFF, 
 }; 


//  5. Definir transparência, tamanho, 
//     e a largura do bitmap de icones em bytes

#define DRAW_ICON_ALPHA_COLOR			12
#define DRAW_ICON_SIZE					16
#define DRAW_ICONS_MTX_WIDTH_IN_BYTES   iconData_COLS_BYTESZ
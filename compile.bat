call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
setlocal EnableDelayedExpansion
setx /M PATH "%PATH%;C:\Development\toolchains\gcc-arm\bin";

set SOURCE_PATH=E:\Hardware\head-sensors\head-sensors
set OTHER_INC_PATH=C:\Development\velociraptor\include\mcu
set SOURCE_ESP_FILE=%SOURCE_PATH%\esp_config.h 

set TARGET_PATH=C:\Development\toolchains\pico-sdk\pico-examples\project
set TARGET_SRC_DRV=E:
set TARGET_PATH_DRV=C:
set TARGET_USB_DRV=F:\
set TARGET_CONFIG_PATH=C:\Development\toolchains\pico-sdk\lib\tinyusb\src
set DEV_TOOLS_PATH=C:\Development\tools

@echo ************************************************
@echo ************************************************
@echo ************************************************
del %TARGET_PATH%\tusb_config.h 
del %SOURCE_PATH%\tusb_config.h
@echo.
@echo A PROCURAR MAIN.CPP POR DEFINE 'APP_USE_USB_PERIPHERALS 1'
%DEV_TOOLS_PATH%\stamp search  %SOURCE_PATH%\main.cpp -str="#define APP_USE_USB_PERIPHERALS 1"


if %ERRORLEVEL%==-1 (
	set /A OPTION_USB=0
	@echo ** SEM PERIFERICOS USB
	copy %SOURCE_PATH%\clists\CMAKELISTS.txt %SOURCE_PATH%\CMAKELISTS.txt 
) else (
	set /A OPTION_USB=1
	@echo ** COM PERIFERICOS USB
	copy %SOURCE_PATH%\clists\CMAKELISTS_USB.txt %SOURCE_PATH%\CMAKELISTS.txt 
	copy %SOURCE_PATH%\tusb_config\tusb_config.h %SOURCE_PATH%\tusb_config.h
)

@echo ************************************************
@echo *** GENERATING ICONS
@echo ************************************************
 %DEV_TOOLS_PATH%\bmp2other chex icons.bmp uiIconsData.hpp -a=4 -l  -n=iconData -N="const W8 iconData[] VL_STORE_FLASH_GROUP("ICONDATA") VL_ALIGN(4) = "  


@echo ************************************************
@echo .
@echo ************************************************
@echo *** COPYING TO TARGET FOLDER %TARGET_PATH% ...
@echo ************************************************

	del %TARGET_PATH%\*.* /F /Q
	rem del %TARGET_PATH%\build\*.* /F /Q
	mkdir %TARGET_PATH%
	mkdir %TARGET_PATH%\rasp_pico

	cd %SOURCE_PATH%
	copy %SOURCE_PATH%\*.* %TARGET_PATH%\  > nul
	copy %OTHER_INC_PATH%\*.* %TARGET_PATH%\ > nul
	copy %OTHER_INC_PATH%\rasp_pico\*.* %TARGET_PATH%\rasp_pico\*.* > nul
	copy %SOURCE_ESP_FILE%  %TARGET_PATH%\  > nul

	
@echo ************************************************
@echo *** ENTRAR NA PASTA TARGET *********************
@echo ************************************************

	%TARGET_PATH_DRV%
	cd %TARGET_PATH%

@echo ************************************************
@echo ************************************************
@echo ****     COMPILING...       ********************
@echo ************************************************
@echo ************************************************
 
	cmake -G "NMake Makefiles" 
	nmake
	IF %ERRORLEVEL% NEQ 0 GOTO process_error1

@echo ************************************************
@echo ************************************************
@echo ****    COPYING TO RASPBERRY PI PICO DRIVE  ****
@echo ************************************************

cd %TARGET_PATH%

@echo Raspberry pi pico
@echo Atencao RAM interna: 264KB / 2MB flash
dir  PROJECT.uf2 
copy PROJECT.uf2 %TARGET_USB_DRV%
IF %ERRORLEVEL% NEQ 0 GOTO process_error2 

@echo ************************************************
@echo *****    OK              ***********************
@echo ************************************************

GOTO process_end

:process_error3
@echo ******************************************
@echo ******* CMD ERROR ************************
@echo ******************************************
GOTO process_end

:process_error1
@echo ******************************************
@echo ******* COMPILE ERROR ********************
@echo ******************************************
GOTO process_end

:process_error2
@echo *******************************************
@echo ******* A PLACA NAO ESTA MONTADA! **********
@echo ********************************************
@echo * OU JA' NAO E A UNIDADE %TARGET_USB_DRV%? *
@echo *                                          *
@echo * OU ESTA DESLIGADA DO USB?                *
@echo *                                          *
@echo * MANTER O BOTAO DE PROGRAMACAO PREMIDO    *
@echo * E TOCAR RAPIDAMENTE NO BOTAO DE RESET    *
@echo ********************************************

@echo *******************************************
@echo ******* RASPBERRY PI PICO NOT MOUNTED ******
@echo ********************************************
@echo * ISNT AT DRIVE  %TARGET_USB_DRV%?         *
@echo *                                          *
@echo * ITS CONNECTED TO USB?                    *
@echo *                                          *
@echo * KEEP PRESSING PROGRAMMING BUTTON WHILE   *
@echo * TOUCHING RESET ONCE                      *
@echo ********************************************
exit

GOTO process_end


:process_end
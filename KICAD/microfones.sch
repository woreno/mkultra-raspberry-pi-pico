EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Amplifier_Operational:LM358 U1
U 1 1 65A9072A
P 12850 4075
F 0 "U1" H 12875 4350 50  0000 C CNN
F 1 "LM358" H 12925 4275 50  0000 C CNN
F 2 "GERAL:standard_DIP_8pins" H 12850 4075 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/lm2904-n.pdf" H 12850 4075 50  0001 C CNN
	1    12850 4075
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:LM358 U1
U 3 1 65A93F34
P 12850 2175
F 0 "U1" V 12945 2175 50  0000 C CNN
F 1 "LM358" V 12854 2175 50  0000 C CNN
F 2 "GERAL:standard_DIP_8pins" H 12850 2175 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/lm2904-n.pdf" H 12850 2175 50  0001 C CNN
	3    12850 2175
	0    -1   -1   0   
$EndComp
$Comp
L Device:CP C1
U 1 1 65A9B8A2
P 12225 3975
F 0 "C1" V 12400 3850 50  0000 C CNN
F 1 "2.2uF" V 12300 3800 50  0000 C CNN
F 2 "GERAL:standard_2pins" H 12263 3825 50  0001 C CNN
F 3 "~" H 12225 3975 50  0001 C CNN
	1    12225 3975
	0    -1   -1   0   
$EndComp
Wire Wire Line
	12375 3975 12550 3975
Connection ~ 12550 3975
$Comp
L Device:C C3
U 1 1 65AAE116
P 12800 3150
F 0 "C3" V 12625 3300 50  0000 C CNN
F 1 "10pF" V 12725 3325 50  0000 C CNN
F 2 "GERAL:standard_2pins_half" H 12838 3000 50  0001 C CNN
F 3 "~" H 12800 3150 50  0001 C CNN
	1    12800 3150
	0    1    1    0   
$EndComp
$Comp
L Device:R_Variable R5
U 1 1 65AB99AA
P 12850 3425
F 0 "R5" V 12725 3425 50  0000 C CNN
F 1 "75K" V 12650 3450 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 12780 3425 50  0001 C CNN
F 3 "~" H 12850 3425 50  0001 C CNN
	1    12850 3425
	0    1    -1   0   
$EndComp
Wire Wire Line
	12550 3150 12650 3150
Wire Wire Line
	12550 3150 12550 3425
Wire Wire Line
	12700 3425 12550 3425
Connection ~ 12550 3425
Wire Wire Line
	12550 3425 12550 3975
Wire Wire Line
	13150 4075 13150 3425
Wire Wire Line
	13150 3425 13000 3425
Wire Wire Line
	12950 3150 13150 3150
Wire Wire Line
	13150 3150 13150 3425
Connection ~ 13150 3425
$Comp
L Device:R R7
U 1 1 65AD27F0
P 13300 3425
F 0 "R7" V 13093 3425 50  0000 C CNN
F 1 "49" V 13184 3425 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 13230 3425 50  0001 C CNN
F 3 "~" H 13300 3425 50  0001 C CNN
	1    13300 3425
	0    1    1    0   
$EndComp
$Comp
L Device:R R9
U 1 1 65AD431E
P 13500 3575
F 0 "R9" H 13400 3575 50  0000 C CNN
F 1 "100K" H 13375 3500 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 13430 3575 50  0001 C CNN
F 3 "~" H 13500 3575 50  0001 C CNN
	1    13500 3575
	1    0    0    -1  
$EndComp
$Comp
L Device:R R11
U 1 1 65AD7EF2
P 7125 5500
F 0 "R11" H 7225 5500 50  0000 C CNN
F 1 "330" H 7275 5425 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 7055 5500 50  0001 C CNN
F 3 "~" H 7125 5500 50  0001 C CNN
	1    7125 5500
	1    0    0    -1  
$EndComp
Wire Wire Line
	13450 3425 13500 3425
$Comp
L Device:R_Variable R1
U 1 1 65ADB594
P 11950 3550
F 0 "R1" H 12150 3625 50  0000 C CNN
F 1 "13.5K" H 12200 3550 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 11880 3550 50  0001 C CNN
F 3 "~" H 11950 3550 50  0001 C CNN
	1    11950 3550
	1    0    0    -1  
$EndComp
Text Label 11950 3325 1    50   Italic 10
VCC
Wire Wire Line
	11950 3325 11950 3400
Text Label 13500 4100 3    50   Italic 10
GND
Wire Wire Line
	13500 3725 13500 4100
$Comp
L Device:Microphone_Condenser MK1
U 1 1 65AF9F15
P 11950 4175
F 0 "MK1" V 11700 4125 50  0000 L CNN
F 1 "Microphone_Condenser" V 11775 4075 50  0000 L CNN
F 2 "GERAL:standard_2pins" V 11950 4275 50  0001 C CNN
F 3 "~" V 11950 4275 50  0001 C CNN
	1    11950 4175
	1    0    0    -1  
$EndComp
Text Label 11950 4375 3    50   Italic 10
GND
$Comp
L Device:R R3
U 1 1 65B08C6F
P 12400 4525
F 0 "R3" V 12300 4575 50  0000 C CNN
F 1 "100K" V 12200 4525 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 12330 4525 50  0001 C CNN
F 3 "~" H 12400 4525 50  0001 C CNN
	1    12400 4525
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R4
U 1 1 65B08C75
P 12700 4525
F 0 "R4" V 12600 4550 50  0000 C CNN
F 1 "100K" V 12500 4500 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 12630 4525 50  0001 C CNN
F 3 "~" H 12700 4525 50  0001 C CNN
	1    12700 4525
	0    -1   -1   0   
$EndComp
Connection ~ 12550 4525
Wire Wire Line
	12550 4175 12550 4525
Text Label 12250 4525 3    50   Italic 10
VCC
Text Label 12900 4525 3    50   Italic 10
GND
Wire Wire Line
	12850 4525 12900 4525
Text Label 12550 4350 2    50   Italic 10
MLEVEL
Wire Wire Line
	12075 3975 11950 3975
Connection ~ 11950 3975
Connection ~ 13500 3425
Wire Wire Line
	11950 3700 11950 3975
Text Label 13150 2275 0    50   Italic 10
GND
Text Label 12550 2275 2    50   Italic 10
VCC
$Comp
L Device:C C5
U 1 1 65B3975D
P 12850 2525
F 0 "C5" V 12775 2675 50  0000 C CNN
F 1 "10pF" V 12675 2700 50  0000 C CNN
F 2 "GERAL:standard_2pins_half" H 12888 2375 50  0001 C CNN
F 3 "~" H 12850 2525 50  0001 C CNN
	1    12850 2525
	0    -1   -1   0   
$EndComp
$Comp
L Amplifier_Operational:LM358 U1
U 2 1 65B51940
P 12875 5900
F 0 "U1" H 12900 6175 50  0000 C CNN
F 1 "LM358" H 12950 6100 50  0000 C CNN
F 2 "GERAL:standard_DIP_8pins" H 12875 5900 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/lm2904-n.pdf" H 12875 5900 50  0001 C CNN
	2    12875 5900
	1    0    0    1   
$EndComp
$Comp
L Device:CP C2
U 1 1 65B51946
P 12250 5800
F 0 "C2" V 12425 5675 50  0000 C CNN
F 1 "2.2uF" V 12325 5625 50  0000 C CNN
F 2 "GERAL:standard_2pins" H 12288 5650 50  0001 C CNN
F 3 "~" H 12250 5800 50  0001 C CNN
	1    12250 5800
	0    -1   -1   0   
$EndComp
Wire Wire Line
	12400 5800 12575 5800
Connection ~ 12575 5800
$Comp
L Device:C C4
U 1 1 65B5194E
P 12825 4975
F 0 "C4" V 12650 5125 50  0000 C CNN
F 1 "10pF" V 12750 5150 50  0000 C CNN
F 2 "GERAL:standard_2pins_half" H 12863 4825 50  0001 C CNN
F 3 "~" H 12825 4975 50  0001 C CNN
	1    12825 4975
	0    1    1    0   
$EndComp
$Comp
L Device:R_Variable R6
U 1 1 65B51954
P 12875 5250
F 0 "R6" V 12750 5250 50  0000 C CNN
F 1 "75K" V 12675 5275 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 12805 5250 50  0001 C CNN
F 3 "~" H 12875 5250 50  0001 C CNN
	1    12875 5250
	0    1    -1   0   
$EndComp
Wire Wire Line
	12575 4975 12675 4975
Wire Wire Line
	12575 4975 12575 5250
Wire Wire Line
	12725 5250 12575 5250
Connection ~ 12575 5250
Wire Wire Line
	12575 5250 12575 5800
Wire Wire Line
	13175 5900 13175 5250
Wire Wire Line
	13175 5250 13025 5250
Wire Wire Line
	12975 4975 13175 4975
Wire Wire Line
	13175 4975 13175 5250
Connection ~ 13175 5250
$Comp
L Device:R R8
U 1 1 65B51964
P 13325 5250
F 0 "R8" V 13118 5250 50  0000 C CNN
F 1 "49" V 13209 5250 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 13255 5250 50  0001 C CNN
F 3 "~" H 13325 5250 50  0001 C CNN
	1    13325 5250
	0    1    1    0   
$EndComp
$Comp
L Device:R R10
U 1 1 65B5196A
P 13525 5400
F 0 "R10" H 13425 5400 50  0000 C CNN
F 1 "100K" H 13400 5325 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 13455 5400 50  0001 C CNN
F 3 "~" H 13525 5400 50  0001 C CNN
	1    13525 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	13475 5250 13525 5250
$Comp
L Device:R_Variable R2
U 1 1 65B5197F
P 11975 5375
F 0 "R2" H 12175 5450 50  0000 C CNN
F 1 "13.5K" H 12225 5375 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 11905 5375 50  0001 C CNN
F 3 "~" H 11975 5375 50  0001 C CNN
	1    11975 5375
	1    0    0    -1  
$EndComp
Text Label 11975 5150 1    50   Italic 10
VCC
Wire Wire Line
	11975 5150 11975 5225
Text Label 13525 5925 3    50   Italic 10
GND
Wire Wire Line
	13525 5550 13525 5925
$Comp
L Device:Microphone_Condenser MK2
U 1 1 65B5198A
P 11975 6000
F 0 "MK2" V 11725 5950 50  0000 L CNN
F 1 "Microphone_Condenser" V 11800 5900 50  0000 L CNN
F 2 "GERAL:standard_2pins" V 11975 6100 50  0001 C CNN
F 3 "~" V 11975 6100 50  0001 C CNN
	1    11975 6000
	1    0    0    -1  
$EndComp
Text Label 11975 6200 3    50   Italic 10
GND
Text Label 12575 6000 2    50   Italic 10
MLEVEL
Wire Wire Line
	12100 5800 11975 5800
Connection ~ 11975 5800
Connection ~ 13525 5250
Wire Wire Line
	13525 5250 13700 5250
Wire Wire Line
	11975 5525 11975 5800
Wire Wire Line
	12550 2275 12550 2525
Wire Wire Line
	12550 2525 12700 2525
Wire Wire Line
	13000 2525 13150 2525
Wire Wire Line
	13150 2525 13150 2275
$Comp
L Device:R R12
U 1 1 65B68DC4
P 7125 5800
F 0 "R12" H 7225 5800 50  0000 C CNN
F 1 "660" H 7275 5725 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 7055 5800 50  0001 C CNN
F 3 "~" H 7125 5800 50  0001 C CNN
	1    7125 5800
	1    0    0    -1  
$EndComp
Text Label 13700 5250 0    50   Italic 10
MIC_2_OUT
Wire Wire Line
	7100 6500 7400 6500
$Comp
L Device:R R14
U 1 1 65B7267C
P 7100 6650
F 0 "R14" H 7200 6650 50  0000 C CNN
F 1 "66" H 7250 6575 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 7030 6650 50  0001 C CNN
F 3 "~" H 7100 6650 50  0001 C CNN
	1    7100 6650
	1    0    0    -1  
$EndComp
$Comp
L Device:R R13
U 1 1 65B51970
P 7100 6350
F 0 "R13" H 7200 6350 50  0000 C CNN
F 1 "30" H 7250 6275 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 7030 6350 50  0001 C CNN
F 3 "~" H 7100 6350 50  0001 C CNN
	1    7100 6350
	1    0    0    -1  
$EndComp
Connection ~ 7125 5650
$Comp
L MCU_RaspberryPi_and_Boards:Pico U7
U 1 1 656079F2
P 4425 4875
F 0 "U7" H 4425 6090 50  0000 C CNN
F 1 "Pico" H 4425 5999 50  0000 C CNN
F 2 "pico:RPi_Pico_SMD_TH" V 4425 4875 50  0001 C CNN
F 3 "" H 4425 4875 50  0001 C CNN
	1    4425 4875
	1    0    0    -1  
$EndComp
Text Label 3725 4225 2    50   Italic 10
SD_CS
Text Label 3725 4325 2    50   Italic 10
SD_OUT
Text Label 3725 4725 2    50   Italic 10
SD_CLK
Text Label 3725 4825 2    50   Italic 10
SD_IN
Text Notes 3100 4816 2    66   Italic 13
 |\n |\n<\n |\n |\n 
Text Notes 3000 4491 2    66   Italic 13
SD e UART\n
Text Label 3725 5225 2    50   Italic 10
DISP_CLK
Text Label 3725 5325 2    50   Italic 10
DISP_OUT
Text Label 3725 5425 2    50   Italic 10
DISP_RES
Text Label 3725 5525 2    50   Italic 10
DISP_DC
Text Label 3725 5725 2    50   Italic 10
DISP_CS
Wire Wire Line
	3725 4625 3125 4625
Wire Wire Line
	3125 4625 3125 5125
Connection ~ 3125 5125
Wire Wire Line
	3125 5125 3725 5125
Wire Wire Line
	3725 4125 3125 4125
Wire Wire Line
	3125 4125 3125 4625
Connection ~ 3125 4625
Text Notes 3075 5891 2    66   Italic 13
 |\n |\n<\n |\n |\n |\n 
Text Notes 2975 5466 2    66   Italic 13
DISPLAY
Text Label 5125 4925 0    50   Italic 10
~MCU_RESET
Text Label 5625 6500 0    50   Italic 10
DISP_BRIGHT
Text Label 5125 4325 0    50   Italic 10
VCC_3.3V
Text Label 4425 6025 3    50   Italic 10
GND
Wire Wire Line
	3125 6250 4425 6250
Wire Wire Line
	4425 6250 4425 6025
Wire Wire Line
	3125 5125 3125 5625
Wire Wire Line
	3725 5625 3125 5625
Connection ~ 3125 5625
Wire Wire Line
	3125 5625 3125 6250
Text Notes 3425 5625 0    67   Italic 13
////
Text Notes 3425 5125 0    67   Italic 13
////
Text Notes 3450 4625 0    67   Italic 13
////
Text Notes 3450 4125 0    67   Italic 13
////
Text Notes 5125 4625 0    67   Italic 13
////
Text Notes 5125 5125 0    67   Italic 13
////
Text Notes 5125 5625 0    67   Italic 13
////
Text Notes 4125 4175 0    67   Italic 13
////////
Text Notes 4125 4650 0    67   Italic 13
////////
Text Notes 4175 5175 0    67   Italic 13
////////
Text Notes 4150 5675 0    67   Italic 13
////////
Wire Wire Line
	5925 5125 5925 5625
Wire Wire Line
	5125 5625 5925 5625
Wire Wire Line
	5125 5125 5925 5125
Wire Wire Line
	5125 4125 5925 4125
Text Label 5725 4125 0    50   Italic 10
GND
Text Notes 4500 4975 0    67   Italic 13
////
$Comp
L ESP8266:ESP-01v090 U8
U 1 1 6573347E
P 5350 2700
F 0 "U8" H 5350 3215 50  0000 C CNN
F 1 "ESP-01v090" H 5350 3124 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x04_P2.54mm_Horizontal" H 5350 2700 50  0001 C CNN
F 3 "http://l0l.org.uk/2014/12/esp8266-modules-hardware-guide-gotta-catch-em-all/" H 5350 2700 50  0001 C CNN
	1    5350 2700
	1    0    0    -1  
$EndComp
Text Label 6300 2850 0    50   Italic 10
SERIAL_TX
Text Label 4400 2550 2    50   Italic 10
SERIAL_RX
Text Label 6300 2550 0    50   Italic 10
GND
Text Label 4400 2850 2    50   Italic 10
VCC_3.3V
Connection ~ 5925 4625
Wire Wire Line
	5925 4125 5925 4625
Wire Wire Line
	5925 4625 5925 5125
Wire Wire Line
	5125 4625 5925 4625
Text Label 3725 5025 2    50   Italic 10
~MCU_IN
Connection ~ 5925 5125
Text Label 4400 2750 2    50   Italic 10
~MCU_RESET
Text Label 5125 4025 0    50   Italic 10
VCC
Wire Wire Line
	5125 4025 5400 4025
Wire Wire Line
	5425 3675 5300 3675
Wire Wire Line
	5400 4025 5400 3775
Wire Wire Line
	5400 3775 5425 3775
Wire Wire Line
	5125 3925 5300 3925
Wire Wire Line
	5300 3925 5300 3675
$Comp
L GERAL:flipSwitch U12
U 1 1 6567FA06
P 5475 3525
F 0 "U12" H 6153 3421 50  0000 L CNN
F 1 "flipSwitch" H 6153 3330 50  0000 L CNN
F 2 "GERAL:flipSwitch3" H 5575 3725 50  0001 C CNN
F 3 "" H 5575 3725 50  0001 C CNN
	1    5475 3525
	1    0    0    -1  
$EndComp
Text Label 5525 3925 0    30   Italic 6
POWER_USB_WITH_VCC
Text Label 5525 3475 0    30   Italic 6
DONT_POWER_USB_WITH_VCC
Text Label 3725 4025 2    50   Italic 10
PIN_BUZZER
Wire Wire Line
	3725 4025 2500 4025
Wire Wire Line
	5125 4425 5125 4325
Text Notes 3000 5025 0    67   Italic 13
guardar 
Text Notes 1725 4025 0    67   Italic 13
tocar speaker
Text Label 3725 4525 2    50   Italic 10
SERIAL_RX
Text Label 3725 4425 2    50   Italic 10
SERIAL_TX
Text Label 5125 5225 0    50   Italic 10
BUTTON_0
Text Label 5125 5325 0    50   Italic 10
BUTTON_1
Text Label 5125 5425 0    50   Italic 10
BUTTON_2(RIGHT)
Text Label 5125 5525 0    50   Italic 10
BUTTON_3
Text Label 5125 5825 0    50   Italic 10
PIN_LED
$Comp
L Connector:Conn_01x03_Female J2
U 1 1 65AD7BBD
P 10775 4100
F 0 "J2" H 10667 3775 50  0000 C CNN
F 1 "Conn_01x03_Female" H 10667 3866 50  0000 C CNN
F 2 "Connector_PinHeader_1.00mm:PinHeader_1x03_P1.00mm_Horizontal" H 10775 4100 50  0001 C CNN
F 3 "~" H 10775 4100 50  0001 C CNN
	1    10775 4100
	-1   0    0    1   
$EndComp
Text Label 6450 4225 2    50   Italic 10
MIC_1_OUT
Text Label 9100 4550 3    50   Italic 10
GND
Text Label 11975 5800 2    50   Italic 10
MIC_2_IN
Text Label 11950 3975 2    50   Italic 10
MIC_1_IN
Text Label 10975 4100 0    50   Italic 10
GND
Text Label 10975 4200 0    50   Italic 10
MIC_1_IN
Text Label 10975 4000 0    50   Italic 10
MIC_2_IN
$Comp
L Transistor_BJT:MPSA42 Q1
U 1 1 65BDC343
P 9000 4350
F 0 "Q1" H 9191 4396 50  0000 L CNN
F 1 "MPSA13" H 9191 4305 50  0000 L CNN
F 2 "Package_TO_SOT_THT:TO-92_Inline" H 9200 4275 50  0001 L CIN
F 3 "http://www.onsemi.com/pub_link/Collateral/MPSA42-D.PDF" H 9000 4350 50  0001 L CNN
	1    9000 4350
	1    0    0    -1  
$EndComp
$Comp
L Device:R R18
U 1 1 65BDE008
P 9100 4000
F 0 "R18" H 9170 4046 50  0000 L CNN
F 1 "100" H 9170 3955 50  0000 L CNN
F 2 "GERAL:standard_2pins" V 9030 4000 50  0001 C CNN
F 3 "~" H 9100 4000 50  0001 C CNN
	1    9100 4000
	1    0    0    -1  
$EndComp
$Comp
L Device:R R17
U 1 1 65BDEB8D
P 8650 4350
F 0 "R17" V 8857 4350 50  0000 C CNN
F 1 "47K" V 8766 4350 50  0000 C CNN
F 2 "GERAL:standard_2pins" V 8580 4350 50  0001 C CNN
F 3 "~" H 8650 4350 50  0001 C CNN
	1    8650 4350
	0    -1   -1   0   
$EndComp
$Comp
L Device:Speaker LS1
U 1 1 65BEAD5C
P 9300 3525
F 0 "LS1" H 9470 3521 50  0000 L CNN
F 1 "UltraSound Speaker" H 9470 3430 50  0000 L CNN
F 2 "GERAL:standard_2pins" H 9300 3325 50  0001 C CNN
F 3 "~" H 9290 3475 50  0001 C CNN
	1    9300 3525
	1    0    0    -1  
$EndComp
Text Label 8325 3525 2    50   Italic 10
3.3V
Wire Wire Line
	8325 3525 9100 3525
Text Label 9600 4150 3    50   Italic 10
GND
$Comp
L Device:C C6
U 1 1 65C0AC90
P 9600 4000
F 0 "C6" H 9715 4046 50  0000 L CNN
F 1 "C" H 9715 3955 50  0000 L CNN
F 2 "GERAL:standard_2pins_half" H 9638 3850 50  0001 C CNN
F 3 "~" H 9600 4000 50  0001 C CNN
	1    9600 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	9100 3850 9600 3850
Connection ~ 9100 3850
$Comp
L Device:D D1
U 1 1 65C1EC56
P 8800 4500
F 0 "D1" H 8800 4717 50  0000 C CNN
F 1 "D" H 8800 4626 50  0000 C CNN
F 2 "GERAL:standard_2pins" H 8800 4500 50  0001 C CNN
F 3 "~" H 8800 4500 50  0001 C CNN
	1    8800 4500
	0    1    1    0   
$EndComp
Connection ~ 8800 4350
Text Label 8800 4650 3    50   Italic 10
GND
Wire Wire Line
	9100 3625 9100 3850
$Comp
L Connector:Conn_01x03_Female J4
U 1 1 65C419FD
P 7650 4025
F 0 "J4" H 7542 3700 50  0000 C CNN
F 1 "Conn_01x03_Female" H 7542 3791 50  0000 C CNN
F 2 "Connector_PinHeader_1.00mm:PinHeader_1x03_P1.00mm_Horizontal" H 7650 4025 50  0001 C CNN
F 3 "~" H 7650 4025 50  0001 C CNN
	1    7650 4025
	-1   0    0    1   
$EndComp
Text Label 7850 4025 0    50   Italic 10
GND
Text Label 7850 4125 0    50   Italic 10
ULTRA_AUDIO_IN
Text Notes 7475 3100 0    180  Italic 36
Ultrasound\nSpeaker
Text Notes 7375 5000 1    80   Italic 16
________________________________________
Text Notes 10300 5675 1    80   Italic 16
________________________________________________________________
Text Notes 10400 2050 0    180  Italic 36
Microphones\n
Wire Wire Line
	7125 5650 7425 5650
Connection ~ 7100 6500
Text Label 14800 4475 0    50   Italic 10
VCC
Text Label 14200 4575 2    50   Italic 10
GND
$Comp
L Connector:Mini-DIN-6 J3
U 1 1 65B2E94C
P 14500 4475
F 0 "J3" H 14500 4108 50  0000 C CNN
F 1 "Mini-DIN-6" H 14500 4199 50  0000 C CNN
F 2 "GERAL:minidin5" H 14500 4475 50  0001 C CNN
F 3 "http://service.powerdynamics.com/ec/Catalog17/Section%2011.pdf" H 14500 4475 50  0001 C CNN
	1    14500 4475
	1    0    0    -1  
$EndComp
Wire Wire Line
	14200 4375 13700 4375
Wire Wire Line
	13700 4375 13700 3425
Wire Wire Line
	13500 3425 13700 3425
Wire Wire Line
	14200 4475 13700 4475
Wire Wire Line
	13700 4475 13700 5250
Text Label 7125 5350 2    50   Italic 10
MIC_1_OUT
Text Label 7100 6200 2    50   Italic 10
MIC_2_OUT
Text Label 7100 6025 2    50   Italic 10
GND
Wire Wire Line
	7125 5950 7125 6025
Wire Wire Line
	7125 6025 7100 6025
Text Label 7075 6875 2    50   Italic 10
GND
Wire Wire Line
	7100 6800 7100 6875
Wire Wire Line
	7100 6875 7075 6875
Text Label 7425 5650 0    50   Italic 10
MIC_1_ADC
Text Label 7400 6500 0    50   Italic 10
MIC_2_ADC
Text Label 5125 4525 0    50   Italic 10
MIC_1_ADC
Text Label 5125 4725 0    50   Italic 10
MIC_2_ADC
Text Label 7050 4325 0    50   Italic 10
VCC
Text Label 6450 4425 2    50   Italic 10
GND
$Comp
L Connector:Mini-DIN-6 J1
U 1 1 65C19FB0
P 6750 4325
F 0 "J1" H 6750 3958 50  0000 C CNN
F 1 "Mini-DIN-6" H 6750 4049 50  0000 C CNN
F 2 "GERAL:minidin5" H 6750 4325 50  0001 C CNN
F 3 "http://service.powerdynamics.com/ec/Catalog17/Section%2011.pdf" H 6750 4325 50  0001 C CNN
	1    6750 4325
	1    0    0    -1  
$EndComp
Text Label 6450 4325 2    50   Italic 10
MIC_2_OUT
Text Label 13700 3425 0    50   Italic 10
MIC_1_OUT
Text Label 7850 3925 0    50   Italic 10
3.3V
Text Label 5125 4825 0    50   Italic 10
ULTRA_SIGNAL_OUT
Text Label 8500 4350 2    50   Italic 10
ULTRA_SIGNAL_OUT
Text Label 5125 5725 0    50   Italic 10
ROTARY_ENC_CLK
Text Label 3725 5825 2    50   Italic 10
ROTARY_ENC_DATA
Text Label 5825 5425 0    50   Italic 10
ROTARY_ENC_SW
Wire Wire Line
	5825 5425 5125 5425
Text Notes 10550 2800 0    80   Italic 16
G=RR/RL\n\n
Text Notes 11975 3550 0    80   Italic 16
RL\n\n
Text Notes 12650 3500 0    80   Italic 16
RR\n\n
$EndSCHEMATC

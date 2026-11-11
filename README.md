# mkultra-raspberry-pi-pico
A stereo audio recorder and ultrasound player using Raspberry Pi Pico made in 2022

I have severe tinitus running my ears, sometimes suddenly i feel my ears popping. For fun i decided to make a device to record my ears. So this consists of a SD Card reader, a small display, piezo speaker, a set of buttons, a dial (rotary encoder), two small electrec microphones, a op-amp circuit for amplification, and a Raspberry Pi Pico to record the sound. After i added a ultrasonic speaker to sweep frequencies. The Raspberry Pi Pico gadget and the microphone amplifier goes into 2 separated boxes. The microphone amplifier can be disconnected from Raspberry Pi Pico box, and connected to a PC line jack for recording audio using the PC software instead.

The small electrec microphones (cushioned) enters one's ears, and goes to microphone amplifier box then to the Raspberry Pi Pico box where the user can control the recording. There's also an option that draws the microphone levels on screen, so the microphones gain can be ajusted thru the potentiometers inside the amplifier box conveniently. 

While recording, the user can save into a file timestamps generated when a button is pressed. In this way the user can take note when some strange tinitus sound appears by just pressing a button, and review later these timestamps along the recorded file.

The system uses fat filesystem, Pico DMA's ADC channels, two cores, and for performance reasons, it records fat clusters sequentially and writes the fat once the records ends, so the SDCard shall only have files created by this system. The sound is recorded stereo with a 12-bit amplitude quantization due to the Raspberry Pi Pico ADC limits, and the rate (samples per second) can be selected from the user interface. From the main menu user can put the ultrasonic speaker playing, sweeping frequencies with the dial (in this setup i used a cheap ultrasonic speaker (2.5€), capable of 36KHz so just a bit above the human ear limit, 20 kHz. 
 










'# mkultra-raspberry-pi-pico
A stereo audio recorder and ultrasound player using Raspberry Pi Pico

I have severe titinus in my ears, sometimes suddenly i feel my ears popping. For fun i decided to make a device record my ears. So this consists of a SD Card reader, a small display, a set of buttons, a dial (rotary encoder), two small electrec microphones, a op-amp circuit for amplification, a Raspberry Pi Pico to record the sound. After i added a ultrasonic speaker to sweep frequencies. 

While recording the sound, the system can save a into a file timestamps generated when the user press a button. In this way the user can take note when some strange tinitus sound appears by pressing a button. 

The system uses fat filesystem, it records fat clusters sequentially and just writes the fat for the file when records ends. The sound is recorded as stereo with a 12-bit amplitude due to the Raspberry Pi Pico ADC limits. From main menu user can put the ultrasonic speaker sweeping frequencies with the dial. 













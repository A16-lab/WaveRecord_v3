# WaveRecord_v3
AnalogDiscovery 2 code

PURPOSE:
The WaveRecord2 program is designed to work with AnalogDiscovery 2 in order to generate/record high-frequency signals. 
The duration of the signals is limited by the AnalogDiscovery 2 buffer capability, 
8192 – reception counts, 4096 – radiation counts, sampling rate up to 100 MHz. 
                                                                                                                                                                                   
                                                                                                                                                                                   
OPTION:                                                                                                                                                                       
- The program allows you to set as a radiated signal: radio pulse (monochrome) or LFM (sweep),                                                                            
  or an arbitrary-shaped signal specified in the file (InPut.dat).                                                                                                                     
- Based on the measurement results, a text file '%Y-%m-%dT%H%M%S.dat' is generated, the file name contains the measurement time.                                                                   
  The file contains all the measurement data necessary to restore the measurement results, as well as the measurement configuration file.,                                              
  the measurement results description file. The content of the file is mixed: the preamble is text, the measurement results are binary data.                                               
- At the moment: radiation/reception is implemented for two channels, the shapes of the emitted pulse are identical for both channels in the case of radio pulse and FM,                             
  in the case of specifying an arbitrary signal as a file, the dependence of the pulse shape for each channel can be set individually.                                               
                                                                                                                                                                                   
                                                                                                                                                                                   
IT IS NECESSARY TO SET: 
- pulse shape (fm_type = 0 monochrome, fm_type = 1 LPM, fm_type = 2 arbitrary);                                                                                                
- time parameters of the pulse (monochrome - fi_Hz [Hz] carrier frequency, LFM - {f0_Hz, f1_Hz} [Hz] the initial and final frequency of the spectrum);                                            
- the number of samples per radiation (nSamplesOut <= 4096);
- the number of samples per reception (nSamplesIn <= 8192);                                                                       
- time sampling step for radiation (fsOut_Hz [Hz]) and separately time sampling step for reception (fsIn_Hz [Hz])                                                     
  (ADC and DAC operate on a time grid of 100 MHz, respectively, the sampling step should be a multiple of this frequency);                                                                
- the amplitudes of the emitted signals for each channel ({MaxVolts0, MaxVolts1} <= 5[V]);                                                                                               
- maximum wave travel time (DT0 is set in abstract units [tick]: DT0 < 512 + 32768);                                                                                
- the delay time of the start of reception relative to the moment of emission (DT1 is set in abstract units [tick]: DT1 < DT0 - 1);                                                         
- time scale (TimeScale = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000}) determining the value of one tick = TimeScale * 1 [msec];                                      
- ADC sensitivity parameters (voltsRange0, voltsRange1 [V]) separately for each channel,                                                                                        
 if voltsRange = 5.0 [V] then the sensitivity of ADC = 0.32mV/count, if voltsRange = 50.0 [V] then the sensitivity of ADC = 3.58mV/count;                                            
- the number of radiation/reception cycles within a single TotalPing measurement (not normalized);                                                                                         
- interval between radiation/reception cycles within a single TimeDelay measurement [msec] (not normalized),                                                                             
  the real time between the radiation/reception cycles must be estimated using an oscilloscope over time intervals                                                                 
  between the pulses on the digital pin D0, the estimated time between the radiation/reception cycles is ~ 40 [msec] + TimeDelay [msec].                                                      
                                                                                                                                                                                   
                                                                                                                                                                                   
CONNECTION DIAGRAM:                                                                                                                                                                 
Analog inputs/outputs (reception/radiation) can be connected directly or via an expansion board (BNC connectors);                                                         
The ground is connected via an expansion board or directly;                                                                                                                     
It is necessary to connect:                                                                                                                                                             
digital output D0 <-> trigger input Trig1;                                                                                                                                         
digital output D1 <-> trigger input Trig2;                                                                                                                                         
The digital output D2 is used to externally trigger the measurement series.;                                                                                                               
The parameters of the external trigger pulse are as follows: a single pulse, voltage up to 3.3 [V], and the start of measurements at the failing edge after 1.5 ms.                                
                                                                                                                                                                                   
                                                                                                                                                                                   
THE ALGORITHM OF OPERATION:                                                                                                                                                                   
After the arrival of the external clock pulse to the digital pin D2 (3.3 [V]), a series of measurements takes place along the falling edge of the clock pulse.                                        
A total of TotalPing measurements are performed in one measurement series. The number of measurement series is unlimited.                                                                            
The interval between the measurement series is at least 1 second, which allows you to write a file with a different name.                                                                                         
At TimeScale*DT0 [usec] time intervals, a clock pulse is applied to the digital output D0, which generates radiation.                                                
After the clock pulse on D0, after the TimeScale*DT1 [usec] time interval, a clock pulse is applied to the digital pin D1, which is used for data logging.          
The digital pin D0 is connected to the input of the trigger Trig1, and radiation occurs on the trigger Trig1.                                                                            
The digital pin D1 is connected to the input of the trigger Trig2, and data is recorded at the signal on the trigger Trig2.                                                                   
Thus, the TimeScale * DT1 [usec] time interval is the delay time between radiation and reception.                                                                           
The TimeScale * DT0 [usec] time interval determines the maximum delay between the start of radiation and the start of data reception.                                                       
                                                                                                                                                                                   

WORK FEATURES:                                                                                                                                                                
Apparently, the AnalogDiscovery 2 ADC operates at a frequency of 100 MHz, and there is a random delay
of about 220 [nsec] between the rising edge of the signal at the input of the Trig2 trigger and the start of data recording.                                                                  
For convenience, a voltage of +3.3[V]/0.1[A] is output to the V+ power pin, which can be used to form a isolation circuit D2 from external equipment.            
                                                                                                                                                                                   
                                                                                                                                                                                   
PROGRAM OPTIONS:                                                                                                                                                                   
WaveRecord3.exe --help => outputs this help and saves it as a READ-ME file.TXT;                                                                         
WaveRecord3.exe --file config.dat => runs the program for execution with the parameters specified in the configuration file 'config.dat';                                        
WaveRecord3.exe --example => writes an example configuration file 'config.dat' and an example script, matlab's 'plotResult2.m',                                            
                                          which builds measurement results based on the file '%Y-%m-%dT%H%M%S.dat',                                                              
                                          it also contains an example of the InPut.dat file formation.                                                                                   
                                                                                                                                                                                   
                                                                                                                                                                                   
FILES REQUIRED FOR OPERATION:                                                                                                                                                      
                                                                                                                                                                                   
Required:                                                                                                                                                                      
‘WaveRecord3.exe ’ is an executable file, before it is put into operation,                                                                                                                         
                    it is advisable to check the connection of AnalogDiscovery 2 using the standard WaveForms program;                                                                      
                                                                                                                                                                                   
‘config.dat’ is a file for setting the configuration parameters of measurements.;                                                                                                                 
                                                                                                                                                                                   
‘plotResult3.m' is a matlab code file for reading measurement data;                                                                                               
                                                                                                                                                                                   
Additional:                                                                                                                                                                    
‘DESCR.TXT’ is a text file, in any form, with a description of the measurements being performed.                                                                                              
              The contents of this file are included in the measurement results file %Y-%m-%dT%H%M%S.dat.                                                                                
              The file DESCR.TXT - MUST NOT CONTAIN A string of the form "===============",                                                                                                  
              because it is used in the result file to separate text data from binary data.                                                                          
                                                                                                                                                                                   
                                                                                                                                                                                   
                                                                                                                                                                                   
FILE FORMAT:                                                                                                                                                                     
‘config.dat' - is set before the start of measurements
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
5            % TimeScale // 1 => (1 tick == 1 us); 10 => (1 tick == 10 us); 100 => (1 tick == 0.1 ms); 1000 => (1 tick == 1 ms);                                               
1000         % DT0  [tick]   //Max value = 512 + 32768 [tick]; The DT0 Must be: (DT0 > WAVE PROPAGATION TIME)                                                                      
0            % DT1  [tick]   //Must be DT1: DT1 < DT0 - PULSE_WIDTH, where PULSE_WIDTH == 1; The time delay between radiation and reception                                        
25           % TotalPing     //The total number of measurement series                                                                                                              
10           % TimeDelay     //The measurments series param. // [ms] Time delay between measurments series, real time delay ~ 40 ms + TimeDelay                                    
3.75         % MaxVolts0     //[V] MaxVolts0 <= 5; DAC0 range                                                                                                                      
2.5          % MaxVolts1     //[V] MaxVolts1 <= 5; DAC1 range                                                                                                                      
50.0         % voltsRange0   //[V] voltsRange0 = {5.0 OR 50.0}; ADC0 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV; 
50.0         % voltsRange1   //[V] voltsRange1 = {5.0 OR 50.0}; ADC1 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV; 
0            % fm_type       //The emission signal type // fm_type = 0 -> mono pulse; fm_type = 1 -> sweep pulse; fm_type = 2 -> custom (read from file)                           
12500000.0   % fi_Hz         //[Hz] The carrier frequency of the monochromatic pulse                                                                                               
10000000.0   % f0_Hz         //[Hz] Start frequency band of sweep signal                                                                                                           
12500000.0   % f1_Hz         //[Hz] Stop frequency band of sweep signal                                                                                                            
4000         % nSamplesOut   //nSamplesOut <= 4096;                                                                                                                                
8192         % nSamplesIn    //nSamplesIn <= 8192;                                                                                                                                 
100000000.0 % fsIn_Hz //[Hz] Input sampling frequency - the sampling frequency of the input signal                                                                              
100000000.0 % fsOut_Hz //[Hz] Output sampling frequency - the sampling frequency of the output signal                                                                            
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------


// --- Parameters that determine the delay of the moment when the recording of signals begins relative to the moment of radiation --------------------                                                         
uint32 TimeScale - defines the timeline for {DT0, DT1} if TimeScale = 1, {DT0, DT1} is set in [usec] (1 tick == 1 [usec]),                                             
                   if TimeScale = 1000, {DT0, DT1} is set in [msec] (1 tick == 1 [msec]);                                                                                       
uint32 DT0 [tick] is the time interval that must be longer than the delay time of the measurement moment relative to the moment of radiation, limitations: DT0 < 512 + 32768;              
uint32 DT1 [tick] is the time interval between the moment of emission and the moment of the beginning of recording signals, the limitations of DT1 < DT0 - 1;                                                        
                                                                                                                                                                                   
// --- Parameters that determine the number of radiation/reception cycles and the time interval between measurement cycles -------------                                                         
uint32 TotalPing - the total number of radiation/reception cycles within one series is not normalized.;                                                                                  
uint32 TimeDelay is an outdated parameter, left for compatibility                                                                                                                 
The time interval [msec] between measurement cycles (radiation/reception) is determined by the product TimeScale*DT0 [msec],                                                               
                                                                                                                                                                                   
// --- Parameters defining the configuration of ADC/DAC channels --------------------------------------------------------------                                                         
double MaxVolts0 [V] - signal amplitude [V] DAC0, limits MaxVolts0 <= 5[V];                                                                                               
double MaxVolts1 [V] - signal amplitude [V] DAC1, limits MaxVolts1 <= 5[V];                                                                                               
double voltsRange0 [V] - voltsRange0 = {5.0 or 50.0} dynamic range of ADC0, the specified range corresponds to 14 bits of the ADC scale;                                             
double voltsRange1 [V] - voltsRange1 = {5.0 or 50.0} dynamic range of ADC1, the specified range corresponds to 14 bits of the ADC scale;                                             
                                                                                                                                                                                   
// --- Parameters that determine the shape of the emitted DAC signal --------------------------------------------------------------                                                        
uint32 fm_type - defines the type of the emitted signal {0 - monochrome, 1 - LCHM, 2 - specified by the InPut.dat file}                                                                         
double fi_Hz [Hz] is the carrier frequency of the radio pulse;                                                                                                                                 
double f0_Hz [Hz] - the initial frequency of the LF spectrum;                                                                                                                                 
double f1_Hz [Hz] - the final frequency of the LF spectrum;                                                                                                                                  
                                                                                                                                                                                   
// --- Parameters defining the ADC/DAC time grid -------------------------------------------------------------------                                                          
uint32 nSamplesOut [smpl] - number of samples in the emitted signal, nSamplesOut limits <= 4096;                                                                             
uint32 nSamplesIn [smpl] - the number of samples in the received signal, nSamplesIn limits <= 8192;                                                                             
double fsIn_Hz [Hz] - ADC frequency grid, fsIn_Hz limits must be a multiple of 100 MHz;                                                                                   
double fsOut_Hz [Hz] - DAC frequency grid, fsOut_Hz limits must be a multiple of 100 MHz;                                                                                  
                                                                                                                                                                                   
                                                                                                                                                                                   
'InPut.dat' - is set before measurements, describes the shape of the signals applied to DAC0, DAC1                                                                                            
// ------------------------------------------------------------------------------                                                                                                  
 0.0000000e+00 0.0000000e+00                                                                                                                                                       
 6.2831440e-03 1.2566040e-02                                                                                                                                                       
 1.2566040e-02 2.5130095e-02                                                                                                                                                       
 1.8848440e-02 3.7690183e-02                                                                                                                                                       
 2.5130095e-02 5.0244318e-02                                                                                                                                                       
 3.1410759e-02 6.2790520e-02                                                                                                                                                       
 3.7690183e-02 7.5326806e-02                                                                                                                                                       
 4.3968118e-02 8.7851197e-02                                                                                                                                                       
 5.0244318e-02 1.0036171e-01                                                                                                                                                       
 5.6518534e-02 1.1285638e-01                                                                                                                                                       
 6.2790520e-02 1.2533323e-01                                                                                                                                                       
 6.9060026e-02 1.3779029e-01                                                                                                                                                       
 .......................................................                                                                                                                           
// ------------------------------------------------------------------------------
the first column is the unit-normalized waveform applied to DAC0                                                                                                        
the second column is the unit-normalized waveform applied to DAC1                                                                                                        
                                                                                                                                                                                   
The amplitude of the signals is described by the parameters {MaxVolts0, MaxVolts1} of the file 'config.dat';                                                                                              
The time grid is described by the fsOut_Hz parameter of the 'config.dat' file;                                                                                                                
The number of samples (lines of the file) must match the nSamplesOut parameter of the 'config.dat' file and be greater;

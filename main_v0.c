/* READ-ME.TXT

Программа запуска AD2:

выходной массив 1 х 4096 отсчета, входной массив 2 х 8192 отсчета

частота дискретизации входного сигнала 1 МГц
частота выходной дискретизации задается через время излучения

на выход последовательно задается ЛЧМ, а потом набор синусоид


*/

/*
HOW TO COMPILE DIGILENT C - CODE APPLICATION

2. I have compiled  WaveFormsSDK  examples by Microsoft Visual Studio 2013 on Winx64, by:

2.0. Create "Win 32 Console Application Project".

2.1. While config "Win 32 Application Wizard" -- > Application Settings->Additional options->Unmark "Precompiled header".

2.3. Delete all  default "header & source" files from project.

2.4. Copy  "sample.h, dwf.h, analogin_sample.c, dwf.lib (from Digilent/WaveFormsSDK/lib/x64)" to project catalog(may be to .. / include, .. / lib subcatalogs).

2.5. Add  "sample.h, dwf.h, analogin_sample.c" to project.

2.6. Edit "sample.h"


#include <stdio.h>
#include <stdlib.h>

 #ifdef WIN32
 //  #include "../../inc/dwf.h"
 #include "dwf.h"
 #else
 //  #include <digilent/waveforms/dwf.h>
 #include "dwf.h"
 #endif

 #ifdef WIN32
 #include <windows.h>
 #define Wait(ts) Sleep((int)(1000*ts))
 #else
 #include <unistd.h>
 #include <sys/time.h>
 #define Wait(ts) usleep((int)(1000000*ts))
 #endif
 ...............................

2.7  Project Properties-- > In "Configuration Manager"  (top, right button)  it is necessary to create a configuration "x64"

2.7.1.Configuration Manager-- > Active solution platform-- > New Solution Platform-- > Type or selected the new platform-- > x64
2.7.2.Active solution platform-- > must be x64
2.7.3.Active solution configuration-- > must be x64

2.8.Project Properties-- > C\C++   -- > General-- > Additional Include Directories-- > "To add the catalog with:  sample.h, dwf.h   files"

2.9.Project Properties-- > C\C++   -- > All Options-- > Calling Convention-- > "__cdecl (/Gd)"

2.10.Project Properties-- > C\C++   -- > All Options-- > Compile as-- > "Compile as C code (/TC)"

2.11.Project Properties-- > Linker-- > General-- > Additional Library Directories-- > "To add the catalog with  dwf.lib  file"

2.12.Project Properties-- > Linker-- > Input-- > Additional Dependencies-- > "To add the 'dwf.lib;' string  in file list"

2.13  Пункты 2.11 и 2.12 должны быть выполнены для обоих типов компиляции(Debug & Release)




COMPILATION ERROR: Previous IPDB not found, fall back to full compilation

TRICK:
The way to fix this is Visual Studio is to look at the library build options and change the linker options.

Linker --> Optimization --> Use Fast Link Time Code Generation (/LTCG:incremental)

Remove the incremental in the linker options

Linker --> Optimization --> Use Fast Link Time Code Generation (/LTCG)

Situation: One library project One Windows Application project Library is created using default settings and linked to windows application project. Program is working fine. But in release build of the library produces the same error as above.


*/

#define _CRT_SECURE_NO_WARNINGS

//#define REFACTORING
#ifdef REFACTORING
#else

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h> // russian lang

#include "sample.h"
#include "dwf.h"

#define PWR_SUPPLY 	// Power supply 
#define DIGITAL_OUT // Digital clk
#define DIGITAL_IN  // DIO - Input trigger
#define ANALOG_OUT 
#define ANALOG_IN 
#define FILE_OUT
#define CFG_FILE

#define nSamplesInMax 8192
#define nSamplesOutMax 4096

#define VOLTS_RANGE_LOW 5.0
#define VOLTS_RANGE_HIGH 50.0

#define strLength    32
#define bufferLength (4*strLength*strLength)

//#define printf mexPrintf

double tOut[nSamplesOutMax];
double tIn[nSamplesInMax];

double xt0[nSamplesOutMax];
double xt1[nSamplesOutMax];

double xr0[nSamplesInMax];
double xr1[nSamplesInMax];

size_t ret_code; // число записаных данных  fwrite

// --- буферизация ввода\вывода -----------
#define U8_4MB_COUNT			4194304
char buff[U8_4MB_COUNT];
// --- буферизация ввода\вывода -----------


char szError[512] = { 0 };

HDWF hdwf;
STS sts;

int true_mex = 1;
int false_mex = 0;
int idxCh0 = 0; // Channel 0
int idxCh1 = 1; // Channel 1
int idxCh2 = 2; // Channel 2
int idxCh3 = 3; // Channel 3
int idxCh4 = 4; // Channel 4
int idxCh5 = 5; // Channel 5
int idxCh6 = 6; // Channel 6
int idxCh7 = 7; // Channel 7

#define CLK_DIV_1MHz  1000000

double pi = 3.1415926535897932384626433832795;

// --- User define DigitalOut parameters -------------------------------------
unsigned int TimeScale = 0; // 1 => (1 tick == 1 us);  10 => (1 tick == 10 us);  100 => (1 tick == 0.1 ms);  1000 => (1 tick == 1 ms);
unsigned int PULSE_WIDTH = 1;
unsigned int DT0 = 0; // [tick] max = 512 + 32768 [tick]  The DT0 MUST BE: (DT0 > WAVE PROPAGATION TIME)
unsigned int DT1 = 0;   // [tick] (DT1 < DT0 - PULSE_WIDTH) The time delay between radiation and reception
// --- User define DigitalOut parameters -------------------------------------

// --- User define Analog Input\Output parametrs -----------------------------
double MaxVolts0 = 0.0; // MaxVolts < 5; DAC range
double MaxVolts1 = 0.0; // MaxVolts < 5; DAC range

double voltsRange0 = 0.0; // VOLTS_RANGE_LOW;  // voltsRange0 = 5.0;  ADC range
double voltsRange1 = 0.0; // VOLTS_RANGE_HIGH; // voltsRange1 = 50.0; ADC range

unsigned int fm_type = 0; // fm_type = 0 -> mono pulse; fm_type = 1 -> sweep pulse; fm_type = 2 -> custom (read file)

unsigned int ping = 0;
unsigned int TotalPing = 0;
clock_t TimeDelay = 0; // [ms] Time delay between measurments series, real time delay ~ 40 ms + TimeDelay

unsigned int nSamplesOut = 0 * nSamplesOutMax;
unsigned int nSamplesIn = 0 * nSamplesInMax;

double fsIn_Hz = 0.0;             // [Hz] Input sampling frequency - частота дискретизации входного сигнала
double fsOut_Hz = 0.0;            // [Hz] Output sampling frequency - частота дискретизации выходного сигнала

double TsOut_s = 0 * 1.0e-8;                  // [s] Output sampling time
double Ts_sec = 0 * 1.0e-8 * nSamplesOutMax;  // [s] Signal emission time - время излучения, частота дискретизации определяется через время излучения Ts_sec
double TsIn_s = 0 * 1.0e-8;                   // [s] Input sampling time

// mono pulse
double fi_Hz = 0 * 1.0e+7; // [Hz] The carrier frequency of the monochromatic pulse

//sweep
double f0_Hz = 0 * 0.75e+7; // [Hz] Start frequency band of signal 
double f1_Hz = 0 * 1.25e+7; // [Hz] Stop frequency band of signal  
// ---------------------------------------------------------------------------------

// --- PWR_SUPPLY ------------------------------------------------------------------
double vUSB, aUSB, vAUX, aAUX;
int fOn;
// --- PWR_SUPPLY ------------------------------------------------------------------


void operation_parameters_print(void);
int command_line_parsing(int argc, char* argv[]);
void help_print(void);
void example_print(void);
void config_parsing(const char* filename);

// Pauses for a specified number of milliseconds.
void sleep(clock_t wait)
{
	clock_t goal;
	goal = wait + clock();
	while (goal > clock());
}

int main(int argc, char* argv[])
{
	clock_t start, finish;
	double  duration;

	double hzSys; // The internal clock frequency for DigitalOut 
	unsigned int clk_out_div; // Sets the divider value of the specified channel.
	unsigned int clk_in_div; // Sets the divider value of the specified channel.

	// --- Print argc\argv ---------------
	printf("Print argc/argv\n\r");
	printf("argc = %d\n", argc);
	for (int i = 0; i <= argc - 1; i++)
	{
		printf("argv[%u] = %s\n\r", i, argv[i]);
	}
	// --- Print argc\argv ---------------

	if (command_line_parsing(argc, argv) != 0)
	{
		return -1;
	}


	// ---- REFACTORING ----------------------------------------------------------------
#ifndef  CFG_FILE

	TimeScale = 10; // 1 => (1 tick == 1 us);  10 => (1 tick == 10 us);  100 => (1 tick == 0.1 ms);  1000 => (1 tick == 1 ms);
	DT0 = 5000; // [tick] max = 512 + 32768 [tick]  The DT0 MUST BE: (DT0 > WAVE PROPAGATION TIME)
	DT1 = 0;   // [tick] (DT1 < DT0 - PULSE_WIDTH) The time delay between radiation and reception
	PULSE_WIDTH = 1; //[tick]
	TotalPing = 500;
	
	MaxVolts0 = 2.0; // MaxVolts < 5; DAC range
	MaxVolts1 = 1; // MaxVolts < 5; DAC range
	voltsRange0 = VOLTS_RANGE_LOW;  // voltsRange0 = 5.0;  ADC range
	voltsRange1 = VOLTS_RANGE_HIGH; // voltsRange1 = 50.0; ADC range
	fm_type = 0; // fm_type = 0 -> mono pulse; fm_type = 1 -> sweep pulse; fm_type = 2 -> custom (read file)

	// mono pulse
	fi_Hz = 12.5e+6; // [Hz] The carrier frequency of the monochromatic pulse

	//sweep
	f0_Hz = 10.0e+6; // [Hz] Start frequency band of signal 
	f1_Hz = 12.5e+6; // [Hz] Stop frequency band of signal 	


	nSamplesOut = 4000;// nSamplesOutMax;
	nSamplesIn = nSamplesInMax;

	fsIn_Hz = 100000000.0;				// [Hz] Input sampling frequency - частота дискретизации входного сигнала
	fsOut_Hz = 100000000.0;				// [Hz] Output sampling frequency - частота дискретизации выходного сигнала

	TsOut_s = 1.0 / fsOut_Hz;           // [s] Output sampling time
	Ts_sec = TsOut_s * nSamplesOut;     // [s] Signal emission time - время излучения, частота дискретизации определяется через время излучения Ts_sec
	TsIn_s = 1.0 / fsIn_Hz;             // [s] Input sampling time

	TsOut_s = 1.0 / fsOut_Hz;           // [s] Output sampling time
	Ts_sec = TsOut_s * nSamplesOut;     // [s] Signal emission time - время излучения, частота дискретизации определяется через время излучения Ts_sec
	TsIn_s = 1.0 / fsIn_Hz;             // [s] Input sampling time

	// The output signal time [s]
	for (unsigned int i = 0; i < nSamplesOut; i++)
	{
		tOut[i] = TsOut_s * i;
	}

	// The input signal time [s]
	for (unsigned int i = 0; i < nSamplesIn; i++)
	{
		tIn[i] = (1.0e-6) * TimeScale * DT1 + TsIn_s * i;
	}

	ping = 0;
	BOOL fRepeatTrigger = 0;

#endif //CFG_FILE

	// ---- OpenDigilent ---------------------------------------------------------------
	if (!FDwfDeviceOpen(-1, &hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		fprintf(stderr, "Device open failed\n\t%s", szError);
		return -1;
	}
	// ---- OpenDigilent ---------------------------------------------------------------

	// --- power supply ----------------------------------------------------------------
#ifdef PWR_SUPPLY

	//Output Voltage					0.5V to 5V or -0.5V to - 5V
	//Maximum Power(USB)  				500mW total
	//Maximum Power(External Supply) 	2.1W per channel
	//Maximum Current					700mA per channel when using external power

	// FDwfAnalogIOChannelNodeSet(	HDWF hdwf, int idxChannel, int idxNode, double value)
	// Sets the node value for the specified node on the specified channel.
	//	- idxNode – Node index.
	//	- idxChannel – Analog I / O channel index of the device.
	//	- value – Value to set.

	//enable positive supply 
	FDwfAnalogIOChannelNodeSet(hdwf, 0, 0, 1);
	// set positive voltage 3.3V or 5V
	FDwfAnalogIOChannelNodeSet(hdwf, 0, 1, 3.3);
	// set current limitation 0 and 0.4A
	FDwfAnalogIOChannelNodeSet(hdwf, 0, 2, +0.1);

	//enable negative supply
	//FDwfAnalogIOChannelNodeSet(hdwf, 1, 0, 1);
	// set negative  voltage 3.3V or 5V
	//FDwfAnalogIOChannelNodeSet(hdwf, 1, 1, -5.0);
	// set current limitation 0 and 0.4A
	//FDwfAnalogIOChannelNodeSet(hdwf, 1, 2, -0.05);

	//master enable
	FDwfAnalogIOEnableSet(hdwf, true_mex);

#endif //PWR_SUPPLY
	// --- power supply ----------------------------------------------------------------

	// -------- DigitalIn channel 2 sets ---------------------------------------------
#ifdef	DIGITAL_IN

	FDwfDigitalOutInternalClockInfo(hdwf, &hzSys);

	clk_in_div = (unsigned int)(hzSys / CLK_DIV_1MHz); // prescaler to 1MHz, SystemFrequency/CLK_DIV_1MHz

	// sample rate = system frequency / divider, 100MHz/100 = 1000kHz
	FDwfDigitalInDividerSet(hdwf, clk_in_div);
	FDwfDigitalInTriggerSourceSet(hdwf, trigsrcDetectorDigitalIn);
	// trigger detector mask: low &  hight & ( rising | falling )
	FDwfDigitalInTriggerSet(hdwf, 0, 0, 4, 0); // On (DOI 2 - rising) trigger set
	FDwfDigitalInTriggerResetSet(hdwf, 0, 0, 0, 4); // On (DOI 2 - falling) trigger reset

	/*
	ping = 0;
	while (1)
	{
		ping = ping + 1;
		printf("Starting record %u...\n", ping);
		// begin acquisition
		FDwfDigitalInConfigure(hdwf, 0, 1);
		do
		{
			//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
			//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
			//stsError = 8;		stsBusy = 9;		stsStop = 10;
			//	- fReadData – TRUE if data should be read.
			FDwfDigitalInStatus(hdwf, 0, &sts);
		} while (sts != stsDone);
		printf("DigitalInStatus - 1 sts = %d\n", sts);
	}

	printf("done\n");

	FDwfDeviceClose(hdwf);
	return 0;
	*/

#endif // DIGITAL_IN
	// -------- DigitalIn channel 2 sets ---------------------------------------------

	// ---- DigitalOut: clk configure; channel 0 sets ; channel 1 sets -----------------
#ifdef DIGITAL_OUT

	unsigned int vLow; // tick low
	unsigned int vHigh; // tick high
	unsigned int fHigh; // Start high
	unsigned int start_div_cnt; //Divider initial value.

	// ---- DigitalOut The internal clock frequency for DigitalOut ---------------------
	FDwfDigitalOutInternalClockInfo(hdwf, &hzSys);
	//printf("SystemFrequency [Hz]: hzSys = %8.4e \n", hzSys);

	clk_out_div = TimeScale * (unsigned int)(hzSys / CLK_DIV_1MHz); // prescaler to 1MHz, SystemFrequency/CLK_DIV_1MHz

	//Sets the repeat count
	unsigned int cRepeat = TotalPing;
	FDwfDigitalOutRepeatSet(hdwf, cRepeat);

	//Sets the run length for the instrument in Seconds.
	//secRun – Run length to set expressed in seconds.
	double dT = (double)clk_out_div / hzSys; // [sec] clk sample
	double secRun = dT * DT0 * cRepeat;
	FDwfDigitalOutRunSet(hdwf, secRun);

	//Sets the repeat trigger option.
	//To include the trigger in wait - run repeat cycles, set fRepeatTrigger to TRUE.
	//fRepeatTrigger – Boolean used to specify if the trigger should be included in a repeat cycle
	BOOL fRepeatTrigger = 0;
	FDwfDigitalOutRepeatTriggerSet(hdwf, fRepeatTrigger);
	// ---- DigitalOut The internal clock frequency for DigitalOut ---------------------


	// -------- DigitalOut channel 0 sets ----------------------------------------------
	// Sets the initial divider value of the specified channel.
	FDwfDigitalOutDividerInitSet(hdwf, idxCh0, clk_out_div);
	// Sets the divider value of the specified channel.
	FDwfDigitalOutDividerSet(hdwf, idxCh0, clk_out_div);

	// Sets the initial state and counter value of the specified channel.
	fHigh = 0; // Start high
	start_div_cnt = (DT0 - PULSE_WIDTH) - DT1; //Divider initial value.
	FDwfDigitalOutCounterInitSet(hdwf, idxCh0, fHigh, start_div_cnt);

	// Sets the counter low and high values for the specified channel.
	vLow = DT0 - PULSE_WIDTH; // low tick number 
	vHigh = PULSE_WIDTH; // high tick number 
	FDwfDigitalOutCounterSet(hdwf, idxCh0, vLow, vHigh);

	//Sets the idle output of the specified channel.
	FDwfDigitalOutIdleSet(hdwf, idxCh0, DwfDigitalOutIdleLow);

	//Sets the output type of the specified channel
	//DwfDigitalOutTypePulse: Frequency = internal frequency/divider/(low + high counter).
	//DwfDigitalOutTypeCustom : Sample rate = internal frequency / divider
	FDwfDigitalOutTypeSet(hdwf, idxCh0, DwfDigitalOutTypePulse);

	//  Specifies output mode of the channel.
	//FDwfDigitalOutOutputSet(hdwf, idxCh0, DwfDigitalOutOutputPushPull);

	// Enables or disables the channel specified by idxChannel.
	FDwfDigitalOutEnableSet(hdwf, idxCh0, true_mex);      // pulse on IO pin 0
	// -------- DigitalOut channel 0 sets ----------------------------------------------

	// -------- DigitalOut channel 1 sets ---------------------------------------------
	// Sets the initial divider value of the specified channel.
	FDwfDigitalOutDividerInitSet(hdwf, idxCh1, clk_out_div);
	// Sets the divider value of the specified channel.
	FDwfDigitalOutDividerSet(hdwf, idxCh1, clk_out_div);

	// Sets the initial state and counter value of the specified channel.
	fHigh = 0; // Start high
	start_div_cnt = 0; //Divider initial value.
	FDwfDigitalOutCounterInitSet(hdwf, idxCh1, fHigh, start_div_cnt);

	// Sets the counter low and high values for the specified channel.
	vLow = DT0 - PULSE_WIDTH; // low tick number 
	vHigh = PULSE_WIDTH; // high tick number 
	FDwfDigitalOutCounterSet(hdwf, idxCh1, vLow, vHigh);

	//Sets the idle output of the specified channel.
	FDwfDigitalOutIdleSet(hdwf, idxCh1, DwfDigitalOutIdleLow);

	//Sets the output type of the specified channel
	//DwfDigitalOutTypePulse: Frequency = internal frequency/divider/(low + high counter).
	//DwfDigitalOutTypeCustom : Sample rate = internal frequency / divider
	FDwfDigitalOutTypeSet(hdwf, idxCh1, DwfDigitalOutTypePulse);

	//  Specifies output mode of the channel.
	//FDwfDigitalOutOutputSet(hdwf, idxCh1, DwfDigitalOutOutputPushPull);

	// Enables or disables the channel specified by idxChannel.
	FDwfDigitalOutEnableSet(hdwf, idxCh1, true_mex);      // pulse on IO pin 1
	// -------- DigitalOut channel 1 sets ---------------------------------------------

	// -------- DigitalOut channel trigerr set  ---------------------------------------
#ifdef	DIGITAL_IN
	FDwfDigitalOutTriggerSourceSet(hdwf, trigsrcDetectorDigitalIn);
#else
	FDwfDigitalOutTriggerSourceSet(hdwf, trigsrcPC); // Sets the trigger source for the instrument
#endif // DIGITAL_IN

	FDwfDigitalOutConfigure(hdwf, 1); // Starts or stops the instrument.
	// -------- DigitalOut channel trigerr set  ---------------------------------------

#endif // DIGITAL_OUT
	// ---- DigitalOut: clk configure; channel 0 sets ; channel 1 sets -----------------

	// ---- ANALOG_OUT: Определяем временную зависимость излучающих каналов {xt0[i], xt1[i]} ----
#ifdef	ANALOG_OUT

	switch (fm_type)
	{
	case 0: // mono pulse;
	{
		// model monopulse signal -  generate custom samples normalized to +-1
		for (unsigned int i = 0; i < nSamplesOut; i++)
		{
			xt0[i] = sin(2. * pi * fi_Hz * TsOut_s * i);
			xt1[i] = sin(2. * pi * fi_Hz * TsOut_s * i);
			//printf("i = %u, phase = %8.4e   xt0[i] = %8.4e\n", i, fi_Hz * TsOut_s * i, xt0[i]);
		}
	}
	break;

	case 1: // sweep pulse
	{
		double dF_Hz = f1_Hz - f0_Hz;     //[Hz] Sweep spectrum width параметры ЛЧМ

		// model sweep signal - generate custom samples normalized to +-1
		for (unsigned int i = 0; i < nSamplesOut; i++)
		{
			double ti = TsOut_s * i;
			xt0[i] = sin(2. * pi * (f0_Hz + ti * dF_Hz / (2 * Ts_sec)) * ti);
			xt1[i] = sin(2. * pi * (f0_Hz + ti * dF_Hz / (2 * Ts_sec)) * ti);
			//printf("i = %u, phase = %8.4e   xt0[i] = %8.4e\n", i, fi_Hz * ti, xt0[i]);
		}
	}
	break;

	case 2: // read data file
	{
		FILE* InPutStream;
		errno_t err = fopen_s(&InPutStream, "InPut.dat", "r"); //Last line in file InPut.dat must be correct - NOT EMPTY LINE
		if (err == 0)
		{
			for (unsigned int i = 0; i < nSamplesOut; i++) {

				errno_t count = fscanf_s(InPutStream, "%lf%lf\n", &xt0[i], &xt1[i]);

				if (count == 0) { // read data error check
					fprintf(stderr, "ERROR: The 'InPut.dat' file has not been read. fscanf - matching failure\n");
					return -1;
				}
			}
			fclose(InPutStream);

			printf("The  InPut.dat file was opened & read\n");
		}
		else
		{ // file open error check
			fprintf(stderr, "ERROR: The 'InPut.dat' file file was not opened. fopen_s - failure\n");
			return -1;
		}
	}
	break;

	default:

		fprintf(stderr, "ERROR: The 'fm_type' parameter cannot be determined.\n");
		return -1;
		break;
	}

#endif //	ANALOG_OUT
	// ---- ANALOG_OUT: Определяем временную зависимость излучающих каналов {xt0[i], xt1[i]} ----

	// ----- AnalogOut channels configure ----------------------------------------------
#ifdef	ANALOG_OUT

	// FDwfAnalogOutReset(HDWF hdwf, int idxChannel)
	FDwfAnalogOutReset(hdwf, -1); // To reset instrument parameters across all channels, set idxChannel to - 1.

	// enable channel
	FDwfAnalogOutNodeEnableSet(hdwf, 0, AnalogOutNodeCarrier, true_mex);
	FDwfAnalogOutNodeEnableSet(hdwf, 1, AnalogOutNodeCarrier, true_mex);

	// MaxVolts = 2V amplitude, 4V pk2pk, for sample value -1 will output -2V, for 1 +2V
	FDwfAnalogOutNodeAmplitudeSet(hdwf, 0, AnalogOutNodeCarrier, MaxVolts0);
	FDwfAnalogOutNodeAmplitudeSet(hdwf, 1, AnalogOutNodeCarrier, MaxVolts1);

	// by default the offset is 0V
	// FDwfAnalogOutNodeOffsetSet(HDWF hdwf, int idxChannel, AnalogOutNode node, double vOffset (in Volts))
	FDwfAnalogOutNodeOffsetSet(hdwf, 0, AnalogOutNodeCarrier, 0);
	FDwfAnalogOutNodeOffsetSet(hdwf, 1, AnalogOutNodeCarrier, 0);

	//	DwfAnalogOutIdleOffset	-	The idle output is the configured Offset level.
	//	DwfAnalogOutIdleInitial	-	The idle output voltage level has the initial waveform value of the current configuration.
	// FDwfAnalogOutIdleSet(hdwf, 0, DwfAnalogOutIdleInitial);
	FDwfAnalogOutIdleSet(hdwf, 0, DwfAnalogOutIdleOffset);
	FDwfAnalogOutIdleSet(hdwf, 1, DwfAnalogOutIdleOffset);

	// Set running time limit (in Seconds)
	FDwfAnalogOutRunSet(hdwf, 0, Ts_sec);
	FDwfAnalogOutRunSet(hdwf, 1, Ts_sec);

	// Set wait time before start (double in seconds)
	double secWait = 0.0;
	FDwfAnalogOutWaitSet(hdwf, 0, secWait);
	FDwfAnalogOutWaitSet(hdwf, 1, secWait);

	// set the trigger source for the channel on instrument.
	//FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcPC);
	//FDwfAnalogOutTriggerSourceSet(hdwf, 1, trigsrcPC);
	FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcExternal1);
	FDwfAnalogOutTriggerSourceSet(hdwf, 1, trigsrcExternal1);

	fRepeatTrigger = 1;
	// Sets the repeat trigger option. To include the trigger in wait-run repeat cycles, set fRepeatTrigger to TRUE. It is disabled by default.
	FDwfAnalogOutRepeatTriggerSet(hdwf, 0, fRepeatTrigger);
	FDwfAnalogOutRepeatTriggerSet(hdwf, 1, fRepeatTrigger);

	// Description: Sets the repeat count.
	FDwfAnalogOutRepeatSet(hdwf, 0, cRepeat);
	FDwfAnalogOutRepeatSet(hdwf, 1, cRepeat);

#endif //	ANALOG_OUT
	// ----- AnalogOut channels configure ----------------------------------------------

	// --- AnalogOut Arm ---------------------------------------------------------------
#ifdef	ANALOG_OUT

	// Arm Set Up to signal acquisition (AnalogOut)
	// set custom waveform samples normalized to ±1 values
		// The function above is used to set the custom data or to prefill the buffer with play samples.
		// The samples are double precision floating point values(rgdData) normalized to ±1.
		// With the custom function option, the data samples(cdData) will be interpolated to the device buffer size.
		// The output value will be Offset + Sample*Amplitude, for instance:
		// - 0 value sample will output : Offset.
		// - +1 value sample will output : Offset + Amplitude.
		// - -1 value sample will output : Offset – Amplitude.
	FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, xt0, nSamplesOut);
	FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, xt1, nSamplesOut);

	// set custom function
	FDwfAnalogOutNodeFunctionSet(hdwf, 0, AnalogOutNodeCarrier, funcCustom);
	FDwfAnalogOutNodeFunctionSet(hdwf, 1, AnalogOutNodeCarrier, funcCustom);

	// waveform frequency == 1.0/Ts_sec
	// The FDwfAnalogOutNodeFrequency for standard and custom waveform = 1 / the entire waveform period
	// Note the FDwfAnalogOutNodeFrequencySet refers to the waveform period and not sample rate
	FDwfAnalogOutNodeFrequencySet(hdwf, 0, AnalogOutNodeCarrier, (double)(1.0 / Ts_sec));
	FDwfAnalogOutNodeFrequencySet(hdwf, 1, AnalogOutNodeCarrier, (double)(1.0 / Ts_sec));

	//printf("--Arm set Up to signal generation & acquisition-- \n");

	// Arm set Up to signal generation (AnalogOut)
	FDwfAnalogOutConfigure(hdwf, 0, true_mex);
	do
	{ // Must be after  FDwfAnalogOutConfigure(hdwf, 0, true_mex);
		FDwfAnalogOutStatus(hdwf, 0, &sts);
		//printf("FDwfAnalogOutStatus 0 sts = %d\n", sts); // -> sts == 1 Armed
	} while (sts != stsArm);
	//printf("AnalogOutPutLine 0 is Armed\n");

	FDwfAnalogOutConfigure(hdwf, 1, true_mex);
	do
	{ // Must be after  FDwfAnalogOutConfigure(hdwf, 1, true_mex);
		FDwfAnalogOutStatus(hdwf, 1, &sts);
		//printf("FDwfAnalogOutStatus 1 sts = %d\n", sts); // -> sts == 1 Armed
	} while (sts != stsArm);
	//printf("AnalogOutPutLine 1 is Armed\n");

#endif //	ANALOG_OUT
	// --- AnalogOut Arm ---------------------------------------------------------------

	// ----- AnalogIn channels configure ----------------------------------------------
#ifdef ANALOG_IN 
	// The function resets and configures(by default, having auto configure enabled)
	// all AnalogIn instrument parameters to default values.
	FDwfAnalogInReset(hdwf);

	// set the  buffer size
	FDwfAnalogInBufferSizeSet(hdwf, nSamplesIn);

	// enable channels
	FDwfAnalogInChannelEnableSet(hdwf, 0, true_mex);
	FDwfAnalogInChannelEnableSet(hdwf, 1, true_mex);

	// The function is used to set the acquisition mode.
	// acqmodeSingle Perform a single buffer acquisition. This is the default setting
	FDwfAnalogInAcquisitionModeSet(hdwf, acqmodeSingle);

	// Set pk2pk input range for 0 channels
	// set 2.5V pk2pk input range, -2.5V to 2.5V: FDwfAnalogInChannelRangeSet(hdwf, 0, 2.5);
	FDwfAnalogInChannelRangeSet(hdwf, 0, voltsRange0);
	FDwfAnalogInChannelRangeSet(hdwf, 1, voltsRange1);

	// The AnalogIn ADC always runs at maximum frequency, but the method in which the samples are stored in the buffer 
	// can be individually configured for each channel with FDwfAnalogInChannelFilterSet function
	// The function is used to set the sample frequency for the instrument.
	FDwfAnalogInFrequencySet(hdwf, fsIn_Hz);

	// Set the acquisition filter for each AnalogIn channel.
	// FILTER: -- filterDecimate : Looks for trigger in each ADC conversion, can detect glitches.
	// FILTER: -- filterAverage : Looks for trigger only in average of N samples, given by FDwfAnalogInFrequencySet.
	FDwfAnalogInChannelFilterSet(hdwf, 0, filterAverage);
	FDwfAnalogInChannelFilterSet(hdwf, 1, filterAverage);

	// configure trigger
	FDwfAnalogInTriggerSourceSet(hdwf, trigsrcExternal2);

	int trigcond;
	FDwfAnalogInTriggerConditionSet(hdwf, DwfTriggerSlopeRise);
	do
	{
		FDwfAnalogInTriggerConditionGet(hdwf, &trigcond);
		printf("AnalogInTriggerCondition = %d\n", trigcond);
	} while (trigcond != DwfTriggerSlopeRise);

	int trigtype;
	FDwfAnalogInTriggerTypeSet(hdwf, trigtypeEdge);
	do
	{
		FDwfAnalogInTriggerTypeGet(hdwf, &trigtype);
		printf("AnalogInTriggerType = %d\n", trigtype);
	} while (trigtype != trigtypeEdge);

	FDwfAnalogInTriggerLevelSet(hdwf, 1.0); // level set 1 [V]

	// The function is used to configure the horizontal trigger position in seconds.
	double secPosition = ((double)(nSamplesIn) / fsIn_Hz / 2);
	FDwfAnalogInTriggerPositionSet(hdwf, secPosition);

	// FDwfAnalogInTriggerChannelSet(HDWF hdwf, int idxChannel)
	// -idxChannel – Trigger channel index to set.
	// The function is used to set the trigger channel.
	FDwfAnalogInTriggerChannelSet(hdwf, 0);
	FDwfAnalogInTriggerChannelSet(hdwf, 1);

	// disable auto trigger
	double secTimeout = 0;
	FDwfAnalogInTriggerAutoTimeoutSet(hdwf, secTimeout);

	//trigger on digital signal
	//FDwfAnalogInTriggerSourceSet(hdwf, trigsrcDetectorDigitalIn);
	//FDwfDigitalInTriggerSet(hdwf, 0, 0, 3, 0);// # DIO - 2 rising edge
	//FDwfDigitalInConfigure(hdwf, 1, 1);

	//	FDwfAnalogInTriggerConditionInfo(HDWF hdwf, int* pfstrigcond)
	//	Description : Returns the supported trigger type options for the instrument.They are returned(by reference) as a
	//	bit field.This bit field can be parsed using the IsBitSet Macro.Individual bits are defined using the
	//	DwfTriggerSlope constants in dwf.h.These trigger condition options are :
	// 
	//• DwfTriggerSlopeRise(This is the default setting) :
	//	o For edgeand transition trigger on rising edge.
	//	o For pulse trigger on positive pulse; For window exiting.
	// 
	//	• DwfTriggerSlopeFall :
	//  o For edgeand transition trigger on falling edge.
	//	o For pulse trigger on negative pulse; For window entering.
	//
	//	• DwfTriggerSlopeEither :
	//  o For edgeand transition trigger on either edge.
	//	o For pulse trigger on either positive or negative pulse.
	//int pfstrigcond;
	//FDwfAnalogInTriggerConditionInfo(hdwf, &pfstrigcond);
	//printf("AnalogInTriggerConditionInfo = %d\n", pfstrigcond);
	//pfstrigcond = pfstrigcond;

	// FDwfAnalogInTriggerTypeInfo(HDWF hdwf, int *pfstrigtype)
	// Description: Returns the supported trigger type options for the instrument.They are returned(by reference) as a
	// bit field.This bit field can be parsed using the IsBitSet Macro.Individual bits are defined using the
	// TRIGTYPE constants in dwf.h.These trigger type options are :
	// trigtypeEdge : trigger on rising or falling edge.This is the default setting.
	// trigtypePulse : trigger on positive or negative; less, timeout, or more pulse lengths.
	// trigtypeTransition : trigger on rising or falling; less, timeout, or more transition times.
	// trigtypeWindow : trigger on exiting or entering level + / -hysteresis window.
	//FDwfAnalogInTriggerTypeInfo(hdwf, &pfstrigcond);
	//printf("AnalogInTriggerTypeInfo = %d\n", pfstrigcond);

#endif // ANALOG_IN 
	// ----- AnalogIn channels configure ----------------------------------------------

	// --- AnalogIn Arm ----------------------------------------------------------------
#ifdef ANALOG_IN 
	//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
	//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
	//stsError = 8;		stsBusy = 9;		stsStop = 10;

	// FDwfAnalogInConfigure(HDWF hdwf, int fReconfigure, int fStart)
	// Configures the instrument and start or stop the acquisition. 
	// To reset the Auto trigger timeout, set fReconfigure to TRUE.
	//	- fReconfigure – Configure the device.
	//	- fStart – Start the acquisition.
	FDwfAnalogInConfigure(hdwf, true_mex, true_mex);
	do
	{
		FDwfAnalogInStatus(hdwf, 0, &sts);
		//printf("STEP 0: AnalogInStatus - Wait for ARM  sts = %d\n", sts);
	} while (sts != stsArm);
	//printf("STEP 0: AnalogInStatus -ARM- sts = %d\n", sts);
#endif // ANALOG_IN 
	// --- AnalogIn Arm ----------------------------------------------------------------

	// ---- Reset DigitalOut Trigger ----------------------------------------------------
#ifdef DIGITAL_OUT
	// Starts or stops the instrument.
	FDwfDigitalOutConfigure(hdwf, 1);
#endif // DIGITAL_OUT
	// ---- Reset DigitalOut Trigger ----------------------------------------------------

	// ---- SUPER LOOP ------------------------------------------------------------------
#ifdef	DIGITAL_IN
	int series = 0;
	while (1)
	{

		FDwfDigitalOutConfigure(hdwf, 1); // Starts or stops the instrument.
		
		// Description: Sets the repeat count.
		FDwfAnalogOutRepeatSet(hdwf, 0, cRepeat);
		FDwfAnalogOutRepeatSet(hdwf, 1, cRepeat);

		// Arm set Up to signal generation (AnalogOut)
		FDwfAnalogOutConfigure(hdwf, 0, true_mex);
		do
		{ // Must be after  FDwfAnalogOutConfigure(hdwf, 0, true_mex);
			FDwfAnalogOutStatus(hdwf, 0, &sts);
			//printf("FDwfAnalogOutStatus 0 sts = %d\n", sts); // -> sts == 1 Armed
		} while (sts != stsArm);
		//printf("AnalogOutPutLine 0 is Armed\n");

		FDwfAnalogOutConfigure(hdwf, 1, true_mex);
		do
		{ // Must be after  FDwfAnalogOutConfigure(hdwf, 1, true_mex);
			FDwfAnalogOutStatus(hdwf, 1, &sts);
			//printf("FDwfAnalogOutStatus 1 sts = %d\n", sts); // -> sts == 1 Armed
		} while (sts != stsArm);
		//printf("AnalogOutPutLine 1 is Armed\n");

		FDwfAnalogInConfigure(hdwf, true_mex, true_mex);
		do
		{
			FDwfAnalogInStatus(hdwf, 0, &sts);
			//printf("STEP 0: AnalogInStatus - Wait for ARM  sts = %d\n", sts);
		} while (sts != stsArm);
		//printf("AnalogInStatus is Armed sts = %d\n", sts);


		series = series + 1;
		printf("Starting series %u...\n", series);

		// --- Waite for DigitalInTrigger --------------------------------------------------
		FDwfDigitalInConfigure(hdwf, 0, 1);
		do
		{
			//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
			//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
			//stsError = 8;		stsBusy = 9;		stsStop = 10;
			//	- fReadData – TRUE if data should be read.
			FDwfDigitalInStatus(hdwf, 0, &sts);
		} while (sts != stsDone);
		//printf("DigitalInStatus - 1 sts = %d\n", sts);
		// --- Waite for DigitalInTrigger --------------------------------------------------

		// ------ Output files  name configure ---------------------------------------------
#ifdef FILE_OUT

	// --- Output file name == Data & Time ISO -----------------------
		time_t mytime = time(NULL);
		struct tm* now = localtime(&mytime);

		char file_name[bufferLength];
		strftime(file_name, sizeof(file_name), "%Y-%m-%dT%H%M%S", now);
		sprintf((char*)&file_name[17], ".dat");
		printf("Time: %s\n", file_name);
		// --- Output file name == Data & Time ISO -----------------------

		FILE* stream0;
		errno_t err;
		char buffer[bufferLength];
		char str[strLength];

		err = fopen_s(&stream0, file_name, "wb");
		if (err != 0)
		{ // file open error check
			fprintf(stderr, "ERROR: The output file %s was not create & opened. fopen_s - failure\n", file_name);
			return -1;
		}
		else
		{
			// ---- буферизация потока ввода\вывода -------------------------------
			if (setvbuf(stream0, buff, _IOFBF, sizeof(buff)) != 0)
				printf("Incorrect type or size of buffer for stream0\n\r");
			// ---- буферизация потока ввода\вывода -------------------------------

			// --- пишем дату время измерений --------------------------
			memset(&buffer[strLength - 1], 0x0A, 1);

			memset(buffer, 0x20, strLength - 1);
			sprintf_s(str, strLength, "Date: %02d.%02d.%4d", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			sprintf_s(str, strLength, "Time: %02d:%02d:%02d\n", now->tm_hour, now->tm_min, now->tm_sec);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);
			// --- пишем дату время измерений --------------------------

			// --- проверяем есть ли файл с описанием измерений, если есть то копируем --------------
			FILE* stream1;

			err = fopen_s(&stream1, ".//DESCR.TXT", "r");
			if (err != 0)
			{ // file open error check
				fprintf(stderr, "WARNING: The 'DESCR.TXT' file file was not exist.\n");
			}
			else
			{
				memset(buffer, 0x20, bufferLength - 1);
				memset(&buffer[bufferLength - 1], 0x0A, 1);

				fread(buffer, sizeof(char), bufferLength - 1, stream1);
				fclose(stream1);

				fwrite(buffer, sizeof(char), bufferLength, stream0);
			}
			// --- проверяем есть ли файл с описанием измерений, если есть то копируем --------------


			// --- копируем данные конфигурационного файла ------------------------------------
			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d TimeScale", TimeScale);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d DT0", DT0);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d DT1", DT1);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d TotalPing", TotalPing);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d TimeDelay", TimeDelay);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f MaxVolts0", MaxVolts0);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f MaxVolts1", MaxVolts1);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f voltsRange0", voltsRange0);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f voltsRange1", voltsRange1);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d fm_type", fm_type);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f fi_Hz", fi_Hz);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f f0_Hz", f0_Hz);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f f1_Hz", f1_Hz);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d nSamplesOut", nSamplesOut);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%d nSamplesIn", nSamplesIn);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f fsIn_Hz", fsIn_Hz);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);

			memset(buffer, 0x20, strLength - 1);
			memset(&buffer[strLength - 1], 0x0A, 1);
			sprintf_s(str, strLength, "%f fsOut_Hz", fsOut_Hz);
			memcpy(buffer, str, strlen(str));
			ret_code = fwrite(buffer, sizeof(char), strLength, stream0);
			// --- копируем данные конфигурационного файла ------------------------------------


			// --- пишем в бинарном виде параметры измерений -----------
			ret_code = fwrite(&TotalPing, sizeof(TotalPing), 1, stream0);
			ret_code = fwrite(&nSamplesOut, sizeof(nSamplesOut), 1, stream0);
			ret_code = fwrite(&nSamplesIn, sizeof(nSamplesIn), 1, stream0);
			ret_code = fwrite(&MaxVolts0, sizeof(MaxVolts0), 1, stream0);
			ret_code = fwrite(&MaxVolts1, sizeof(MaxVolts1), 1, stream0);
			ret_code = fwrite(&fsIn_Hz, sizeof(fsIn_Hz), 1, stream0);
			ret_code = fwrite(&fsOut_Hz, sizeof(fsOut_Hz), 1, stream0);
			ret_code = fwrite(&TimeScale, sizeof(TimeScale), 1, stream0);
			ret_code = fwrite(&DT1, sizeof(DT1), 1, stream0);

			ret_code = fwrite(tOut, sizeof(double), nSamplesOut, stream0);
			fflush(stream0);

			ret_code = fwrite(tIn, sizeof(double), nSamplesIn, stream0);
			fflush(stream0);

			ret_code = fwrite(xt0, sizeof(double), nSamplesOut, stream0);
			fflush(stream0);

			ret_code = fwrite(xt1, sizeof(double), nSamplesOut, stream0);
			fflush(stream0);

			//fclose(stream0);
			// --- пишем в бинарном виде параметры измерений -----------
		}
#endif // FILE_OUT
		// ------ Output files  name configure ---------------------------------------------

		start = clock();
		ping = 0;

		while (ping < TotalPing)
		{
			FDwfAnalogInStatus(hdwf, false_mex, &sts);
			//printf("AnalogInStatus sts = %d, ping = %d\n", sts, ping);

	// --- Wait AnalogIn done ----------------------------------------------------------
#ifdef ANALOG_IN
		//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
		//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
		//stsError = 8;		stsBusy = 9;		stsStop = 10;
			do
			{
				// FDwfAnalogInStatus(HDWF hdwf, int fReadData, DwfState* psts)
				// Checks the state of the acquisition.To read the data from the device, set fReadData to TRUE.
				// For	single acquisition mode, the data will be read only when the acquisition is finished.
				//	- fReadData – TRUE if data should be read.
				//	- psts – Variable to receive the acquisition state.
				FDwfAnalogInStatus(hdwf, true_mex, &sts);
				//printf("STEP 1: AnalogInStatus - Wait for DONE  sts = %d, ping = %d\n", sts, ping);
			} while (sts != stsDone);
			//printf("STEP 1: AnalogInStatus -DONE- sts = %d, ping = %d\n", sts, ping);
#endif // ANALOG_IN
	// --- Wait AnalogIn done ----------------------------------------------------------

	// ---- Get the AnalogIn data ------------------------------------------------------
#ifdef ANALOG_IN
		// get the samples for each channel
			FDwfAnalogInStatusData(hdwf, 0, xr0, nSamplesIn);
			FDwfAnalogInStatusData(hdwf, 1, xr1, nSamplesIn);
#endif // ANALOG_IN
			// ---- Get the AnalogIn data ------------------------------------------------------

			// --- AnalogIn Arm ----------------------------------------------------------------
#ifdef ANALOG_IN
		//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
		//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
		//stsError = 8;		stsBusy = 9;		stsStop = 10;

		// FDwfAnalogInConfigure(HDWF hdwf, int fReconfigure, int fStart)
		// Configures the instrument and start or stop the acquisition.
		// To reset the Auto trigger timeout, set fReconfigure to TRUE.
		//	- fReconfigure – Configure the device.
		//	- fStart – Start the acquisition.
	//FDwfAnalogInConfigure(hdwf, true_mex, true_mex);
			FDwfAnalogInConfigure(hdwf, false_mex, true_mex);
			do
			{
				FDwfAnalogInStatus(hdwf, 0, &sts);
				//printf("STEP 2: AnalogInStatus - Wait for ARM  sts = %d, ping = %d\n", sts, ping);
			} while (sts != stsArm);
			//printf("STEP 2: AnalogInStatus -RE ARM- sts = %d, ping = %d\n", sts, ping);
#endif // ANALOG_IN
	// --- AnalogIn Arm ----------------------------------------------------------------

	// --- Write to file the receive data ----------------------------------------------
#ifdef FILE_OUT
		// --- номер измерения -------------
			ret_code = fwrite(&ping, sizeof(unsigned int), 1, stream0);

			// --- данные канала CH0 -----------
			ret_code = fwrite(xr0, sizeof(double), nSamplesIn, stream0);
			fflush(stream0);

			if (ret_code != nSamplesIn) // error handling
			{
				printf("ERROR: ping = %u write xr0 \n", ping);
			}

			// --- данные канала CH1 -----------
			ret_code = fwrite(xr1, sizeof(double), nSamplesIn, stream0);
			fflush(stream0);

			if (ret_code != nSamplesIn) // error handling
			{
				printf("ERROR: ping = %u write xr1 \n", ping);
			}
#endif // FILE_OUT
			// --- Write to file the receive data ----------------------------------------------

			// ---- reset {xr0, xr1} ---------------------------------------------------------------
#ifdef ANALOG_IN
			for (unsigned int i = 0; i < nSamplesInMax; i++)
			{
				xr0[i] = 0.0;
				xr1[i] = 0.0;
			}
#endif // ANALOG_IN
			// ---- reset {xr0, xr1} ---------------------------------------------------------------

			finish = clock();
			duration = (double)(finish - start); // [msec] //duration = (double)(finish - start) / CLOCKS_PER_SEC; //[sec]
			printf("%2.3f [msec]\n", duration);

			ping++;
		}

		printf("Done series %u...\n", series);
	}


#else

	//  ----------- The function generates one pulse on the PC trigger line. ------------
	if (!FDwfDeviceTriggerPC(hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		printf("DeviceTriggerPC failed\n\t%s", szError);
		return 0;
	}
	//  ----------- The function generates one pulse on the PC trigger line. ------------

	start = clock();
	ping = 0;

	while (ping < TotalPing)
	{

		FDwfAnalogInStatus(hdwf, false_mex, &sts);
		//printf("AnalogInStatus sts = %d, ping = %d\n", sts, ping);

// --- Wait AnalogIn done ----------------------------------------------------------
#ifdef ANALOG_IN
		//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
		//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
		//stsError = 8;		stsBusy = 9;		stsStop = 10;
		do
		{
			// FDwfAnalogInStatus(HDWF hdwf, int fReadData, DwfState* psts)
			// Checks the state of the acquisition.To read the data from the device, set fReadData to TRUE.
			// For	single acquisition mode, the data will be read only when the acquisition is finished.
			//	- fReadData – TRUE if data should be read.
			//	- psts – Variable to receive the acquisition state.
			FDwfAnalogInStatus(hdwf, true_mex, &sts);
			//printf("STEP 1: AnalogInStatus - Wait for DONE  sts = %d, ping = %d\n", sts, ping);
		} while (sts != stsDone);
		//printf("STEP 1: AnalogInStatus -DONE- sts = %d, ping = %d\n", sts, ping);
#endif // ANALOG_IN
	// --- Wait AnalogIn done ----------------------------------------------------------

	// ---- Get the AnalogIn data ------------------------------------------------------
#ifdef ANALOG_IN
		// get the samples for each channel
		FDwfAnalogInStatusData(hdwf, 0, xr0, nSamplesIn);
		FDwfAnalogInStatusData(hdwf, 1, xr1, nSamplesIn);
#endif // ANALOG_IN
		// ---- Get the AnalogIn data ------------------------------------------------------

		// --- AnalogIn Arm ----------------------------------------------------------------
#ifdef ANALOG_IN
		//stsRdy = 0;		stsArm = 1;			stsDone = 2;		stsTrig = 3;
		//stsCfg = 4;		stsPrefill = 5;		stsNotDone = 6;		stsTrigDly = 7;
		//stsError = 8;		stsBusy = 9;		stsStop = 10;

		// FDwfAnalogInConfigure(HDWF hdwf, int fReconfigure, int fStart)
		// Configures the instrument and start or stop the acquisition.
		// To reset the Auto trigger timeout, set fReconfigure to TRUE.
		//	- fReconfigure – Configure the device.
		//	- fStart – Start the acquisition.
	//FDwfAnalogInConfigure(hdwf, true_mex, true_mex);
		FDwfAnalogInConfigure(hdwf, false_mex, true_mex);
		do
		{
			FDwfAnalogInStatus(hdwf, 0, &sts);
			//printf("STEP 2: AnalogInStatus - Wait for ARM  sts = %d, ping = %d\n", sts, ping);
		} while (sts != stsArm);
		//printf("STEP 2: AnalogInStatus -RE ARM- sts = %d, ping = %d\n", sts, ping);
#endif // ANALOG_IN
	// --- AnalogIn Arm ----------------------------------------------------------------

	// --- Write to file the receive data ----------------------------------------------
#ifdef FILE_OUT
		// --- номер измерения -------------
		ret_code = fwrite(&ping, sizeof(unsigned int), 1, stream0);

		// --- данные канала CH0 -----------
		ret_code = fwrite(xr0, sizeof(double), nSamplesIn, stream0);
		fflush(stream0);

		if (ret_code != nSamplesIn) // error handling
		{
			printf("ERROR: ping = %u write xr0 \n", ping);
		}

		// --- данные канала CH1 -----------
		ret_code = fwrite(xr1, sizeof(double), nSamplesIn, stream0);
		fflush(stream0);

		if (ret_code != nSamplesIn) // error handling
		{
			printf("ERROR: ping = %u write xr1 \n", ping);
		}
#endif // FILE_OUT
		// --- Write to file the receive data ----------------------------------------------

		// ---- reset {xr0, xr1} ---------------------------------------------------------------
#ifdef ANALOG_IN
		for (unsigned int i = 0; i < nSamplesInMax; i++)
		{
			xr0[i] = 0.0;
			xr1[i] = 0.0;
		}
#endif // ANALOG_IN
		// ---- reset {xr0, xr1} ---------------------------------------------------------------

		finish = clock();
		duration = (double)(finish - start); // [msec] //duration = (double)(finish - start) / CLOCKS_PER_SEC; //[sec]
		printf("%2.3f [msec]\n", duration);

		ping++;
	}

#endif //DIGITAL_IN

	// ---- SUPER LOOP ------------------------------------------------------------------

	// ---- stop digital signal generation ---------------------------------------------
#ifdef DIGITAL_OUT
	FDwfDigitalOutReset(hdwf);
#endif // DIGITAL_OUT
	// ---- stop digital signal generation ---------------------------------------------

	// --- power supply ----------------------------------------------------------------
#ifndef PWR_SUPPLY

	printf("Total supply power and load percentage:\n");

	for (int i = 0; i < 5; i++)
	{
		// wait 1 second between readings
		Wait(1);
		// fetch analogIO status from device
		FDwfAnalogIOStatus(hdwf);

		// supply monitor
		FDwfAnalogIOChannelNodeStatus(hdwf, 2, 0, &vUSB);
		FDwfAnalogIOChannelNodeStatus(hdwf, 2, 1, &aUSB);
		FDwfAnalogIOChannelNodeStatus(hdwf, 3, 0, &vAUX);
		FDwfAnalogIOChannelNodeStatus(hdwf, 3, 1, &aAUX);

		printf("USB: %.3lf V \t%.3lf A \tAUX: %.3lf V \t%.3lf A  \n", vUSB, aUSB, vAUX, aAUX);

		// in case of over-current condition the supplies are disabled
		FDwfAnalogIOEnableStatus(hdwf, &fOn);
		if (!fOn) {
			// re-enable supplies
			FDwfAnalogIOEnableSet(hdwf, false_mex);
			FDwfAnalogIOEnableSet(hdwf, true_mex);
		}
	}

#endif // PWR_SUPPLY
	// --- power supply ----------------------------------------------------------------

	// ---- CloseDigilent --------------------------------------------------------------
	if (!FDwfDeviceClose(hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		printf("DeviceCloseError: %s\n", szError);
		return -1;
	}
	else
	{
		printf("THE JOB IS DONE.\n");
		return 1;
	}
	// ---- CloseDigilent --------------------------------------------------------------

}

void operation_parameters_print(void)
{

	printf("TotalPing                                    [num]: TotalPing   = %u \n", TotalPing);
	printf("The time delay between measurments series     [ms]: TimeDelay   = %u \n", TimeDelay); // real time delay ~ 40 ms + TimeDelay
	printf("The user-defined max wave propagation time     [s]: DT0         = %8.3e \n", 1.0e-6 * TimeScale * DT0);
	printf("The time delay between radiation and reception [s]: DT1         = %8.3e \n\n", 1.0e-6 * TimeScale * DT1);

	printf("Channel 0: DAC max output voltage [V]: MaxVolts0    = %4.2f \n", MaxVolts0);
	printf("Channel 1: DAC max output voltage [V]: MaxVolts0    = %4.2f \n\n", MaxVolts1);
	printf("Channel 0: ADC input voltage range[V]: voltsRange0  = %4.2f \n", voltsRange0);
	printf("Channel 1: ADC input voltage range[V]: voltsRange1  = %4.2f \n\n", voltsRange1);

	switch (fm_type)
	{
	case 0: // mono pulse;
	{
		printf("Mono: Signal frequency            [Hz]: fi_Hz       = %16.9e \n", fi_Hz);
		printf("Output smpl point on period     [smpl]: Ns          = %6.2f  \n", (1.0 / fi_Hz) / TsOut_s);
		printf("Output signal emission time        [s]: Ts_sec      = %16.9e \n", Ts_sec);
		printf("Input smpl point on period      [smpl]: Ns          = %6.2f \n", (1.0 / fi_Hz) / TsIn_s);
		printf("Input total measurments time       [s]: TsIn_sec    = %16.9e \n", nSamplesIn * TsIn_s);
	}
	break;

	case 1: // sweep pulse
	{
		printf("Sweep: Min signal frequency       [Hz]: f0_Hz       = %16.9e \n", f0_Hz);
		printf("Sweep: Max signal frequency       [Hz]: f1_Hz       = %16.9e \n", f1_Hz);
		printf("Output min smpl point on period [smpl]: min Ns      = %6.2f  \n", (1.0 / f1_Hz) / TsOut_s);
		printf("Output signal emission time        [s]: Ts_sec      = %16.9e \n", Ts_sec);
		printf("Input min smpl point on period  [smpl]: min Ns      = %6.2f \n", (1.0 / f1_Hz) / TsIn_s);
		printf("Input total measurments time       [s]: TsIn_sec    = %16.9e \n", nSamplesIn * TsIn_s);
	}
	break;

	case 2: // read data file
	{
		printf("Output smpl frequency             [Hz]: fsOut_Hz    = %16.9e \n", fsOut_Hz);
		printf("Output signal emission time        [s]: Ts_sec      = %16.9e \n", Ts_sec);
		printf("Input smpl frequenc y             [Hz]: fsIn_Hz     = %16.9e \n", fsIn_Hz);
		printf("Input measurments time             [s]: TsIn_sec    = %16.9e \n", nSamplesIn * TsIn_s);
	}
	break;

	default:

		fprintf(stderr, "ERROR: The 'fm_type' parameter cannot be determined.\n");
		//return -1;
		break;
	}

}

int command_line_parsing(int argc, char* argv[])
{
	if (argc <= 1)
	{
		printf("Misunderstood input, please enter:\n");
		printf("WaveRecord.exe --help    OR   WaveRecord.exe --file config.dat    OR    WaveRecord.exe --example\n\t");
		return -1;
	}
	if (!strcmp(argv[1], "--help"))
	{
		help_print();
		printf("print help\n\t");
		return -1;
	}
	else if (!strcmp(argv[1], "--example"))
	{
		example_print();
		printf("write example config files\n\t");
		return -1;
	}
	else if (!strcmp(argv[1], "--file"))
	{
		if (argc >= 3) {
#ifdef  CFG_FILE
			config_parsing(argv[2]);
#endif	
		}
		else
		{
			printf("ERROR: No config file name.\n\t");
		}

		return 0;
	}
	else {
		printf("Unknown parameter: '%s'. Type %s --help for help.\n", argv[1], argv[0]);
		return -1;
	}

}

void help_print(void)
{
	printf("You requested help message. => print file READ_ME.TXT\n");

	FILE* stream;
	stream = fopen("READ_ME.TXT", "w"); //Last line in file InPut.dat must be correct - NOT EMPTY LINE
	if (stream != NULL)
	{
		fprintf(stream, "%s\n", "НАЗНАЧЕНИЕ:                                                                                                                                                                        ");
		fprintf(stream, "%s\n", "Программа WaveRecord3 предназначена для работы с AnalogDiscovery 2 с целью генерации/записи высокочастотных сигналов.                                                              ");
		fprintf(stream, "%s\n", "Длительность сигналов ограничена возможностью буфера AnalogDiscovery 2,                                                                                                            ");
		fprintf(stream, "%s\n", "8192 – отсчета на прием, 4096 – отсчёта на излучение, частота дискретизации до 100 МГц.                                                                                            ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "ВОЗМОЖНОСТИ:                                                                                                                                                                       ");
		fprintf(stream, "%s\n", "- Программа позволяет задавать в качестве излучаемого сигнала: радиоимпульс (монохром) или ЛЧМ (sweep),                                                                            ");
		fprintf(stream, "%s\n", "  или сигнал произвольной формы, заданный в файле (InPut.dat).                                                                                                                     ");
		fprintf(stream, "%s\n", "- По результатам измерений формируется текстовый файл '%Y-%m-%dT%H%M%S.dat', имя файла содержит время измерений.                                                                   ");
		fprintf(stream, "%s\n", "  В файл записаны все данные измерений необходимые для восстановления результатов измерений, а также конфигурационный файл измерения,                                              ");
		fprintf(stream, "%s\n", "  файл описания результатов измерений. Содержание файла является смешанным: преамбула – текст, результаты измерений бинарные данные.                                               ");
		fprintf(stream, "%s\n", "- На данный момент: излучение/прием реализованы для двух каналов, формы излучаемого импульса идентичны для обоих каналов в случае радиоимпульса и ЛЧМ,                             ");
		fprintf(stream, "%s\n", "  в случае задания произвольного сигнала в виде файла зависимость формы импульса для каждого канала может быть задана индивидуально.                                               ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "ЗАДАЮТСЯ:                                                                                                                                                                          ");
		fprintf(stream, "%s\n", "- форма импульса (fm_type = 0 монохром, fm_type = 1 ЛЧМ, fm_type = 2 произвольная);                                                                                                ");
		fprintf(stream, "%s\n", "- временные параметры импульса (монохром - fi_Hz [Гц] несущая частота, ЛЧМ - {f0_Hz, f1_Hz} [Гц] начальная и конечная частота спектра);                                            ");
		fprintf(stream, "%s\n", "- количество отсчетов на излучение (nSamplesOut <= 4096), количество отсчетов на прием (nSamplesIn <= 8192);                                                                       ");
		fprintf(stream, "%s\n", "- шаг дискретизации по времени для излучения (fsOut_Hz [Гц]) и отдельно шаг дискретизации по времени для приема (fsIn_Hz [Гц])                                                     ");
		fprintf(stream, "%s\n", "  (ADC и DAC работают на временной сетке 100 МГц, соответственно шаг дискретизации должен быть кратен это частоте);                                                                ");
		fprintf(stream, "%s\n", "- амплитуды излучаемых сигналов для каждого канала ({MaxVolts0, MaxVolts1} <= 5[V]);                                                                                               ");
		fprintf(stream, "%s\n", "- максимальное время пробега волны (DT0 задается в абстрактных единицах [tick]: DT0 < 512 + 32768);                                                                                ");
		fprintf(stream, "%s\n", "- время задержки начало приема относительно момента излучения (DT1 задается в абстрактных единицах [tick]: DT1 < DT0 - 1);                                                         ");
		fprintf(stream, "%s\n", "- масштаб временной шкалы (TimeScale = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000}) определяющий величину одного tick = TimeScale * 1 [мксек];                                      ");
		fprintf(stream, "%s\n", "- праметры чувствительности ADC (voltsRange0, voltsRange1 [V]) отдельно для каждого канала,                                                                                        ");
		fprintf(stream, "%s\n", " если voltsRange = 5.0 [V] то чувствительность ADC = 0.32mV/отсчет, если voltsRange = 50.0 [V] то чувствительность ADC = 3.58mV/отсчет;                                            ");
		fprintf(stream, "%s\n", "- количество циклов излучения/приема в рамках одного измерения TotalPing (не нормируется);                                                                                         ");
		fprintf(stream, "%s\n", "- интервал между циклами излучения/приема в рамках одного измерения TimeDelay [мсек] (не нормируется),                                                                             ");
		fprintf(stream, "%s\n", "  реальное время между циклами излучения/приема необходимо оценивать при помощи осциллографа по интервалам времени                                                                 ");
		fprintf(stream, "%s\n", "  между импульсами на цифровом выводе D0, ориентировочно время между циклами излучения/приема ~ 40 [мсек] + TimeDelay [мсек].                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "СХЕМА ПОДКЛЮЧЕНИЯ:                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "Аналоговые входы/выходы (приема/излучения) могут быть подключены непосредственно или через плату расширения (BNC разъемы);                                                         ");
		fprintf(stream, "%s\n", "Земля подключается через плату расширения или непосредственно;                                                                                                                     ");
		fprintf(stream, "%s\n", "Необходимо подключить:                                                                                                                                                             ");
		fprintf(stream, "%s\n", "цифровой вывод D0 <-> вход триггера Trig1;                                                                                                                                         ");
		fprintf(stream, "%s\n", "цифровой вывод D1 <-> вход триггера Trig2;                                                                                                                                         ");
		fprintf(stream, "%s\n", "цифровой вывод D2 используется для внешнего запуска серии измерения;                                                                                                               ");
		fprintf(stream, "%s\n", "Параметры запускающего импульса следующие - одиночный импульс, напряжение до 3.3 [В], старт измерений по заднему спадающему фронту через 1.5 мсек.                                ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "АЛГОРИТМ РАБОТЫ:                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "После прихода внешнего тактового импулься на цифровой пин D2 (3.3 [В]), по спадающему фронту тактового импульса происходит серия измерений.                                        ");
		fprintf(stream, "%s\n", "Всего проводится TotalPing измерений в одной серии измерений. Количество серий измерений не ограничено.                                                                            ");
		fprintf(stream, "%s\n", "Интервал между сериями измерений не менее 1 сек, что успеть записать файл с другим именем.                                                                                         ");
		fprintf(stream, "%s\n", "Через интервалы времени TimeScale*DT0 [мксек], на цифровой вывод D0 подается тактовый импульс, по которому производиться излучение.                                                ");
		fprintf(stream, "%s\n", "После тактового импульса на D0, через интервал времени TimeScale*DT1 [мксек] подается тактовый импульс на цифровой вывод D1, по которому производится регистрация данных.          ");
		fprintf(stream, "%s\n", "Цифровой вывод D0 подключен ко входу триггера Trig1, по сигналу на триггере Trig1 происходит излучение.                                                                            ");
		fprintf(stream, "%s\n", "Цифровой вывод D1 подключен ко входу триггера Trig2, по сигналу на триггере Trig2 происходит регистрация данных.                                                                   ");
		fprintf(stream, "%s\n", "Таким образом временной интервал TimeScale * DT1 [мксек], это время задержки между излучением и приемом.                                                                           ");
		fprintf(stream, "%s\n", "Временной интервал TimeScale * DT0 [мксек] определяет максимальную задержку между началом излучения и началом приема данных.                                                       ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "ОСОБЕННОСТИ РАБОТЫ:                                                                                                                                                                ");
		fprintf(stream, "%s\n", "По всей видимости АЦП AnalogDiscovery 2 работает на частоте 100 МГц, и существует случайная задержка                                                                               ");
		fprintf(stream, "%s\n", "порядка 220 [нсек] между восходящим фронтом (rasing edge)сигнала на входе триггера Trig2 и началом записи данных.                                                                  ");
		fprintf(stream, "%s\n", "Для удобства на пин питания V+ выведено напряжения +3.3[В]/0.1[A] что может быть использовано для формирования схемы гальванической развязки D2 от внешней аппаратуры. 2           ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "ОПЦИИ ПРОГРАММЫ:                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "WaveRecord3.exe --help	                => выводит данную справку, и сохраняет ее в виде файла READ-ME.TXT;                                                                         ");
		fprintf(stream, "%s\n", "WaveRecord3.exe --file config.dat      => запускает программу на исполнение c параметрами, заданными в конфигурационном файле 'config.dat';                                        ");
		fprintf(stream, "%s\n", "WaveRecord3.exe --example              => записывает пример файла конфигурации 'config.dat' и пример скрипта, matlab-а 'plotResult2.m',                                            ");
		fprintf(stream, "%s\n", "                                          который строит результаты измерений на основе файла '%Y-%m-%dT%H%M%S.dat',                                                              ");
		fprintf(stream, "%s\n", "                                          а также содержит пример формирования файла InPut.dat.                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "НЕОБХОДИМЫЕ ДЛЯ РАБОТЫ ФАЙЛЫ:                                                                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "Обязательные:                                                                                                                                                                      ");
		fprintf(stream, "%s\n", "‘WaveRecord3.exe’ – исполняемый файл, до запуска в работу,                                                                                                                         ");
		fprintf(stream, "%s\n", "                    желательно проверить подключение AnalogDiscovery 2 с помощью штатной программы WaveForms;                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "‘config.dat’ – файл задания конфигурационных параметров измерений;                                                                                                                 ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "‘plotResult3.m’ – файл с кодом matlab-а для считывания данных результатов измерений;                                                                                               ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "Дополнительные:                                                                                                                                                                    ");
		fprintf(stream, "%s\n", "‘DESCR.TXT’ – текстовый файл, в произвольной форме, с описанием проводимых измерений.                                                                                              ");
		fprintf(stream, "%s\n", "              Содержание данного файла включается в файл результатов измерений %Y-%m-%dT%H%M%S.dat.                                                                                ");
		fprintf(stream, "%s\n", "              Файл DESCR.TXT - НЕ ДОЛЖЕН СОДЕРЖАТЬ строку вида «===============»,                                                                                                  ");
		fprintf(stream, "%s\n", "              поскольку она используется в файле результатов для разделения текстовых данных от бинарных.                                                                          ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "ФОРМАТ ФАЙЛОВ:                                                                                                                                                                     ");
		fprintf(stream, "%s\n", "‘config.dat’ - задается до начала измерений                                                                                                                                        ");
		fprintf(stream, "%s\n", "// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
		fprintf(stream, "%s\n", "5            % TimeScale     // 1 => (1 tick == 1 us); 10 => (1 tick == 10 us); 100 => (1 tick == 0.1 ms); 1000 => (1 tick == 1 ms);                                               ");
		fprintf(stream, "%s\n", "1000         % DT0  [tick]   //Max value = 512 + 32768 [tick]; The DT0 Must be: (DT0 > WAVE PROPAGATION TIME)                                                                      ");
		fprintf(stream, "%s\n", "0            % DT1  [tick]   //Must be DT1: DT1 < DT0 - PULSE_WIDTH, where PULSE_WIDTH == 1; The time delay between radiation and reception                                        ");
		fprintf(stream, "%s\n", "25           % TotalPing     //The total number of measurement series                                                                                                              ");
		fprintf(stream, "%s\n", "10           % TimeDelay     //The measurments series param. // [ms] Time delay between measurments series, real time delay ~ 40 ms + TimeDelay                                    ");
		fprintf(stream, "%s\n", "3.75         % MaxVolts0     //[V] MaxVolts0 <= 5; DAC0 range                                                                                                                      ");
		fprintf(stream, "%s\n", "2.5          % MaxVolts1     //[V] MaxVolts1 <= 5; DAC1 range                                                                                                                      ");
		fprintf(stream, "%s\n", "50.0         % voltsRange0   //[V] voltsRange0 = {5.0 OR 50.0}; ADC0 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV; ");
		fprintf(stream, "%s\n", "50.0         % voltsRange1   //[V] voltsRange1 = {5.0 OR 50.0}; ADC1 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV; ");
		fprintf(stream, "%s\n", "0            % fm_type       //The emission signal type // fm_type = 0 -> mono pulse; fm_type = 1 -> sweep pulse; fm_type = 2 -> custom (read from file)                           ");
		fprintf(stream, "%s\n", "12500000.0   % fi_Hz         //[Hz] The carrier frequency of the monochromatic pulse                                                                                               ");
		fprintf(stream, "%s\n", "10000000.0   % f0_Hz         //[Hz] Start frequency band of sweep signal                                                                                                           ");
		fprintf(stream, "%s\n", "12500000.0   % f1_Hz         //[Hz] Stop frequency band of sweep signal                                                                                                            ");
		fprintf(stream, "%s\n", "4000         % nSamplesOut   //nSamplesOut <= 4096;                                                                                                                                ");
		fprintf(stream, "%s\n", "8192         % nSamplesIn    //nSamplesIn <= 8192;                                                                                                                                 ");
		fprintf(stream, "%s\n", "100000000.0  % fsIn_Hz       //[Hz] Input sampling frequency - частота дискретизации входного сигнала                                                                              ");
		fprintf(stream, "%s\n", "100000000.0  % fsOut_Hz      //[Hz] Output sampling frequency - частота дискретизации выходного сигнала                                                                            ");
		fprintf(stream, "%s\n", "// -------------------------------------------------------------------------------------------------------------------------------------------------------------------             ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "// --- Параметры, определяющие задержку момента начала записи сигналов относительно момента излучения --------------------                                                         ");
		fprintf(stream, "%s\n", "uint32 TimeScale - определяет временную шкалу для {DT0, DT1}, если TimeScale = 1, {DT0, DT1} задается в [мксек] (1 tick == 1 [мксек]),                                             ");
		fprintf(stream, "%s\n", "                   если TimeScale = 1000, {DT0, DT1} задается в [мсек] (1 tick == 1 [мсек]);                                                                                       ");
		fprintf(stream, "%s\n", "uint32 DT0 [tick] - интервал времени, который должен быть больше чем время задержки момента измерения относительно момента излучения, ограничения: DT0 < 512 + 32768;              ");
		fprintf(stream, "%s\n", "uint32 DT1 [tick] - интервал времени между моментом излучения и моментом начала записи сигналов, ограничения DT1 < DT0 - 1;                                                        ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "// --- Параметры, определяющие количество циклов излучения/приема и интервал времени между циклами измерений -------------                                                         ");
		fprintf(stream, "%s\n", "uint32 TotalPing - общее количество циклов излучения/приема, в рамках одной серии не нормируется;                                                                                  ");
		fprintf(stream, "%s\n", "uint32 TimeDelay - устаревший параметр, оставлен для совместимости                                                                                                                 ");
		fprintf(stream, "%s\n", "Интервал времени [мсек] между циклами измерений (излучения/приема), определяется произведением TimeScale*DT0 [мксек],                                                               ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "// --- Параметры, определяющие конфигурацию каналов ADC/DAC --------------------------------------------------------------                                                         ");
		fprintf(stream, "%s\n", "double MaxVolts0    [V] - амплитуда сигнала [V] DAC0, ограничения MaxVolts0 <= 5[V];                                                                                               ");
		fprintf(stream, "%s\n", "double MaxVolts1    [V] - амплитуда сигнала [V] DAC1, ограничения MaxVolts1 <= 5[V];                                                                                               ");
		fprintf(stream, "%s\n", "double voltsRange0  [V] - voltsRange0 = {5.0 или 50.0} динамический диапазон ADC0, заданный диапазон соответствует 14 битам шкалы ADC;                                             ");
		fprintf(stream, "%s\n", "double voltsRange1  [V] - voltsRange1 = {5.0 или 50.0} динамический диапазон ADC1, заданный диапазон соответствует 14 битам шкалы ADC;                                             ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "// --- Параметры, определяющие форму излучаемого сигнала DAC --------------------------------------------------------------                                                        ");
		fprintf(stream, "%s\n", "uint32 fm_type - определяет тип излучаемого сигнала {0 - монохром, 1 - ЛЧМ, 2 - задается файлом InPut.dat}                                                                         ");
		fprintf(stream, "%s\n", "double fi_Hz [Hz] - несущая частота радиоимпульса;                                                                                                                                 ");
		fprintf(stream, "%s\n", "double f0_Hz [Hz] - начальная частота ЛЧМ спектра;                                                                                                                                 ");
		fprintf(stream, "%s\n", "double f1_Hz [Hz] - конечная частота ЛЧМ спектра;                                                                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "// --- Параметры определяющие временную сетку ADC/DAC -------------------------------------------------------------------                                                          ");
		fprintf(stream, "%s\n", "uint32 nSamplesOut [smpl] - количество отсчетов в излучаемом сигнале, ограничения nSamplesOut <= 4096;                                                                             ");
		fprintf(stream, "%s\n", "uint32 nSamplesIn  [smpl] - количество отсчетов в принимаемом сигнале, ограничения nSamplesIn <= 8192;                                                                             ");
		fprintf(stream, "%s\n", "double fsIn_Hz     [Hz]   - частотная сетка ADC, ограничения fsIn_Hz должна быть кратна 100 МГц;                                                                                   ");
		fprintf(stream, "%s\n", "double fsOut_Hz    [Hz]   - частотная сетка DAC, ограничения fsOut_Hz должна быть кратна 100 МГц;                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "'InPut.dat' - задается до измерений, описывает форму сигналов, подаваемых на DAC0, DAC1                                                                                            ");
		fprintf(stream, "%s\n", "// ------------------------------------------------------------------------------                                                                                                  ");
		fprintf(stream, "%s\n", " 0.0000000e+00 0.0000000e+00                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 6.2831440e-03 1.2566040e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 1.2566040e-02 2.5130095e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 1.8848440e-02 3.7690183e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 2.5130095e-02 5.0244318e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 3.1410759e-02 6.2790520e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 3.7690183e-02 7.5326806e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 4.3968118e-02 8.7851197e-02                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 5.0244318e-02 1.0036171e-01                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 5.6518534e-02 1.1285638e-01                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 6.2790520e-02 1.2533323e-01                                                                                                                                                       ");
		fprintf(stream, "%s\n", " 6.9060026e-02 1.3779029e-01                                                                                                                                                       ");
		fprintf(stream, "%s\n", " .......................................................                                                                                                                           ");
		fprintf(stream, "%s\n", "// ------------------------------------------------------------------------------                                                                                                  ");
		fprintf(stream, "%s\n", "первый столбец - нормированная к единице форма сигнала, подаваемого на DAC0                                                                                                        ");
		fprintf(stream, "%s\n", "второй столбец - нормированная к единице форма сигнала, подаваемого на DAC1                                                                                                        ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "Амплитуда сигналов описывается параметрами {MaxVolts0, MaxVolts1} файла 'config.dat';                                                                                              ");
		fprintf(stream, "%s\n", "Временная сетка описывается параметром fsOut_Hz файла 'config.dat';                                                                                                                ");
		fprintf(stream, "%s\n", "Число отсчетов (строк файла) должно соответствовать параметру nSamplesOut файла 'config.dat', быть больше;                                                                         ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                   ");

		fclose(stream);
	}
	else
	{ // file open error check
		fprintf(stderr, "ERROR: The example of 'READ_ME_.TXT' file was not created.\n");
	}
}

void example_print(void)
{
	printf("You requested example message. => print files 'config.dat' & 'plotResult.m' \n");

	FILE* stream;
	stream = fopen("config.dat", "w"); //Last line in file InPut.dat must be correct - NOT EMPTY LINE
	if (stream != NULL)
	{
		fprintf(stream, "%s\n", "5                   %  TimeScale  // The time scale = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000}: 1 => (1 tick == 1 us);  10 => (1 tick == 10 us);  ...");
		fprintf(stream, "%s\n", "1000                %  DT0        // The time interval between measurements    // [tick] Max value = 512 + 32768 [tick];  The DT0 Must be: DT0*TimeScale > 5 msec");
		fprintf(stream, "%s\n", "10                  %  DT1        // The time interval between the emissions and the reception // [tick]  The DT1 Must be: DT1 < DT0 - PULSE_WIDTH, where PULSE_WIDTH  == 1;");
		fprintf(stream, "%s\n", "25                  %  TotalPing  // The total number of measurement in series");
		fprintf(stream, "%s\n", "30                  %  TimeDelay  // The obsolete parameters Must be  == 10");
		fprintf(stream, "%s\n", "3.75                %  MaxVolts0  // [V] MaxVolts0 <= 5; DAC0 range");
		fprintf(stream, "%s\n", "2.5                 %  MaxVolts1  // [V] MaxVolts1 <= 5; DAC1 range");
		fprintf(stream, "%s\n", "5.0                 %  voltsRange0// [V] voltsRange0 = {5.0 OR 50.0};  ADC0 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV;");
		fprintf(stream, "%s\n", "50.0                %  voltsRange1// [V] voltsRange1 = {5.0 OR 50.0};  ADC1 range; If ADC range = 5.0 [V] => ADC resolution = 0.32mV; If ADC range = 50.0 [V] => ADC resolution = 3.58mV;");
		fprintf(stream, "%s\n", "0                   %  fm_type    // The emission signal type  // fm_type = 0 -> mono pulse; fm_type = 1 -> sweep pulse; fm_type = 2 -> custom (read from file)");
		fprintf(stream, "%s\n", "12500000.0          %  fi_Hz      // The emission signal type  // [Hz] The carrier frequency of the monochromatic pulse");
		fprintf(stream, "%s\n", "10000000.0          %  f0_Hz      // The emission signal type  // [Hz] Start frequency band of sweep signal");
		fprintf(stream, "%s\n", "12500000.0          %  f1_Hz      // The emission signal type  // [Hz] Stop frequency band of sweep signal");
		fprintf(stream, "%s\n", "4000                %  nSamplesOut// The time param. of analog input/output channels    // nSamplesOut <= 4096;");
		fprintf(stream, "%s\n", "8192                %  nSamplesIn // The time param. of analog input/output channels    // nSamplesIn  <= 8192;");
		fprintf(stream, "%s\n", "100000000.0         %  fsIn_Hz    // The time param. of analog input/output channels    // [Hz] Input sampling frequency - частота дискретизации входного сигнала");
		fprintf(stream, "%s\n", "100000000.0         %  fsOut_Hz   // The time param. of analog input/output channels    // [Hz] Output sampling frequency - частота дискретизации выходного сигнала");
		fprintf(stream, "%s\n", " ");

		fclose(stream);
	}
	else
	{ // file open error check
		fprintf(stderr, "ERROR: The example of 'config.dat' file was not created.\n");
	}

	stream = fopen("plotResult3.m", "w"); //Last line in file InPut.dat must be correct - NOT EMPTY LINE
	if (stream != NULL)
	{
		fprintf(stream, "%s\n", "%% ==== 0.0 Go to work catalog ============================================                                                                                                      ");
		fprintf(stream, "%s\n", "clear all; clc; %#ok<CLALL>                                                                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "DataDir = 'C:\\CODE\\Piston_source\\Digilent\\WaveRecord_v2\\';                                                                                                                       ");
		fprintf(stream, "%s\n", "WorkDir = 'C:\\CODE\\Piston_source\\Digilent\\WaveRecord_v2\\bin\\';                                                                                                                   ");
		fprintf(stream, "%s\n", "cd(WorkDir)                                                                                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "disp('GO TO WORK CATALOG')                                                                                                                                                       ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "%% ==== 1.0 GET DAT FILE ===========================================                                                                                                             ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "%  Задаем имя файла с данными, который хотим посмотреть                                                                                                                          ");
		fprintf(stream, "%s\n", "cd(DataDir)                                                                                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "[fname, pathname] = uigetfile({'*.dat'}, 'file selector');                                                                                                                       ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "if isequal(fname, 0)                                                                                                                                                             ");
		fprintf(stream, "%s\n", "    disp('Нет файлов с расширением .dat/n')                                                                                                                                      ");
		fprintf(stream, "%s\n", "    return                                                                                                                                                                       ");
		fprintf(stream, "%s\n", "else                                                                                                                                                                             ");
		fprintf(stream, "%s\n", "    fileName = fullfile(pathname, fname);                                                                                                                                        ");
		fprintf(stream, "%s\n", "    disp(['Выбран файл ', fileName])                                                                                                                                             ");
		fprintf(stream, "%s\n", "end                                                                                                                                                                              ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "cd(WorkDir)                                                                                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% ==== 2.0 DATA FILE LOAD ==========================================                                                                                                             ");
		fprintf(stream, "%s\n", "strLength = 32;                                                                                                                                                                  ");
		fprintf(stream, "%s\n", "bufferLength = 4*strLength*strLength;                                                                                                                                            ");
		fprintf(stream, "%s\n", "paramNum = 17;                                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "fileID = fopen(fileName, 'rb');                                                                                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "str = uint8(fread(fileID,strLength,'char'));                                                                                                                                     ");
		fprintf(stream, "%s\n", "DateTXT = char(str(1:end-1)');                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "str = uint8(fread(fileID,strLength,'char'));                                                                                                                                     ");
		fprintf(stream, "%s\n", "TimeTXT = char(str(1:end-1)');                                                                                                                                                   ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "str = uint8(fread(fileID,bufferLength,'char'));                                                                                                                                  ");
		fprintf(stream, "%s\n", "readmeTXT = char(str(1:end-1)');                                                                                                                                                 ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "str = uint8(fread(fileID,paramNum*strLength,'char'));                                                                                                                            ");
		fprintf(stream, "%s\n", "paramTXT = char(str(1:end-1)');                                                                                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% --- Считывем параметры записи -------------------------------------------                                                                                                      ");
		fprintf(stream, "%s\n", "TotalPing = fread(fileID, 1, 'uint32'); % The total number of measurement series                                                                                                 ");
		fprintf(stream, "%s\n", "nSamplesOut = fread(fileID, 1, 'uint32'); % nSamplesOut <= 4096;                                                                                                                 ");
		fprintf(stream, "%s\n", "nSamplesIn = fread(fileID, 1, 'uint32'); % nSamplesIn  <= 8192;                                                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "MaxVolts0 = fread(fileID, 1, 'double'); %  [V] MaxVolts0 <= 5; DAC0 range                                                                                                        ");
		fprintf(stream, "%s\n", "MaxVolts1 = fread(fileID, 1, 'double'); %  [V] MaxVolts1 <= 5; DAC0 range                                                                                                        ");
		fprintf(stream, "%s\n", "fsIn_Hz = fread(fileID, 1, 'double'); % [Hz] Input sampling frequency - частота дискретизации входного сигнала                                                                   ");
		fprintf(stream, "%s\n", "fsOut_Hz = fread(fileID, 1, 'double'); %  [Hz] Output sampling frequency - частота дискретизации выходного сигнала                                                               ");
		fprintf(stream, "%s\n", "TimeScale = fread(fileID, 1, 'uint32'); % TimeScale  // The time scale  1 => (1 tick == 1 us);  10 => (1 tick == 10 us);  100 => (1 tick == 0.1 ms);  1000 => (1 tick == 1 ms);  ");
		fprintf(stream, "%s\n", "DT1 = fread(fileID, 1, 'uint32'); % DT1 - время задержки приема [tick] Must be DT1: DT1 < DT0 - PULSE_WIDTH, where PULSE_WIDTH  == 1;                                            ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "tOut = fread(fileID, [nSamplesOut, 1], 'double'); % [s] временная шкала сигнала на излучение                                                                                     ");
		fprintf(stream, "%s\n", "tIn = fread(fileID, [nSamplesIn, 1], 'double'); % [s] временная шкала принимаемого сигнала                                                                                       ");
		fprintf(stream, "%s\n", "xt0 = fread(fileID, nSamplesOut, 'double'); % нормированное [-1, +1] значение сигнала на излучение CH0                                                                           ");
		fprintf(stream, "%s\n", "xt1 = fread(fileID, nSamplesOut, 'double'); % нормированное [-1, +1] значение сигнала на излучение CH1                                                                           ");
		fprintf(stream, "%s\n", "% --- Считывем параметры записи -------------------------------------------                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% --- Нормируем данные которые выданы на ЦАП ------------------------------                                                                                                      ");
		fprintf(stream, "%s\n", "r0Out = (MaxVolts0) * xt0; % [B] напряжение на ЦАП1                                                                                                                              ");
		fprintf(stream, "%s\n", "r1Out = (MaxVolts1) * xt1; % [B] напряжение на ЦАП2                                                                                                                              ");
		fprintf(stream, "%s\n", "% --- Нормируем данные которые выданы на ЦАП ------------------------------                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% PLOT данные на излучение                                                                                                                                                       ");
		fprintf(stream, "%s\n", "%-------------------------                                                                                                                                                       ");
		fprintf(stream, "%s\n", "fig = figure('Color', 'White');                                                                                                                                                  ");
		fprintf(stream, "%s\n", "set(gca, 'fontname', 'GOST type A', 'FontSize', 16)                                                                                                                              ");
		fprintf(stream, "%s\n", "zoom on; grid on;                                                                                                                                                                ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "ax1 = subplot(2, 1, 1);                                                                                                                                                          ");
		fprintf(stream, "%s\n", "ax2 = subplot(2, 1, 2);                                                                                                                                                          ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "plot(ax1, 0.22+1.0e+6*tOut, r0Out, 'r-', 'LineWidth', 1);                                                                                                                        ");
		fprintf(stream, "%s\n", "hold(ax1, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "plot(ax1, 0.22+1.0e+6*tOut, r1Out, 'b:', 'LineWidth', 1);                                                                                                                        ");
		fprintf(stream, "%s\n", "hold(ax1, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "MaxVolts = max(MaxVolts0, MaxVolts1);                                                                                                                                            ");
		fprintf(stream, "%s\n", "xlim(ax1, [0, max(1.0e+6*tIn)])                                                                                                                                                  ");
		fprintf(stream, "%s\n", "ylim(ax1, 1.25*[-MaxVolts, +MaxVolts])                                                                                                                                           ");
		fprintf(stream, "%s\n", "grid(ax1, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "zoom(ax1, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "xlabel(ax1, 'Time - [us]')                                                                                                                                                       ");
		fprintf(stream, "%s\n", "ylabel(ax1, 'Signal - [V]')                                                                                                                                                      ");
		fprintf(stream, "%s\n", "title(ax1, sprintf('DAC1, DAC2'));                                                                                                                                               ");
		fprintf(stream, "%s\n", "%-------------------------                                                                                                                                                       ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% --- выделяем место для хранения записанных данных -----------------------                                                                                                      ");
		fprintf(stream, "%s\n", "xr0 = zeros(nSamplesIn, TotalPing);                                                                                                                                              ");
		fprintf(stream, "%s\n", "xr1 = zeros(nSamplesIn, TotalPing);                                                                                                                                              ");
		fprintf(stream, "%s\n", "% --- выделяем место для хранения записанных данных -----------------------                                                                                                      ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "% PLOT данные первичных измерений                                                                                                                                                ");
		fprintf(stream, "%s\n", "%-------------------------                                                                                                                                                       ");
		fprintf(stream, "%s\n", "for ping = 1:TotalPing                                                                                                                                                           ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "    record_num = fread(fileID, 1, 'uint32'); % считываем номер измерения                                                                                                         ");
		fprintf(stream, "%s\n", "    xr0(:, ping) = fread(fileID, [nSamplesIn, 1], 'double'); % [В] считываем результаты АЦП1                                                                                     ");
		fprintf(stream, "%s\n", "    xr1(:, ping) = fread(fileID, [nSamplesIn, 1], 'double'); % [В] считываем результаты АЦП2                                                                                     ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "    plot(ax2, 1.0e+6*tIn, xr0(:, ping), 'r-', 'LineWidth', 1);       hold(ax2, 'on');                                                                                            ");
		fprintf(stream, "%s\n", "    plot(ax2, 1.0e+6*tIn, xr1(:, ping), 'b-', 'LineWidth', 1);       hold(ax2, 'on');                                                                                            ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "    drawnow;                                                                                                                                                                     ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "end                                                                                                                                                                              ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "MaxVolts = max(MaxVolts0, MaxVolts1);                                                                                                                                            ");
		fprintf(stream, "%s\n", "xlim(ax2, [0, max(1.0e+6*tIn)])                                                                                                                                                  ");
		fprintf(stream, "%s\n", "ylim(ax2, 1.25*[-MaxVolts, +MaxVolts])                                                                                                                                           ");
		fprintf(stream, "%s\n", "grid(ax2, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "zoom(ax2, 'on');                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", "xlabel(ax2, 'Time - [us]')                                                                                                                                                       ");
		fprintf(stream, "%s\n", "ylabel(ax2, 'Signal - [V]')                                                                                                                                                      ");
		fprintf(stream, "%s\n", "title(ax2, sprintf('ADC1, ADC2'));                                                                                                                                               ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");
		fprintf(stream, "%s\n", " linkaxes([ax1, ax2],'x')                                                                                                                                                        ");
		fprintf(stream, "%s\n", "% ---- Close file handel ----------------------                                                                                                                                  ");
		fprintf(stream, "%s\n", "fclose(fileID);                                                                                                                                                                  ");
		fprintf(stream, "%s\n", "                                                                                                                                                                                 ");

		fclose(stream);
	}
	else
	{ // file open error check
		fprintf(stderr, "ERROR: The example of 'plotResult.m' file was not created.\n");
	}

}

void config_parsing(const char* filename)
{
	FILE* stream;

	stream = fopen(filename, "r"); //Last line in file InPut.dat must be correct - NOT EMPTY LINE

	if (stream != NULL)
	{
		char str[512] = { 0 };
		unsigned int n;

		ping = 0;
		PULSE_WIDTH = 1;

		n = fscanf(stream, "%d%255[^\n\r]", &TimeScale, &(str[0]));

		unsigned int TimeScaleArr[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
		unsigned int TimeScaleArrLen = 10;
		unsigned int TimeScaleArrFlag = 0;
		for (unsigned int i = 0; i < TimeScaleArrLen; i++)
		{
			if (TimeScale != TimeScaleArr[i])
			{
				TimeScaleArrFlag = TimeScaleArrFlag;
			}
			else
			{
				TimeScaleArrFlag = TimeScaleArrFlag + 1;
			}
		}

		if (0 == TimeScaleArrFlag)
		{
			fprintf(stderr, "ERROR: The 'TimeScale' in 'config.dat' must be one of {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000}\n");
		}
		//printf("TimeScale = %u  %s\n", TimeScale, str);

		n = fscanf(stream, "%d%255[^\n\r]", &DT0, &(str[0]));
		if (DT0 > 512 + 32768)
		{
			fprintf(stderr, "ERROR: The 'DT0' in 'config.dat' must be DT0 <= 512 + 32768\n");
		}
		//printf("DT0 = %u  %s\n", DT0, str);

		n = fscanf(stream, "%d%255[^\n\r]", &DT1, &(str[0]));
		if (DT1 >= DT0 - PULSE_WIDTH)
		{
			fprintf(stderr, "ERROR: The 'DT1' in 'config.dat' must be DT1 < DT0 - PULSE_WIDTH\n");
		}
		//printf("DT1 = %u  %s\n", DT1, str);

		n = fscanf(stream, "%d%255[^\n\r]", &TotalPing, &(str[0]));
		//printf("TotalPing = %u  %s\n", TotalPing, str);
		n = fscanf(stream, "%d%255[^\n\r]", &TimeDelay, &(str[0])); // [ms] Time delay between measurments series, real time delay ~40 ms + TimeDelay
		//printf("TimeDelay = %u  %s\n", TimeDelay, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &MaxVolts0, &(str[0])); // MaxVolts < 5; DAC range
		if (MaxVolts0 > 5.0)
		{
			fprintf(stderr, "ERROR: The 'MaxVolts0' in 'config.dat' must be MaxVolts0 <= 5.0\n");
		}
		//printf("MaxVolts0 = %lf  %s\n", MaxVolts0, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &MaxVolts1, &(str[0])); // MaxVolts < 5; DAC range
		if (MaxVolts1 > 5.0)
		{
			fprintf(stderr, "ERROR: The 'MaxVolts1' in 'config.dat' must be MaxVolts1 <= 5.0\n");
		}
		//printf("MaxVolts1 = %lf  %s\n", MaxVolts1, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &voltsRange0, &(str[0])); // voltsRange0 = 5.0 OR 50.0; ADC range
		if ((voltsRange0 != VOLTS_RANGE_LOW) && (voltsRange0 != VOLTS_RANGE_HIGH))
		{
			fprintf(stderr, "ERROR: The 'voltsRange0' in 'config.dat' must be one of {5.0, 50.0}\n");
		}
		//printf("voltsRange0 = %lf  %s\n", voltsRange0, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &voltsRange1, &(str[0])); // voltsRange1 = 5.0 OR 50.0; ADC range
		if ((voltsRange1 != VOLTS_RANGE_LOW) && (voltsRange1 != VOLTS_RANGE_HIGH))
		{
			fprintf(stderr, "ERROR: The 'voltsRange1' in 'config.dat' must be one of {5.0, 50.0}\n");
		}
		//printf("voltsRange1 = %lf  %s\n", voltsRange1, str);

		n = fscanf(stream, "%d%255[^\n\r]", &fm_type, &(str[0])); // mf_type = {0,1,2} 0 -> mono pulse; 1 -> sweep pulse; 2 -> custom (read file)
		if ((fm_type != 0) && (fm_type != 1) && (fm_type != 2))
		{
			fprintf(stderr, "ERROR: The 'fm_type' in 'config.dat' must be one of {0, 1, 2}\n");
		}
		//printf("fm_type = %u  %s\n", fm_type, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &fi_Hz, &(str[0])); // [Hz] The carrier frequency of the monochromatic pulse
		//printf("fi_Hz = %lf  %s\n", fi_Hz, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &f0_Hz, &(str[0])); // [Hz] Start frequency band of signal
		//printf("f0_Hz = %lf  %s\n", f0_Hz, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &f1_Hz, &(str[0])); // [Hz] Stop frequency band of signal
		//printf("f1_Hz = %lf  %s\n", f1_Hz, str);

		n = fscanf(stream, "%d%255[^\n\r]", &nSamplesOut, &(str[0])); // nSamplesOut <=  nSamplesOutMax;
		if (nSamplesOut > nSamplesOutMax)
		{
			fprintf(stderr, "ERROR: The 'nSamplesOut' in 'config.dat' must be nSamplesOut <= 4096\n");
		}
		//printf("nSamplesOut = %u  %s\n", nSamplesOut, str);

		n = fscanf(stream, "%d%255[^\n\r]", &nSamplesIn, &(str[0])); // nSamplesIn <=  nSamplesInMax;
		if (nSamplesIn > nSamplesInMax)
		{
			fprintf(stderr, "ERROR: The 'nSamplesIn' in 'config.dat' must be nSamplesIn <= 8192\n");
		}
		//printf("nSamplesIn = %u  %s\n", nSamplesIn, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &fsIn_Hz, &(str[0])); // [Hz] 
		//printf("fsIn_Hz = %lf  %s\n", fsIn_Hz, str);

		n = fscanf(stream, "%lf%255[^\n\r]", &fsOut_Hz, &(str[0])); // [Hz]
		//printf("fsOut_Hz = %lf  %s\n", fsOut_Hz, str);

		TsOut_s = 1.0 / fsOut_Hz;           // [s] Output sampling time
		Ts_sec = TsOut_s * nSamplesOut;     // [s] Signal emission time - время излучения, частота дискретизации определяется через время излучения Ts_sec
		TsIn_s = 1.0 / fsIn_Hz;             // [s] Input sampling time

		// The output signal time [s]
		for (unsigned int i = 0; i < nSamplesOut; i++)
		{
			tOut[i] = TsOut_s * i;
		}

		// The input signal time [s]
		for (unsigned int i = 0; i < nSamplesIn; i++)
		{
			tIn[i] = (1.0e-6) * TimeScale * DT1 + TsIn_s * i;
		}

		fclose(stream);
	}
	else
	{ // file open error check
		fprintf(stderr, "ERROR: The 'config.dat' file was not opened. fopen_s - failure\n");
	}
}

#endif // REFACTORING
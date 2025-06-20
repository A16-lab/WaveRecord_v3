
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <mex.h> /* only needed because of mexFunction below and mexPrintf */

#define EXPORT_FCNS

#include "ad2lib.h"
#include "dwflib.h"

#define MaxVolts 5.0
#define nSamplesIn 8192
#define nSamplesOut 4096
#define printf mexPrintf

STS sts;

int true_mex = 1;
int false_mex = 0;

double pi = 3.1415926535897932384626433832795;

double voltsRange0 = 5.0;
double voltsRange1 = 50.0;

double hzSmpFreq = 1000000.0; // [Hz] Input sampling frequency - частота дискретизации входного сигнала
double hzRecFreq;

double Ts_sec = 0.001; // [sec] Signal emission time - время излучения, частота дискретизации определяется через время излучения T0_sec
clock_t dT0gap_ms = 20; // [msec] Time gap before emission of signal. For electric stabilization - время до начала излучения, что бы установились электрические параметры
clock_t dT1gap_ms = 20; // [msec] Time gap between emission of signal - время после излучения для затухания реверберации

// Pauses for a specified number of milliseconds.
#include <time.h>
EXPORTED_FUNCTION void sleep(clock_t wait)
{
	clock_t goal;
	goal = wait + clock();
	while (goal > clock())
		;
}

EXPORTED_FUNCTION int Array2In2OutConfigure(HDWF hdwf, double *xr0, double *xt0, double *xr1, double *xt1, double hzSmpFreq)
{

	mexPrintf("Start device configure\n");

	// ---------------------------------------------------------------------------------------
	// FDwfAnalogOutReset(HDWF hdwf, int idxChannel)
	FDwfAnalogOutReset(hdwf, -1); // To reset instrument parameters across all channels, set idxChannel to - 1.

	// enable channel
	FDwfAnalogOutNodeEnableSet(hdwf, 0, AnalogOutNodeCarrier, true_mex);
	FDwfAnalogOutNodeEnableSet(hdwf, 1, AnalogOutNodeCarrier, true_mex);

	// 2V amplitude, 4V pk2pk, for sample value -1 will output -2V, for 1 +2V
	FDwfAnalogOutNodeAmplitudeSet(hdwf, 0, AnalogOutNodeCarrier, MaxVolts);
	FDwfAnalogOutNodeAmplitudeSet(hdwf, 1, AnalogOutNodeCarrier, MaxVolts);

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
	FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcPC);
	FDwfAnalogOutTriggerSourceSet(hdwf, 1, trigsrcPC);
	// -----------------------------------------------------------------------------------------------

	// -----------------------------------------------------------------------------------------------
	// The function resets and configures(by default, having auto configure enabled) all AnalogIn instrument parameters to default values.
	FDwfAnalogInReset(hdwf);

	// set the  buffer size
	FDwfAnalogInBufferSizeSet(hdwf, nSamplesIn);

	// The function is used to configure the horizontal trigger position in seconds.
	double secPosition = ((double)(nSamplesIn) / hzSmpFreq / 2);
	FDwfAnalogInTriggerPositionSet(hdwf, secPosition);

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

	// The function is used to set the sample frequency for the instrument.
	FDwfAnalogInFrequencySet(hdwf, hzSmpFreq);

	// Set the acquisition filter for each AnalogIn channel.
	// FILTER: -- filterDecimate : Looks for trigger in each ADC conversion, can detect glitches.
	// FILTER: -- filterAverage : Looks for trigger only in average of N samples, given by FDwfAnalogInFrequencySet.
	FDwfAnalogInChannelFilterSet(hdwf, 0, filterAverage);
	FDwfAnalogInChannelFilterSet(hdwf, 1, filterAverage);

	// configure trigger
	FDwfAnalogInTriggerSourceSet(hdwf, trigsrcPC);

	// FDwfAnalogInTriggerChannelSet(HDWF hdwf, int idxChannel)
	// -hdwf – Interface handle.
	// -idxChannel – Trigger channel index to set.
	// The function is used to set the trigger channel.
	FDwfAnalogInTriggerChannelSet(hdwf, 0);
	FDwfAnalogInTriggerChannelSet(hdwf, 1);

	// disable auto trigger
	double secTimeout = 0;
	FDwfAnalogInTriggerAutoTimeoutSet(hdwf, secTimeout);
	// -----------------------------------------------------------------------------------------------

	mexPrintf("Device configure done\n");
}

EXPORTED_FUNCTION int Array2In2OutRun(HDWF hdwf, double *xr0, double *xt0, double *xr1, double *xt1)
{

	//printf("Start device \n");

	char szError[512] = {0};

	// -----------------------------------------------------------------------------------------------
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
	hzRecFreq = 1.0 / Ts_sec;
	FDwfAnalogOutNodeFrequencySet(hdwf, 0, AnalogOutNodeCarrier, hzRecFreq);
	FDwfAnalogOutNodeFrequencySet(hdwf, 1, AnalogOutNodeCarrier, hzRecFreq);
	// -----------------------------------------------------------------------------------------------

	// -----------------------------------------------------------------------------------------------
	//printf("--Arm set Up to signal generation & acquisition-- \n");

	// Arm set Up to signal generation
	FDwfAnalogOutConfigure(hdwf, 0, true_mex);
	do
	{ // Must be after  FDwfAnalogOutConfigure(hdwf, 0, true_mex);
		FDwfAnalogOutStatus(hdwf, 0, &sts);
		//printf(" sts = %d\n", sts); // -> sts == 1 Armed
	} while (sts != stsArm);
	//printf("OutPutLine 0 is Armed\n");

	// Arm set Up to signal generation
	FDwfAnalogOutConfigure(hdwf, 1, true_mex);
	do
	{ // Must be after  FDwfAnalogOutConfigure(hdwf, 1, true_mex);
		FDwfAnalogOutStatus(hdwf, 1, &sts);
		//printf(" sts = %d\n", sts); // -> sts == 1 Armed
	} while (sts != stsArm);
	//printf("OutPutLine 1 is Armed\n");

	// Arm Set Up to signal acquisition
	FDwfAnalogInConfigure(hdwf, 0, true_mex);
	do
	{
		FDwfAnalogInStatus(hdwf, 0, &sts);
		//printf(" sts = %d\n", sts);
	} while (sts != stsArm);
	//printf("InPutLine's is Armed\n");

	FDwfAnalogInStatus(hdwf, true_mex, &sts);
	//printf("FDwfAnalogInStatus sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 0, &sts);
	//printf("FDwfAnalogOutStatus 0 sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 1, &sts);
	//printf("FDwfAnalogOutStatus 1 sts = %d\n", sts);
	// -----------------------------------------------------------------------------------------------

	// -----------------------------------------------------------------------------------------------
	//printf("--Trigged 1 -- \n");
	// The function generates one pulse on the PC trigger line.
	if (!FDwfDeviceTriggerPC(hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		printf("DeviceTriggerPC failed\n\t%s", szError);
		return 0;
	}

	do
	{										// printf("Waiting for the emission end\n");
		FDwfAnalogOutStatus(hdwf, 0, &sts); // printf(" sts = %d\n", sts);
	} while (sts != stsDone);

	do
	{										// printf("Waiting for the emission end\n");
		FDwfAnalogOutStatus(hdwf, 1, &sts); // printf(" sts = %d\n", sts);
	} while (sts != stsDone);

	do
	{
		FDwfAnalogInStatus(hdwf, true_mex, &sts);
	} while (sts != stsDone);

	FDwfAnalogInStatus(hdwf, true_mex, &sts);
	//printf("FDwfAnalogInStatus sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 0, &sts);
	//printf("FDwfAnalogOutStatus 0 sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 1, &sts);
	//printf("FDwfAnalogOutStatus 1 sts = %d\n", sts);
	// -----------------------------------------------------------------------------------------------

	// -----------------------------------------------------------------------------------------------
	// get the samples for each channel
	// printf("--Get the samples for each channel--\n");

	FDwfAnalogInStatusData(hdwf, 0, xr0, nSamplesIn);
	FDwfAnalogInStatusData(hdwf, 1, xr1, nSamplesIn);

	FDwfAnalogInStatus(hdwf, true_mex, &sts);
	//printf("FDwfAnalogInStatus sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 0, &sts);
	//printf("FDwfAnalogOutStatus 0 sts = %d\n", sts);

	FDwfAnalogOutStatus(hdwf, 1, &sts);
	//printf("FDwfAnalogOutStatus 1 sts = %d\n", sts);
	// -----------------------------------------------------------------------------------------------

	//mexPrintf("sleep dT1gap_ms = %d\n", dT1gap_ms);

	sleep(dT1gap_ms); // sleep in msec

	//printf("Device stop\n");
}

EXPORTED_FUNCTION int Stream2In2OutRun(HDWF hdwf, double *xr0, double *xt0, double *xr1, double *xt1, double hzSmpFreq, int nSamples)
{

	STS sts;

	char szError[512] = {0};

	int true_mex = 1;
	int false_mex = 0;

	// int nSamples = 0; // [smpl]
	// double hzSmpFreq = 0; // [Hz]

	int cSamplesIn, pcdDataAvailableIn, pcdDataLostIn, pcdDataCorruptIn;
	int cSamplesOut0, cdDataFreeOut0, cdDataLostOut0, cdDataCorruptedOut0;
	int cSamplesOut1, cdDataFreeOut1, cdDataLostOut1, cdDataCorruptedOut1;
	int fLost = false_mex, fCorrupted = false_mex;

	// ---- AnalogIn configure -----------------------------------------------------
	// The function resets and configures(by default, having auto configure enabled) all AnalogIn instrument parameters to default values.
	FDwfAnalogInReset(hdwf);

	// FDwfAnalogInChannelEnableSet(HDWF hdwf, int idxChannel, BOOL fEnable)
	//	- hdwf � Interface handle.
	//	- idxChannel � Zero based index of channel to enable / disable.
	//	- fEnable � Set true_mex to enable, false_mex to disable.
	// The function above is used to enable or disable the specified AnalogIn channel.
	FDwfAnalogInChannelEnableSet(hdwf, 0, true_mex);
	FDwfAnalogInChannelEnableSet(hdwf, 1, true_mex);

	// The function is used to set the sample frequency for the instrument.
	FDwfAnalogInFrequencySet(hdwf, hzSmpFreq);

	// set the  buffer size
	// FDwfAnalogInBufferSizeSet(HDWF hdwf, int nSize)
	// - hdwf � Interface handle.
	// - nSize � Buffer size to set.
	// The function above is used to adjust the AnalogIn instrument buffer size.
	FDwfAnalogInBufferSizeSet(hdwf, nSamplesIn);

	// FDwfAnalogInChannelRangeSet(HDWF hdwf, int idxChannel, double voltsRange)
	// -hdwf � Interface handle.
	// - idxChannel � Channel index.
	// - voltsRange � Voltage range to set.
	// The function is used to configure the range for each channel.
	// With channel index "-1", each enabled Analog In channel range will be configured to the same, new value.
	// Set pk2pk input range for 0 channels
	// set 2.5V pk2pk input range, -2.5V to 2.5V: FDwfAnalogInChannelRangeSet(hdwf, 0, 2.5);
	FDwfAnalogInChannelRangeSet(hdwf, 0, voltsRange0);
	FDwfAnalogInChannelRangeSet(hdwf, 1, voltsRange1);

	// FDwfAnalogInChannelFilterSet(HDWF hdwf, int idxChannel, FILTER filter)
	// - hdwf � Interface handle.
	// - idxChannel � Channel index.
	// - filter � Acquisition sample filter to set.
	// The function above is used to set the acquisition filter for each AnalogIn channel.
	// FILTER:
	//		-- filterDecimate : Looks for trigger in each ADC conversion, can detect glitches.
	//		-- filterAverage : Looks for trigger only in average of N samples, given by FDwfAnalogInFrequencySet.
	// FDwfAnalogInChannelFilterSet(hdwf, 0, filterDecimate);
	// FDwfAnalogInChannelFilterSet(hdwf, 1, filterDecimate);
	FDwfAnalogInChannelFilterSet(hdwf, 0, filterAverage);
	FDwfAnalogInChannelFilterSet(hdwf, 1, filterAverage);

	// FDwfAnalogInAcquisitionModeSet(HDWF hdwf, ACQMODE acqmode)
	// - hdwf � Interface handle.
	// - acqmode � Acquisition mode to set.
	// The function above is used to set the acquisition mode.
	//					acqmodeRecord - Perform acquisition for length of time set by FDwfAnalogInRecordLengthSet.
	FDwfAnalogInAcquisitionModeSet(hdwf, acqmodeRecord);

	// FDwfAnalogInRecordLengthSet(HDWF hdwf, double sLegth)
	// - hdwf � Interface handle.
	// - sLegth � Record length to set expressed in seconds.
	// The function is used to set the Record length in seconds.
	FDwfAnalogInRecordLengthSet(hdwf, (double)nSamples / hzSmpFreq);

	// FDwfAnalogInTriggerChannelSet(HDWF hdwf, int idxChannel)
	// -hdwf  Interface handle.
	// -idxChannel  Trigger channel index to set.
	// The function is used to set the trigger channel.
	FDwfAnalogInTriggerChannelSet(hdwf, 0);
	FDwfAnalogInTriggerChannelSet(hdwf, 1);

	// FDwfAnalogInTriggerPositionSet(HDWF hdwf, double secPosition)
	//- hdwf � Interface handle.
	//- secPosition � Trigger position to set.
	// The function is used to configure the horizontal trigger position in seconds.
	double secPosition = 0;
	FDwfAnalogInTriggerPositionSet(hdwf, secPosition);

	// configure trigger
	// FDwfAnalogInTriggerSourceSet(HDWF hdwf, TRIGSRC trigsrc)
	// -hdwf � Interface handle.
	// -trigsrc � Trigger source to set. // trigsrcDetectorAnalogIn - Trigger detector on analog in channels.
	// The function is used to configure the AnalogIn acquisition trigger source.
	// FDwfAnalogInTriggerSourceSet(hdwf, trigsrcDetectorAnalogIn);
	FDwfAnalogInTriggerSourceSet(hdwf, trigsrcPC);
	// ---------------------------------------------------------------------------------------

	// ---- AnalogOut configure -----------------------------------------------------
	// resets and configures all AnalogOut instrument
	// FDwfAnalogOutReset(HDWF hdwf, int idxChannel)
	// - hdwf � Interface handle.
	// - idxChannel � Channel index.
	// The function resets and configures(by default, having auto configure enabled) all AnalogOut instrument
	// parameters to default values for the specified channel.
	// To reset instrument parameters across all channels, set idxChannel to - 1.
	FDwfAnalogOutReset(hdwf, -1);

	// enable  all channel
	// FDwfAnalogOutNodeEnableSet(	HDWF hdwf, int idxChannel, AnalogOutNode node, BOOL fEnable)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - fEnable � true_mex to enable, false_mex to disable.
	// The function enables or disables the channel node specified by idxChannel and node.
	// The Carrier node enables or disables the channel and AM / FM the modulation.
	// With channel index "-1", each Analog Out channel enable will be configured to use the same, new option
	FDwfAnalogOutNodeEnableSet(hdwf, -1, AnalogOutNodeCarrier, true_mex);

	// sample frequency for continuos play
	// FDwfAnalogOutNodeFrequencySet(HDWF hdwf, int idxChannel, AnalogOutNode node, double hzFrequency)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - hzFrequency � Frequency value to set expressed in Hz.
	// The function above is used to set the frequency.
	// With channel index "-1", each enabled Analog Out channel frequency will be configured to use the same, new option.
	FDwfAnalogOutNodeFrequencySet(hdwf, -1, AnalogOutNodeCarrier, hzSmpFreq);

	// by default the offset is 0V
	// FDwfAnalogOutNodeOffsetSet(HDWF hdwf, int idxChannel, AnalogOutNode node, double vOffset)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - vOffset � Value to set voltage offset in Volts or modulation offset percentage.
	// The function above is used to set the offset value for the specified channel - node on the instrument.
	FDwfAnalogOutNodeOffsetSet(hdwf, 0, AnalogOutNodeCarrier, 0);
	FDwfAnalogOutNodeOffsetSet(hdwf, 1, AnalogOutNodeCarrier, 0);

	// FDwfAnalogOutNodeAmplitudeSet(HDWF hdwf, int idxChannel, AnalogOutNode node, double vAmplitude)
	//	-hdwf � Open interface handle on a device.
	//	- idxChannel � Channel index.
	//	- node � Node index.
	//	- vAmplitude � Amplitude of channel in Volts or modulation index in percentage.
	// The function above is used to set the amplitude or modulation index for the specified channel - node on the
	// instrument. With channel index "-1", each enabled Analog Out channel amplitude(or modulation index) will be configured to use the same, new option.
	FDwfAnalogOutNodeAmplitudeSet(hdwf, -1, AnalogOutNodeCarrier, MaxVolts);

	// FDwfAnalogOutIdleSet(HDWF hdwf, int idxChannel, DwfAnalogOutIdle idle)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - idle � Generator function option to set.
	// The function above is used to set the generator idle output for the specified instrument
	//	DwfAnalogOutIdleOffset	-	The idle output is the configured Offset level.
	//	DwfAnalogOutIdleInitial	-	The idle output voltage level has the initial waveform value of the current configuration.
	//								This depends on the selected signal type, Offset, Amplitude and Amplitude Modulator configuration.
	// FDwfAnalogOutIdleSet(hdwf, 0, DwfAnalogOutIdleInitial);
	FDwfAnalogOutIdleSet(hdwf, 0, DwfAnalogOutIdleOffset);
	FDwfAnalogOutIdleSet(hdwf, 1, DwfAnalogOutIdleOffset);

	// set custom function
	// FDwfAnalogOutNodeFunctionSet( HDWF hdwf, int idxChannel, AnalogOutNode node, FUNC func)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - func � Generator function option to set.
	// The function is used to set the generator output function for the specified instrument channel.
	// With channel	index "-1", each enabled Analog Out channel function will be configured to use the same, new option.
	FDwfAnalogOutNodeFunctionSet(hdwf, -1, AnalogOutNodeCarrier, funcPlay);

	// set custom waveform samples normalized to �1 values
	// FDwfAnalogOutNodeDataSet( HDWF hdwf, int idxChannel, AnalogOutNode node, double *rgdData, int cdData)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - rgbData � Buffer of samples to set.
	// - cData � Number of samples to set in rgbData.
	// The function above is used to set the custom data or to prefill the buffer with play samples.
	// The samples are double precision floating point values(rgdData) normalized to �1.
	// With the custom function option, the data samples(cdData) will be interpolated to the device buffer size.
	// The output value will be Offset + Sample*Amplitude, for instance:
	// - 0 value sample will output : Offset.
	// - +1 value sample will output : Offset + Amplitude.
	// - -1 value sample will output : Offset � Amplitude.
	cSamplesOut0 = 4096;
	cSamplesOut1 = 4096;
	FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, &xt0[0], cSamplesOut0);
	FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, &xt1[0], cSamplesOut1);

	// set the trigger source for the channel on instrument.
	// FDwfAnalogOutTriggerSourceSet(HDWF hdwf, int idxChannel, TRIGSRC trigsrc)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - trigsrc � Trigger source to set.
	// The function is used to set the trigger source for the channel on instrument.
	FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcPC); // FDwfAnalogOutStatus(hdwf, 0, &sts);  // mexPrintf("AnalogOutTriggerSourceSet sts0 = %d\n", sts); // Wait(3); // -> sts == 0 Ready
	FDwfAnalogOutTriggerSourceSet(hdwf, 1, trigsrcPC); // FDwfAnalogOutStatus(hdwf, 1, &sts);  // mexPrintf("AnalogOutTriggerSourceSet sts1 = %d\n", sts); // Wait(3); // -> sts == 0 Ready

	// start
	// FDwfAnalogInConfigure(HDWF hdwf, BOOL fReconfigure, BOOL fStart)
	// -hdwf � Interface handle.
	// -fReconfigure � Configure the device.
	// -fStart � Start the acquisition.
	// The function is used to configure the instrument and start or stop the acquisition.
	// To reset the Auto trigger timeout, set fReconfigure to true_mex.
	FDwfAnalogInConfigure(hdwf, false_mex, true_mex);
	do
	{ // Acquisition not yet started.
		if (!FDwfAnalogInStatus(hdwf, true_mex, &sts))
		{
			FDwfGetLastErrorMsg(szError);
			mexPrintf("AnalogInStatus Error\n\t%s", szError);
			return 0;
		}
	} while (sts != stsArm);
	// mexPrintf("Analog InPutLine 0 & 1 armed \n");

	// start signal generation
	// FDwfAnalogOutConfigure(HDWF hdwf, int idxChannel, BOOL fStart)
	// - hdwf � Interface handle.
	// - idxChannel � Channel index.
	// - fStart � Start the instrument. To stop, set to false_mex.
	// The function is used to start or stop the instrument.
	// With channel index "-1", each enabled Analog Out channel will be configured.
	FDwfAnalogOutConfigure(hdwf, -1, true_mex);

	do
	{ // Acquisition not yet started.
		if (!FDwfAnalogOutStatus(hdwf, 0, &sts))
		{
			FDwfGetLastErrorMsg(szError);
			mexPrintf("Analog Out 0 Status Error \n");
			return 0;
		}
	} while (sts != stsArm); //(sts == stsCfg || sts == stsPrefill || sts == stsArm);
	// mexPrintf("OutPutLine 0 armed \n");

	do
	{ // Acquisition not yet started.
		if (!FDwfAnalogOutStatus(hdwf, 1, &sts))
		{
			FDwfGetLastErrorMsg(szError);
			mexPrintf("Analog Out 1 Status Error \n");
			return 0;
		}
	} while (sts != stsArm); //(sts == stsCfg || sts == stsPrefill || sts == stsArm);
	// mexPrintf("OutPutLine 1 armed \n");

	// wait at least 2 seconds with Analog Discovery for the offset to stabilize, before the first reading after device open or offset/range change
	sleep(200);

	// mexPrintf("--Trigged-- \n");
	//  The function generates one pulse on the PC trigger line.
	if (!FDwfDeviceTriggerPC(hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		mexPrintf("DeviceTriggerPC failed\n\t%s", szError);
		return 0;
	}

	// --------------------------------------------------------------------------------------

	// if (!FDwfAnalogOutStatus(hdwf, 0, &sts)) {
	//	FDwfGetLastErrorMsg(szError);
	//	mexPrintf("AnalogOutStatus 0 - Error\n\t%s", szError);
	//	return 0;
	// }
	// mexPrintf("AnalogOutStatus 0 sts0 = %d\n", sts);
	//

	// if (!FDwfAnalogOutStatus(hdwf, 1, &sts)) {
	//	FDwfGetLastErrorMsg(szError);
	//	mexPrintf("AnalogOutStatus 1 - Error\n\t%s", szError);
	//	return 0;
	// }
	// mexPrintf("AnalogOutStatus 1 sts1 = %d\n", sts);

	// if (!FDwfAnalogInStatus(hdwf, true_mex, &sts)) {
	//	FDwfGetLastErrorMsg(szError);
	//	mexPrintf("AnalogInStatus Error\n\t%s", szError);
	//	return 0;
	// }
	// mexPrintf("AnalogInStatus sts = %d\n", sts);

	// --------------------------------------------------------------------------------

	// FDwfAnalogOutNodePlayStatus(HDWF hdwf, int idxChannel, AnalogOutNode node, 	int *cdDataFree, int *cdDataLost, int *cdDataCorrupted)
	// - hdwf � Open interface handle on a device.
	// - idxChannel � Channel index.
	// - node � Node index.
	// - cdDataFree � Pointer to variable to return the available free buffer space, the number of new samples that can be sent.
	// - cdDataLost � Pointer to variable to return the number of lost samples.
	// - cdDataCorrupted � Pointer to variable to return the number of samples that could be corrupted.
	// The function above is used to retrieve information about the play process.
	// The data lost occurs when the device generator is faster than the sample send process from the PC. In this case, the device buffer gets emptied and generated samples are repeated.
	// Corrupt samples are a warning that the buffer might have been emptied while samples were sent to the device.
	// In this case, try optimizing the loop for faster execution; or reduce the frequency or run time to be less or equal to the device buffer size (run time <= buffer size / frequency).
	FDwfAnalogOutNodePlayStatus(hdwf, 0, AnalogOutNodeCarrier, &cdDataFreeOut0, &cdDataLostOut0, &cdDataCorruptedOut0);
	if (cdDataLostOut0 || cdDataCorruptedOut0)
	{
		fLost = true_mex;
		mexPrintf("AnalogOut 0 - cdDataLostOut0 || cdDataCorruptedOut0 - Error \n");
		mexPrintf("cdDataLostOut0      %d \n", cdDataLostOut0);
		mexPrintf("cdDataCorruptedOut0 %d \n", cdDataCorruptedOut0);
		return 0;
	}

	FDwfAnalogOutNodePlayStatus(hdwf, 1, AnalogOutNodeCarrier, &cdDataFreeOut1, &cdDataLostOut1, &cdDataCorruptedOut1);
	if (cdDataLostOut1 || cdDataCorruptedOut1)
	{
		fLost = true_mex;
		mexPrintf("AnalogOut 1 - cdDataLostOut1 || cdDataCorruptedOut1 - Error \n");
		mexPrintf("cdDataLostOut1      %d \n", cdDataLostOut1);
		mexPrintf("cdDataCorruptedOut1 %d \n", cdDataCorruptedOut1);
		return 0;
	}

	cSamplesIn = 0;
	pcdDataAvailableIn = 0;		

	// FDwfAnalogInStatusRecord( HDWF hdwf, int *pcdDataAvailable, int *pcdDataLost, int *pcdDataCorrupt)
	// - hdwf � Interface handle.
	// - pcdDataAvailable � Pointer to variable to receive the available number of samples.
	// - pcdDataLost � Pointer to variable to receive the lost samples after the last check.
	// - pcdDataCorrupt � Pointer to variable to receive the number of samples that could be corrupt.
	// The function is used to retrieve information about the recording process. The data loss occurs when the
	// device acquisition is faster than the read process to PC. In this case, the device recording buffer is filled and data
	// samples are overwritten. Corrupt samples indicate that the samples have been overwritten by the acquisition
	// process during the previous read. In this case, try optimizing the loop process for faster execution or reduce the
	// acquisition frequency or record length to be less than or equal to the device buffer size(record length <= buffer size / frequency).
	FDwfAnalogInStatusRecord(hdwf, &pcdDataAvailableIn, &pcdDataLostIn, &pcdDataCorruptIn);
	if (pcdDataLostIn || pcdDataCorruptIn)
	{
		fLost = true_mex;
		mexPrintf("AnalogIn - cdDataLostIn || cdDataCorruptedIn - Error \n");
		mexPrintf("pcdDataLostIn    %d \n", pcdDataLostIn);
		mexPrintf("pcdDataCorruptIn %d \n", pcdDataCorruptIn);
		return 0;
	}

	while ((cSamplesIn < nSamples) || (cSamplesOut0 < nSamples) || (cSamplesOut1 < nSamples))
	{

		if (cSamplesIn + pcdDataAvailableIn < nSamples)
		{
			// get samples
			// FDwfAnalogInStatusData( HDWF hdwf, int idxChannel, double *rgdVoltData, int cdData)
			// - hdwf � Interface handle.
			// - idxChannel � Channel index.
			// - rgdVoltData � Pointer to allocated buffer to copy the acquisition data.
			// - cdData � Number of samples to copy.
			// The function is used to retrieve the acquired data samples from the specified idxChannel on the AnalogIn instrument. 
			// It copies the data samples to the provided buffer.
			FDwfAnalogInStatusData(hdwf, 0, &xr0[cSamplesIn], pcdDataAvailableIn);
			FDwfAnalogInStatusData(hdwf, 1, &xr1[cSamplesIn], pcdDataAvailableIn);
			cSamplesIn += pcdDataAvailableIn;

			if (!FDwfAnalogInStatus(hdwf, true_mex, &sts))
			{
				FDwfGetLastErrorMsg(szError);
				mexPrintf("AnalogInStatus Error\n\t%s", szError);
				mexPrintf("AnalogInStatus sts  = %d\n", sts);
				return 0;
			}
			//mexPrintf("AnalogInStatus sts  = %d\n", sts);

			// FDwfAnalogInStatusRecord( HDWF hdwf, int *pcdDataAvailable, int *pcdDataLost, int *pcdDataCorrupt)
			// - hdwf � Interface handle.
			// - pcdDataAvailable � Pointer to variable to receive the available number of samples.
			// - pcdDataLost � Pointer to variable to receive the lost samples after the last check.
			// - pcdDataCorrupt � Pointer to variable to receive the number of samples that could be corrupt.
			// The function is used to retrieve information about the recording process. The data loss occurs when the
			// device acquisition is faster than the read process to PC. In this case, the device recording buffer is filled and data
			// samples are overwritten. Corrupt samples indicate that the samples have been overwritten by the acquisition
			// process during the previous read. In this case, try optimizing the loop process for faster execution or reduce the
			// acquisition frequency or record length to be less than or equal to the device buffer size(record length <= buffer size / frequency).
			FDwfAnalogInStatusRecord(hdwf, &pcdDataAvailableIn, &pcdDataLostIn, &pcdDataCorruptIn);
			if (pcdDataLostIn || pcdDataCorruptIn)
			{
				fLost = true_mex;
				mexPrintf("AnalogIn - pcdDataLostIn || pcdDataCorruptedIn - Error \n");
				mexPrintf("pcdDataLostIn    %d \n", pcdDataLostIn);
				mexPrintf("pcdDataCorruptIn %d \n", pcdDataCorruptIn);
				return 0;
			}
		}
		else if (cSamplesIn < nSamples)
		{
			// get samples
			// FDwfAnalogInStatusData( HDWF hdwf, int idxChannel, double *rgdVoltData, int cdData)
			// - hdwf � Interface handle.
			// - idxChannel � Channel index.
			// - rgdVoltData � Pointer to allocated buffer to copy the acquisition data.
			// - cdData � Number of samples to copy.
			// The function is used to retrieve the acquired data samples from the specified idxChannel on the AnalogIn
			// instrument. It copies the data samples to the provided buffer.
			FDwfAnalogInStatusData(hdwf, 0, &xr0[cSamplesIn], nSamples - cSamplesIn);
			FDwfAnalogInStatusData(hdwf, 1, &xr1[cSamplesIn], nSamples - cSamplesIn);
			cSamplesIn += nSamples - cSamplesIn;
		}

		if (cSamplesOut0 + cdDataFreeOut0 < nSamples)
		{
			// FDwfAnalogOutNodePlayData( HDWF hdwf, int idxChannel, AnalogOutNode node, double *rgdData, int cdData)
			//- hdwf � Open interface handle on a device.
			//- idxChannel � Channel index.
			//- node � Node index.
			//- rgdData � Pointer to samples array to be sent to the device.
			//- cdData � Number of samples to send.
			// The function is used to sending new data samples for play mode. Before starting the Analog Out
			// instrument, prefill the device buffer with the first set of samples using the AnalogOutNodeDataSet function.
			// In the loop of sending the following samples, first call AnalogOutStatus to read the information from the device,
			// then AnalogOutPlayStatus to find out how many new samples can be sent, then send the samples with AnalogOutPlayData.
			FDwfAnalogOutNodePlayData(hdwf, 0, AnalogOutNodeCarrier, &xt0[cSamplesOut0], cdDataFreeOut0);
			cSamplesOut0 += cdDataFreeOut0;

			if (!FDwfAnalogOutStatus(hdwf, 0, &sts))
			{
				FDwfGetLastErrorMsg(szError);
				mexPrintf("AnalogOutStatus 0 - Error\n\t%s", szError);
				mexPrintf("AnalogOutStatus 0 sts0 = %d\n", sts);
				return 0;
			}
			// mexPrintf("AnalogOutStatus 0 sts0 = %d\n", sts);

			// FDwfAnalogOutNodePlayStatus(HDWF hdwf, int idxChannel, AnalogOutNode node, 	int *cdDataFree, int *cdDataLost, int *cdDataCorrupted)
			// - hdwf � Open interface handle on a device.
			// - idxChannel � Channel index.
			// - node � Node index.
			// - cdDataFree � Pointer to variable to return the available free buffer space, the number of new samples that can be sent.
			// - cdDataLost � Pointer to variable to return the number of lost samples.
			// - cdDataCorrupted � Pointer to variable to return the number of samples that could be corrupted.
			// The function above is used to retrieve information about the play process. The data lost occurs when the device
			// generator is faster than the sample send process from the PC.In this case, the device buffer gets emptied and
			// generated samples are repeated.Corrupt samples are a warning that the buffer might have been emptied while
			// samples were sent to the device.In this case, try optimizing the loop for faster execution; or reduce the frequency
			// or run time to be less or equal to the device buffer size(run time <= buffer size / frequency).
			FDwfAnalogOutNodePlayStatus(hdwf, 0, AnalogOutNodeCarrier, &cdDataFreeOut0, &cdDataLostOut0, &cdDataCorruptedOut0);
			if (cdDataLostOut0 || cdDataCorruptedOut0)
			{
				fLost = true_mex;
				mexPrintf("AnalogOut 0 -- cdDataLostOut0 || cdDataCorruptedOut0 - Error \n");
				mexPrintf("cdDataLostOut0      %d \n", cdDataLostOut0);
				mexPrintf("cdDataCorruptedOut0 %d \n", cdDataCorruptedOut0);
				return 0;
			}
		}
		else if (cSamplesOut0 < nSamples)
		{
			// FDwfAnalogOutNodePlayData( HDWF hdwf, int idxChannel, AnalogOutNode node, double *rgdData, int cdData)
			//- hdwf � Open interface handle on a device.
			//- idxChannel � Channel index.
			//- node � Node index.
			//- rgdData � Pointer to samples array to be sent to the device.
			//- cdData � Number of samples to send.
			// The function is used to sending new data samples for play mode. Before starting the Analog Out
			// instrument, prefill the device buffer with the first set of samples using the AnalogOutNodeDataSet function.
			// In the loop of sending the following samples, first call AnalogOutStatus to read the information from the device,
			// then AnalogOutPlayStatus to find out how many new samples can be sent, then send the samples with AnalogOutPlayData.
			FDwfAnalogOutNodePlayData(hdwf, 0, AnalogOutNodeCarrier, &xt0[cSamplesOut0], nSamples - cSamplesOut0);
			cSamplesOut0 += nSamples - cSamplesOut0;
		}

		if (cSamplesOut1 + cdDataFreeOut1 < nSamples)
		{
			// FDwfAnalogOutNodePlayData( HDWF hdwf, int idxChannel, AnalogOutNode node, double *rgdData, int cdData)
			//- hdwf � Open interface handle on a device.
			//- idxChannel � Channel index.
			//- node � Node index.
			//- rgdData � Pointer to samples array to be sent to the device.
			//- cdData � Number of samples to send.
			// The function is used to sending new data samples for play mode. Before starting the Analog Out
			// instrument, prefill the device buffer with the first set of samples using the AnalogOutNodeDataSet function.
			// In the loop of sending the following samples, first call AnalogOutStatus to read the information from the device,
			// then AnalogOutPlayStatus to find out how many new samples can be sent, then send the samples with AnalogOutPlayData.
			FDwfAnalogOutNodePlayData(hdwf, 1, AnalogOutNodeCarrier, &xt1[cSamplesOut1], cdDataFreeOut1);
			cSamplesOut1 += cdDataFreeOut1;

			if (!FDwfAnalogOutStatus(hdwf, 1, &sts))
			{
				FDwfGetLastErrorMsg(szError);
				mexPrintf("AnalogOutStatus 1 - Error\n\t%s", szError);
				mexPrintf("AnalogOutStatus 1 sts1 = %d\n", sts);
				return 0;
			}
			// mexPrintf("AnalogOutStatus 1 sts1 = %d\n", sts);

			// FDwfAnalogOutNodePlayStatus(HDWF hdwf, int idxChannel, AnalogOutNode node, 	int *cdDataFree, int *cdDataLost, int *cdDataCorrupted)
			// - hdwf � Open interface handle on a device.
			// - idxChannel � Channel index.
			// - node � Node index.
			// - cdDataFree � Pointer to variable to return the available free buffer space, the number of new samples that can be sent.
			// - cdDataLost � Pointer to variable to return the number of lost samples.
			// - cdDataCorrupted � Pointer to variable to return the number of samples that could be corrupted.
			// The function above is used to retrieve information about the play process. The data lost occurs when the device
			// generator is faster than the sample send process from the PC.In this case, the device buffer gets emptied and
			// generated samples are repeated.Corrupt samples are a warning that the buffer might have been emptied while
			// samples were sent to the device.In this case, try optimizing the loop for faster execution; or reduce the frequency
			// or run time to be less or equal to the device buffer size(run time <= buffer size / frequency).
			FDwfAnalogOutNodePlayStatus(hdwf, 1, AnalogOutNodeCarrier, &cdDataFreeOut1, &cdDataLostOut1, &cdDataCorruptedOut1);
			if (cdDataLostOut1 || cdDataCorruptedOut1)
			{
				fLost = true_mex;
				mexPrintf("AnalogOut 1 -- cdDataLostOut1 || cdDataCorruptedOut1 - Error \n");
				mexPrintf("cdDataLostOut1      %d \n", cdDataLostOut1);
				mexPrintf("cdDataCorruptedOut1 %d \n", cdDataCorruptedOut1);
				return 0;
			}
		}
		else if (cSamplesOut1 < nSamples)
		{
			// FDwfAnalogOutNodePlayData( HDWF hdwf, int idxChannel, AnalogOutNode node, double *rgdData, int cdData)
			//- hdwf � Open interface handle on a device.
			//- idxChannel � Channel index.
			//- node � Node index.
			//- rgdData � Pointer to samples array to be sent to the device.
			//- cdData � Number of samples to send.
			// The function is used to sending new data samples for play mode. Before starting the Analog Out
			// instrument, prefill the device buffer with the first set of samples using the AnalogOutNodeDataSet function.
			// In the loop of sending the following samples, first call AnalogOutStatus to read the information from the device,
			// then AnalogOutPlayStatus to find out how many new samples can be sent, then send the samples with AnalogOutPlayData.
			FDwfAnalogOutNodePlayData(hdwf, 1, AnalogOutNodeCarrier, &xt1[cSamplesOut1], nSamples - cSamplesOut1);
			cSamplesOut1 += nSamples - cSamplesOut1;
		}
	}

	if (fLost)
	{
		mexPrintf("Samples were lost or could be corrupted! Reduce frequency \n");
		mexPrintf("pcdDataAvailableIn= %d  pcdDataLostIn= %d  pcdDataCorruptIn= %d   cSamplesIn= %d  \n", pcdDataAvailableIn, pcdDataLostIn, pcdDataCorruptIn, cSamplesIn);
		mexPrintf("cdDataFreeOut0= %d  cdDataLostOut0= %d  cdDataCorruptedOut0= %d   cSamplesOut0= %d  \n", cdDataFreeOut0, cdDataLostOut0, cdDataCorruptedOut0, cSamplesOut0);
		mexPrintf("cdDataFreeOut1= %d  cdDataLostOut1= %d  cdDataCorruptedOut1= %d   cSamplesOut1= %d  \n", cdDataFreeOut1, cdDataLostOut1, cdDataCorruptedOut1, cSamplesOut1);
	}
	else
	{
		printf("Samples could be well!\n");
	}

	// stop signal acquisition
	FDwfAnalogInConfigure(hdwf, false_mex, false_mex);

	// stop signal generation
	FDwfAnalogOutConfigure(hdwf, -1, false_mex);

	return 0;
}

EXPORTED_FUNCTION HDWF OpenDigilent(void)
{
	HDWF hdwf;
	char szError[512];

	// open automatically the first available device
	if (!FDwfDeviceOpen(-1, &hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		mexPrintf("Device open failed\n\t%s", szError);
		return -1;
	}
	mexPrintf("Device open \n");
	return hdwf;
}

EXPORTED_FUNCTION int CloseDigilent(HDWF hdwf)
{
	char szError[512];

	if (!FDwfDeviceClose(hdwf))
	{
		FDwfGetLastErrorMsg(szError);
		mexPrintf("DeviceCloseError: %s\n", szError);
		return -1;
	}
	else
	{
		mexPrintf("Device close \n");
		return 1;
	}
}

/* this function exists so that mex may be used to compile the library
   it is not otherwise needed */
void mexFunction(int nlhs, mxArray *plhs[],
				 int nrhs, const mxArray *prhs[])
{
}

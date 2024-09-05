#ifndef constants_h
#define constants_h

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <ctime>
// #include <direct.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <variant>
#include <vector>

// ROOT dependencies
#include "TAttFill.h"
#include "TAttLine.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "THStack.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TLine.h"
#include "TList.h"
#include "TNamed.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TText.h"
#include "TTree.h"

// To write 2D Vectors to root file
#ifdef __MAKECINT__
#pragma link C++ class std::vector < std::vector<double> >+;
#endif 

struct sample
{
	double voltage; // [mV]
	double time;	// [ns]
};

struct points
{
	double value;
	std::vector<int> index;
};

struct gaussParams
{
	double mean;
	double sigma;
};

struct plot2D
{
	TH2D *histo;
	std::string name;
};

struct plot1D
{
	TH1D *histo;
	std::string name;
};

class dataHeader
{
public:
	int timebase;
	std::string activeChannels;
	int numActive;
	std::string activeTriggers;
	float auxTriggerThreshold;
	std::vector<float> chTriggerThreshold;
	std::vector<int> chVRanges;
	std::vector<int> chSamples;
	int preTriggerSamples;
	int numWaveforms;
	int timestamp;
	std::string modelNumber;
	std::string serialNumber;
};

struct dataCollectionParameters
{
	std::vector<std::string> biasFullVec;
	std::vector<std::string> ledFullVec;
	std::vector<std::string> biasShortVec;
	std::vector<std::string> ledShortVec;
};

const std::vector<int>
	ps6000VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
const bool showHeader(false);
const bool doMovingAverage(false);
const bool plotFirstWaveforms(false); // TODO: Implement this

// XXX: To remove
const std::vector<std::string> MPPCsHV{/*"78V", "78.5V", "79V", "79.5V","80V", "80.5V", */ "81V" /*, "81.5V", "82V", "82.5V", "83V"*/};
const std::vector<std::string> LEDV{"Dark", "805mV", "810mV", "820mV", "840mV", "870mV", "890mV"};
const std::vector<std::string> PMTV{"1.2kV" /*, "1.25kV", "1.3kV", "1.35kV", "1.4kV", "1.45kV", "1.5kV"*/};
const std::vector<std::string> picoscopeNames{"IW098-0028", "IW114-0004"};
// XXX: end

// const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "82", "82.5", "83"};
// const std::vector<std::string> ledFullVec{"820", "840", "850", "880", "890", "900"};
// const std::vector<std::string> biasShortVec{"80.5", "81", "81.5"};
// const std::vector<std::string> ledShortVec{"840", "880", "900"};
const std::string g_pmt("1.4kV");

// TEMP
const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "80.5", "81", "81.5", "82", "82.5", "83"};
const std::vector<std::string> ledFullVec{};
const std::vector<std::string> biasShortVec{};
const std::vector<std::string> ledShortVec{"805", "810", "820", "840", "870", "890"};
// TEMP

const dataCollectionParameters g_dcp{biasFullVec, ledFullVec, biasShortVec, ledShortVec};

// Analysis parameters
const uint32_t g_baselineLowerWindow = 0;
const uint32_t g_baselineUpperWindow = 50;
const uint32_t g_integratedLowerWindow = 160;
const uint32_t g_integratedUpperWindow = 210;

// ROOT file branches variables
TTree *tree;
int channel = 0;
double pulseMinimum = 0;
int pulseMinimumSampleNumber = 0;
double pulseMinimumTime = 0;
double pulseMinimumMovingAverage = 0;
double integratedCharge = 0;

// Titles and naming should be modified if adding an histo
const std::vector<std::vector<std::string>> titles{{"First 100 waveforms for ch", "waveforms"},
												   {"Pulse time for ch", "pulseTime"},
												   {"Pulse minimal value for ch", "pulseMinimum"},
												   {"Integrated charge for ch", "integratedCharge"}};

#endif // constants_h

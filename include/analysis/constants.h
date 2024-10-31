#ifndef constants_h
#define constants_h

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <chrono>
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
#include "TApplication.h"
#include "TColor.h"
#include "TFile.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TH1.h"
#include "TH2.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "TF1.h"
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
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TVectorD.h"
#include "TROOT.h"
#include "Math/MinimizerOptions.h"
#include "TError.h"

struct sample
{
	double voltage; // [mV]
	double time;	// [ns]
	
	bool operator<(const sample &s) const 
	{
        return voltage < s.voltage;
    }
	bool operator>(const sample &s) const 
	{
        return voltage > s.voltage;
    }
};

struct environmentSample // Temperature, pressure and humiditu sample
{
	int32_t timestamp; // Saved as JST unix timestamp
	double temperature;
	double humidity;
	double pressure;
	bool operator<(const environmentSample &s) const 
	{
        return timestamp < s.timestamp;
    }
	bool operator>(const environmentSample &s) const 
	{
        return timestamp > s.timestamp;
    }
	bool operator==(const environmentSample &s) const
	{
		return timestamp    == s.timestamp
			&& temperature  == s.temperature
			&& humidity     == s.humidity
			&& pressure     == s.pressure;
	}
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
	uint8_t timebase;
	std::string activeChannels;
	uint8_t numActive;
	std::string activeTriggers;
	float auxTriggerThreshold;
	std::vector<float> chTriggerThreshold;
	std::vector<uint8_t> chVRanges;
	std::vector<uint16_t> chSamples;
	int16_t preTriggerSamples;
	uint32_t numWaveforms;
	int32_t timestamp;
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

struct wfNumbers // this being necessary brings me anger - Alex
{
	int darkWfs;
	int ledWfs;
};

struct pmtResult
{
	double amplitude;  double uAmplitude;
	double mean;       double uMean;
	double sigma;      double uSigma;
	double chi2;
	// placeholder for the moment
};

struct individualPeResult
{
	double amplitude;             double uAmplitude;
	double poissLambda;           double uPoissLambda;
	double noisePeakMean;         double uNoisePeakMean;
	double pePeakSigma;    	      double uPePeakSigma;
	double pePeakSeparation;      double uPePeakSeparation;
	double pePeakSeparationSigma; double uPePeakSeparationSigma;
	int nPeaks; double chi2;
};

struct highPeResult
{
	double amplitude;  double uAmplitude;
	double mean;       double uMean;
	double sigma;      double uSigma;
	double chi2;
};

struct darkResult
{
	double amplitude;  double uAmplitude;
	double mean;       double uMean;
	double sigma;      double uSigma;
	double chi2;
	// placeholder for the moment
};

struct fileResults
{
	std::vector<std::string> mppcNumbers;
	dataCollectionParameters dcp;
	std::vector<std::vector<int32_t>> timestamps;
	std::vector<std::vector<std::vector<highPeResult>>> gaussFits;
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits;
	std::vector<std::vector<std::vector<darkResult>>> darkFits;
};

const double g_2pi = 2 * 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899L;

const int64_t factorialLookUp[21] = {
	1,
	1,
	2,
	6,
	24,
	120,
	720,
	5040,
	40320,
	362880,
	3628800,
	39916800,
	479001600,
	6227020800,
	87178291200,
	1307674368000,
	20922789888000,
	355687428096000,
	6402373705728000,
	121645100408832000,
	2432902008176640000
};

///////////////////////////////////////////////////////////////////////////////
///                          Extraction Parameters                          ///
///////////////////////////////////////////////////////////////////////////////

const std::vector<int>
	ps6000VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
const bool showHeader(false);
const bool positiveSignals = false;
const int g_jstOffset = 32400; // 9 hours in seconds
// const int g_localOffset = -; // - hours in seconds
// TCanvas *g_c = new TCanvas("g_c", "g_c", 100, 100, 500, 500);

///////////////////////////////////////////////////////////////////////////////
///                            Common Parameters                            ///
///////////////////////////////////////////////////////////////////////////////

// XXX: To remove
const std::vector<std::string> MPPCsHV{/*"78V", "78.5V", "79V", "79.5V","80V", "80.5V", */ "81V" /*, "81.5V", "82V", "82.5V", "83V"*/};
const std::vector<std::string> LEDV{"Dark", "805mV", "810mV", "820mV", "840mV", "870mV", "890mV"};
const std::vector<std::string> PMTV{"1.2kV" /*, "1.25kV", "1.3kV", "1.35kV", "1.4kV", "1.45kV", "1.5kV"*/};
// XXX: end

const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "82", "82.5", "83"};
const std::vector<std::string> ledFullVec{"820", "840", "850", "880", "890", "900"};
const std::vector<std::string> biasShortVec{"80.5", "81", "81.5"};
const std::vector<std::string> ledShortVec{"840", "880", "900"};
const std::string g_pmt("1.4kV");
const std::vector<std::string> picoscopeNames{"IW098-0028", "IW114-0004"};

// TEMP
// const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "80.5", "81", "81.5", "82", "82.5", "83"};
// const std::vector<std::string> ledFullVec{};
// const std::vector<std::string> biasShortVec{};
// const std::vector<std::string> ledShortVec{"805", "810", "820", "840", "870", "890"};
// TEMP

const dataCollectionParameters g_dcp{biasFullVec, ledFullVec, biasShortVec, ledShortVec};

///////////////////////////////////////////////////////////////////////////////
///                         Pre-Analysis Parameters                         ///
///////////////////////////////////////////////////////////////////////////////

const uint32_t g_baselineLowerWindow = 0;
const uint32_t g_baselineUpperWindow = 100; // so fitting is a bit more reliable
const uint32_t g_integratedLowerWindow = 160;
const uint32_t g_integratedUpperWindow = 210;

// const bool doMovingAverage(false);
const bool plotFirstWaveforms(false); // TODO: Implement this??

// XXX: TO REMOVE
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

// XXX: END

///////////////////////////////////////////////////////////////////////////////
///                           Analysis Parameters                           ///
///////////////////////////////////////////////////////////////////////////////

// const int g_nPeaks = 3; // number of PE peaks to attempt to fit
// XXX: g_saveAllPlots unimplemented so far
const bool g_saveAllPlots = true; // save all produced hists to chonky pdf, recommend leave off unless debugging
const int g_highPeCutoff = 845; // LED values below are treated as individual PE,  above is high PE
const int g_nBins = 500;

TColor *col = gROOT->GetColor(10);

const std::string g_tmpPdf = "/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpbase.pdf";
int g_plots = 0;
int g_maxPeaks = 0;

#endif // constants_h

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
#include <numeric>

// ROOT dependencies
#include "TAttFill.h"
#include "TAttLine.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TColor.h"
#include "TFile.h"
#include "TGraph.h"
#include "TGraphErrors.h"
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
#include "TMath.h"
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

struct dataHeader
{
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
	double location;   double uLocation;
	double scale;      double uScale;
	double alpha;      double uAlpha;
	double mean;       double uMean;
	double chi2;
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

const double g_pi = 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899L;
const double g_2pi = 2 * g_pi;
const double g_sqrt2_pi = sqrt(2 / g_pi);

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

const std::vector<int> VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
const bool showHeader(false);
const bool positiveSignals = false;
const int g_jstOffset = 32400; // 9 hours in seconds
// const int g_localOffset = -; // - hours in seconds
// TCanvas *g_c = new TCanvas("g_c", "g_c", 100, 100, 500, 500);

///////////////////////////////////////////////////////////////////////////////
///                            Common Parameters                            ///
///////////////////////////////////////////////////////////////////////////////

const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "82", "82.5", "83"};
// const std::vector<std::string> ledFullVec{"820", "840", "850", "880", "890", "900"};
const std::vector<std::string> biasShortVec{"80.5", "81", "81.5"};
// const std::vector<std::string> ledShortVec{"840", "880", "900"};
const std::string g_pmt("1.4kV");
const std::vector<std::string> picoscopeNames{"IW098-0028", "IW114-0004"};

// TEMP
// const std::vector<std::string> biasFullVec{"78", "78.5", "79", "79.5", "80", "80.5", "81", "81.5", "82", "82.5", "83"};
// const std::vector<std::string> ledFullVec{};
// const std::vector<std::string> biasShortVec{};
// const std::vector<std::string> ledShortVec{"805", "810", "820", "840", "870", "890"};

std::vector<std::string> ledShortVec;
std::vector<std::string> ledFullVec;
// TEMP

dataCollectionParameters g_dcp{biasFullVec, ledFullVec, biasShortVec, ledShortVec};

///////////////////////////////////////////////////////////////////////////////
///                         Pre-Analysis Parameters                         ///
///////////////////////////////////////////////////////////////////////////////

const uint32_t g_baselineLowerWindow = 0;
const uint32_t g_baselineUpperWindow = 100; // so fitting is a bit more reliable
const uint32_t g_integratedLowerWindow = 160;
const uint32_t g_integratedUpperWindow = 275;

bool g_quickPreAnalysis = false;
const uint32_t g_quickBaselineLowerWindow = 0;
const uint32_t g_quickBaselineUpperWindow = 20;

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
const int g_highPeCutoff = 545; // LED values equal or below are treated as individual PE,  above is high PE
const int g_nBins = 250;
const std::string g_pePlotLedV = "540";

const int g_threads = 4;
TColor *col = gROOT->GetColor(10);

const double g_ampGain = 25.56; // 10^2.815
const double g_splitter = 0.668; // 10^(0.3 insertion loss + 0.05 extra loss for given freq)
const double g_terminalCap = 500e-12; // terminal capacitance // might be 900???
const double g_elementaryCharge = 1.602176634e-19;
const double g_pixelsPerCh = 3531;

int g_plots = 0;
int g_maxPeaks = 0;

#endif // constants_h

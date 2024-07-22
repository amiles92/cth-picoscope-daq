#ifndef constants_h
#define constants_h

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <ctime>
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
#include "TEllipse.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "THn.h"
#include "THStack.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TLine.h"
#include "TList.h"
#include "TNamed.h"
#include "TRandom3.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TText.h"
#include "TTree.h"


struct sample{
	double voltage; // [mV]
	double time; // [ns]
};

struct points{
	double value;
	std::vector<int> index;
};

struct gaussParams{
	double mean;
	double sigma;
};

const std::vector<int> ps6000VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
const bool showHeader(false);
const bool doMovingAverage(false);
const std::vector<std::string> MPPCsHV{"78V", "78.5V", "79V", "79.5V","80V", "80.5V", "81V", "81.5V", "82V", "82.5V", "83V"};
const std::vector<std::string> LEDV{"Dark", "805mV", "810mV", "820mV", "830mV", "840mV", "850mV", "860mV", "870mV", "880mV", "890mV", "900mV", "905mV", "920mV"};
const std::vector<std::string> picoscopeNames{"IW098-0028", "IW114-0004"};

#endif //constants_h

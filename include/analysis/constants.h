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

std::vector<int> ps6000VRanges{10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
bool showHeader(false);
bool doMovingAverage(false);

#endif //constants_h

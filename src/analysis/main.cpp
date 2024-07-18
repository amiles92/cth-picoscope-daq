#include "constants.h"

bool isExisting(const std::string& name){
    return (access(name.c_str(), F_OK) != -1);
}

std::vector<std::string> stringComponents(const std::string& name){
	std::string stringToDecompose(name), delimiter("_");
    std::vector<std::string> components;
    size_t pos(0);
    while ((pos = stringToDecompose.find(delimiter)) != std::string::npos) {
        std::string stringPart(stringToDecompose.substr(0, pos));
        components.push_back(stringPart);
        stringToDecompose.erase(0, pos + delimiter.length());
    }
	return components;
}

  ///////////////////////////////////////////////////////////////////////////////
 ///                          Extraction functions                           ///
///////////////////////////////////////////////////////////////////////////////

std::string byteBin(const char byte){
    return std::bitset<8>(static_cast<unsigned char>(byte)).to_string();
}

std::string byteHex(const char byte){
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(byte));
    return ss.str();
}

std::string bytesBin(std::ifstream& f, const int n){
    std::string s;
    for(int i0(0); i0<n; ++i0){
        char byte;
        f.read(&byte, 1);
        s += byteBin(byte);
    }
    return s;
}

int bytesInt(std::ifstream& f, const int n){
    std::string s;
    for(int i0(0); i0<n; ++i0){
        char byte;
        f.read(&byte, 1);
        s += byteHex(byte);
    }
    return std::stoi(s, nullptr, 16);
}

std::string bytesHex(std::ifstream& f, const int n){
    std::string s;
    for(int i0(0); i0<n; ++i0){
        char byte;
        f.read(&byte, 1);
        s += byteHex(byte);
    }
    return s;
}

int bytesTwos(std::ifstream& f, const int n) {
    std::string hexStr(bytesHex(f, n));
    int value(std::stoi(hexStr, nullptr, 16));
    if(value & (1 << (n * 8 - 1))) {
        value -= 1 << (n * 8);
    }
    return value;
}

std::string bytesString(std::ifstream& f, const int n = 0) {
    char c;
    std::string s;
    f.read(&c, 1);
    while(c != '\0'){
        s += c;
        f.read(&c, 1);
    }
    return s;
}

float adc2mv(const int value, const int range) {
    return (value / 32512.0f) * ps6000VRanges[range];
}

std::unordered_map<std::string, std::variant<int, float, std::string>> readHeader(std::ifstream& f){
    std::unordered_map<std::string, std::variant<int, float, std::string>> d;
    int nCh (4);
    char b;
    f.read(&b, 1);
    d["timebase"] = b >> 4;
    d["activeChannels"] = byteBin(b).substr(4);
    d["activeTriggers"] = byteBin(f.get()).substr(3);
    d["auxTriggerThreshold"] = adc2mv(bytesTwos(f, 2), 6);
    for (int i0(0); i0<nCh; ++i0) {
        d["ch" + std::string(1, 'A'+i0) + "TriggerThreshold"] = adc2mv(bytesTwos(f, 2), 6);
    }
    std::string vRanges = bytesBin(f, 2);
    for (int i0(0); i0<nCh; ++i0) {
        d["ch" + std::string(1, 'A'+i0) + "VRange"] = std::stoi(vRanges.substr(4*i0, 4), nullptr, 2);
    }
    for (int i0(0); i0<nCh; ++i0) {
        d["ch" + std::string(1, 'A'+i0) + "Samples"] = bytesInt(f, 2);
    }
    d["preTriggerSamples"] = bytesInt(f, 2);
    d["numWaveforms"] = bytesInt(f, 4);
    d["timestamp"] = bytesTwos(f, 4);
    d["modelNumber"] = bytesString(f);
    d["serialNumber"] = bytesString(f);
    return d;
}

void printHeader(const std::unordered_map<std::string, std::variant<int, float, std::string>>& header) {
    std::cout << "Header Information:\n";
    for(const auto& [key, value] : header){
        std::cout << key << ": ";
        if(std::holds_alternative<int>(value)){
            std::cout << std::get<int>(value);
        }else if (std::holds_alternative<float>(value)){
            std::cout << std::fixed << std::setprecision(2) << std::get<float>(value);
        }else if (std::holds_alternative<std::string>(value)){
            std::cout << std::get<std::string>(value);
        }
        std::cout << std::endl;
    }
}

std::vector<std::vector<std::vector<sample>>> readData(std::ifstream& f, std::unordered_map<std::string, std::variant<int, float, std::string>>& d){
    std::vector<std::vector<std::vector<sample>>> data;
    int nWf = std::get<int>(d["numWaveforms"]);
    double timeBase = pow(2, std::get<int>(d["timebase"])) * 0.2;
    for(int ch(0); ch<4; ++ch){
        if(std::get<std::string>(d["activeChannels"])[ch] == '0'){
            continue;
        }
        int nSamples = std::get<int>(d["ch" + std::string(1, 'A'+ch) + "Samples"]);
        std::vector<int> chADCData(nWf * nSamples);
        f.read(reinterpret_cast<char*>(chADCData.data()), nWf*nSamples*sizeof(int));
        std::vector<std::vector<sample>> chData;
        for (int i0(0); i0<nWf; ++i0) {
        	std::vector<sample> wfChData;
        	for(int i1(0); i1<nSamples; ++i1){
        		sample newSample{adc2mv(chADCData[i0*nSamples+i1], std::get<int>(d["ch" + std::string(1, 'A'+ch) + "VRange"])),timeBase*i1};
            	wfChData.push_back(newSample);
            }
            chData.push_back(wfChData);
        }
        data.push_back(chData);
    }
    return data;
}

int getNumSamples(std::unordered_map<std::string, std::variant<int, float, std::string>>& d){
	int numSamples(std::get<int>(d["chASamples"]));
	for(int ch(0); ch<4; ++ch){
		if(std::get<int>(d["ch" + std::string(1, 'A'+ch) + "Samples"]) != numSamples && std::get<int>(d["ch" + std::string(1, 'A'+ch) + "Samples"]) != 0){
			std::cout << "WARNING: all channels do not have the same number of samples, default value of '1' retuned...";
			return 1;
		}
	}
	return numSamples;
}

double getTimeBase(std::unordered_map<std::string, std::variant<int, float, std::string>>& d){
	double timebase(pow(2, std::get<int>(d["timebase"])) * 0.2);
	return timebase;
}

  ///////////////////////////////////////////////////////////////////////////////
 ///                         Pre-analysis functions                          ///
///////////////////////////////////////////////////////////////////////////////

points getMinData(const std::vector<sample>& data){
    std::vector<sample> sortedData(data);
    std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b){return a.voltage < b.voltage;});
    double minimum = sortedData[0].voltage;
    sortedData.clear();
    std::vector<int> minimumIndexes;
    for(int i0(0); i0<data.size(); ++i0){
    	if(data[i0].voltage == minimum){
    		minimumIndexes.push_back(i0);
    	}
    }
    points result{minimum,minimumIndexes};
    return result;
}

points getMaxData(const std::vector<sample>& data){
    std::vector<sample> sortedData(data);
    std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b){return a.voltage > b.voltage;});
    double maximum = sortedData[0].voltage;
    sortedData.clear();
    std::vector<int> maximumIndexes;
    for(int i0(0); i0<data.size(); ++i0){
    	if(data[i0].voltage == maximum){
    		maximumIndexes.push_back(i0);
    	}
    }
    points result{maximum,maximumIndexes};
    return result;
}

void plottingFirstWaveforms(const std::vector<std::vector<std::vector<sample>>>& data, std::vector<THn*> plots, 
                            const int nBinsT, const int nBinsV, const int nSamples, const double timeBase){
    int nBins[2] = {nBinsT, nBinsV};
    double lower[2] = {0, 0};
    double upper[2] = {nSamples*timeBase, 0};
    int numWf(100);
    for(int i0(0); i0<data.size(); ++i0){
		if(numWf>data[i0].size()){
			numWf=data[i0].size();
		}
	}
	for(int i0(0); i0<data.size(); ++i0){
		for(int i1(0); i1<numWf; ++i1){
			points minimum(getMinData(data[i0][i1]));
			points maximum(getMaxData(data[i0][i1]));
			if(minimum.value < lower[1]){
				lower[1] = minimum.value;
			}
			if(maximum.value > upper[1]){
				upper[1] = maximum.value;
			}
		}
	}
    
	for(int i0(0); i0<data.size(); ++i0){
		TString ch(std::string(1, 'A'+i0)), nWf(std::to_string(numWf));
		THnD* h = new THnD("ch"+ch+"_"+nWf+"Wf", ";time [ns];charge [mV]",2,  nBins, lower, upper);
		for(int i1(0); i1<numWf; ++i1){
			for(int i2(0); i2<data[i0][i1].size(); ++i2){
				h->Fill(data[i0][i1][i2].time, data[i0][i1][i2].voltage);
			}
		}
		plots.push_back(h);
	}
}

void plottingMinimum(const std::vector<std::vector<std::vector<sample>>>& data, std::vector<THn*> plots, 
                     const int nBinsMinimumTime, const int nBinsPulseMinimum, const int nSamples, const double timeBase){
	int nBinsTime[1] = {nBinsMinimumTime};
	double lowerTime[1] = {0};
	double upperTime[1] = {nSamples * timeBase};
	int nBinsMini[1] = {nBinsPulseMinimum};
	double lowerMini[1] = {0};
	double upperMini[1] = {0};
	std::vector<std::vector<points>> matrixMinimum;
	for(int i0(0); i0<data.size(); ++i0){
		std::vector<points> vecMinimum;
		for(int i1(0); i1<data[i0].size(); ++i1){
			points minimum(getMinData(data[i0][i1]));
			points maximum(getMaxData(data[i0][i1]));
			if(minimum.value < lowerMini[0]){
				lowerMini[0] = minimum.value;
			}
			if(maximum.value > upperMini[0]){
				upperMini[0] = maximum.value;
			}
			vecMinimum.push_back(minimum);
		}
		matrixMinimum.push_back(vecMinimum);
	}
	std::vector<THn*> plotsMini;
	std::vector<THn*> plotsTime;
	for(int i0(0); i0<matrixMinimum.size(); ++i0){
		TString ch(std::string(1, 'A'+i0));
		THnD* h = new THnD("ch"+ch+"_pulseMinimum", ";charge [mV]", 1, nBinsMini, lowerMini, upperMini);
		THnD* h1 = new THnD("ch"+ch+"_peakTiming", ";time [ns]", 1, nBinsTime, lowerTime, upperTime);
		for(int i1(0); i1<matrixMinimum[i0].size(); ++i1){
			h->Fill(matrixMinimum[i0][i1].value);
			for(int i2(0); i2<matrixMinimum[i0][i1].index.size(); ++i2){
				h1->Fill(matrixMinimum[i0][i1].index[i2] * timeBase);
			}
		}
		plotsMini.push_back(h);
		plotsTime.push_back(h1);
	}
	for(int i0(0); i0<plotsMini.size(); ++i0){
		plots.push_back(plotsMini[i0]);
	}
	for(int i0(0); i0<plotsTime.size(); ++i0){
		plots.push_back(plotsTime[i0]);
	}
	matrixMinimum.clear();
}

std::vector<sample> movingAverage(const std::vector<sample>& data, const int windowLowerEdge, const int windowUpperEdge, const int nHalfAverage){
	std::vector<sample> newData;
	int lowerEdge, upperEdge;
	if(windowLowerEdge - nHalfAverage < 0 && windowUpperEdge + nHalfAverage > data.size() - 1){
		lowerEdge = nHalfAverage;
		upperEdge = data.size() - 1 - nHalfAverage;
	}else if(windowLowerEdge - nHalfAverage < 0){
		lowerEdge = nHalfAverage;
		upperEdge = windowUpperEdge;
	}else if(windowUpperEdge + nHalfAverage > data.size() - 1){
		lowerEdge = windowLowerEdge;
		upperEdge = data.size() - 1 - nHalfAverage;
	}else{
		lowerEdge = windowLowerEdge;
		upperEdge = windowUpperEdge;
	}
	for(int i0(lowerEdge); i0 <= upperEdge; ++i0){
		newData.push_back(data[i0]);
		for(int i1(1); i1 <= nHalfAverage; ++i1){
			newData[i0].voltage += newData[i0-i1].voltage + newData[i0+i1].voltage;
		}
		newData[i0].voltage /= 2 * nHalfAverage + 1;
	}
	return newData;
}

//TODO: Do we really need it???
void plottingMinimumMovingAverage(const std::vector<std::vector<std::vector<sample>>>& data, std::vector<THn*> plots, 
                             const int nBins, const int windowLowerEdge, const int windowUpperEdge, const int nHalfAverage,
                             const int nSamples, const double timeBase){
}

gaussParams baseLine(const std::vector<sample>& data, const int windowLowerEdge, const int windowUpperEdge){
	double x(0), x2(0);
	for(int i0(windowLowerEdge); i0 <= windowUpperEdge; ++i0){
		x += data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
		x2 += data[i0].voltage * data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
	}
	gaussParams parameters{x, sqrt(((windowUpperEdge - windowLowerEdge + 1) / (windowUpperEdge - windowLowerEdge)) * (x2 - x * x))}; 
	return parameters;
}

double chargeIntegration(const std::vector<sample>& data, const int windowLowerEdge, 
                         const int windowUpperEdge, const double timeBase, const double baseLineValue){
	double integratedCharge(0);
	for(int i0(windowLowerEdge); i0 <= windowUpperEdge; ++i0){
		integratedCharge += data[i0].voltage * timeBase;
	}
	return integratedCharge;
}

void plottingIntegratedCharge(const std::vector<std::vector<std::vector<sample>>>& data, std::vector<THn*> plots, const int nBinsIntegrated, 
                              const int windowLowerEdge, const int windowUpperEdge, const double timeBase){
	int nBins[1] = {nBinsIntegrated};
	double lower[1] = {0}, upper[1] = {0};
	std::vector<std::vector<double>> matrixIntegratedCharge;
	for(int i0(0); i0<data.size(); ++i0){
		std::vector<double> vecIntegratedCharge;
		for(int i1(0); i1<data[i0].size(); ++i1){
			double baseLineValue(0);
			double inteQ(chargeIntegration(data[i0][i1], windowLowerEdge, windowUpperEdge, timeBase, baseLineValue));
			vecIntegratedCharge.push_back(inteQ);
			if(inteQ < lower[0]){
				lower[0] = inteQ;
			}
			if(inteQ > upper[0]){
				upper[0] = inteQ;
			}
		}
		matrixIntegratedCharge.push_back(vecIntegratedCharge);
	}
	for(int i0(0); i0<matrixIntegratedCharge.size(); ++i0){
		TString ch(std::string(1, 'A'+i0));
		THnD* h = new THnD("ch"+ch+"_integratedCharge", ";integrated charge [V.ns]", 1, nBins, lower, upper);
		for(int i1(0); i1<matrixIntegratedCharge[i0].size(); ++i1){
			h->Fill(matrixIntegratedCharge[i0][i1]);
		}
		plots.push_back(h);
	}
	matrixIntegratedCharge.clear();
}

  ///////////////////////////////////////////////////////////////////////////////
 ///                            Saving functions                             ///
///////////////////////////////////////////////////////////////////////////////

Double_t maxValueTH2D(TH2D* histo){
	Double_t max_value=0;
	Int_t x_bins=histo->GetNbinsX();
	Int_t y_bins=histo->GetNbinsY();
	for(Int_t i=0;i<x_bins;++i){
		for(Int_t j=0;j<y_bins;++j){
			Double_t test_value=histo->GetBinContent(i+1,j+1);
			if(test_value>max_value){
				max_value=test_value;
			}
		}
	}
	return max_value;
}

Double_t minValueTH2D(TH2D* histo){
	Double_t min_value=maxValueTH2D(histo);
	Int_t x_bins=histo->GetNbinsX();
	Int_t y_bins=histo->GetNbinsY();
	for(Int_t i=0;i<x_bins;++i){
		for (Int_t j=0;j<y_bins;++j){
			Double_t test_value=histo->GetBinContent(i+1,j+1);
			if (test_value<min_value && test_value>0){
				min_value=test_value;
			}
		}
	}
	return min_value;
}


void setStyle(TCanvas* canvas){
	canvas->SetTicks();
	canvas->SetTopMargin(0.05f);
	canvas->SetRightMargin(0.15f);
	canvas->SetLeftMargin(0.15f);
}

void addTitle(TCanvas* canvas, const TString title){
	canvas->cd();
	TLatex Tl;
	Tl.SetNDC();
	Tl.SetTextColor(kBlack);
	Tl.SetTextAlign(31);
	Tl.SetTextSize(0.03f);
	Tl.DrawLatex(0.85,0.96,title);
}


TCanvas* drawCanvas(TString canvasName, TH1D* histo, const TString &title){
	TCanvas* canvas=new TCanvas(canvasName,canvasName,1000,1000);
	canvas->cd();
	setStyle(canvas);
	addTitle(canvas,title);
	return canvas;
}

TCanvas* drawColorCanvas(TString canvasName, TH2D* histo, const TString &title, const Double_t max_value, const Double_t min_value){
	TCanvas* canvas=new TCanvas(canvasName,canvasName,1238,1000);
	canvas->cd();
	setStyle(canvas);
	histo->SetMaximum(max_value);
	histo->SetMinimum(min_value);
	histo->Draw("colz");
	gPad->Update();
	addTitle(canvas,title);
	
	return canvas;
}

void saveInPDF(const TString directory, const TString pdf_name, const TString png_name, TCanvas* canvas){
	TCanvas* C = canvas;
	C->SaveAs(directory+pdf_name);
	C->Delete();
}


/*void savingPlots(const std::vector<THn*>& plots){
	TString outputDirectory("/home/chadeau/Documents/codes/MPPC_QC/cth-picoscope-daq/");
	std::vector<TString> pdfFiles{"100wf", "pulseMin", "pulseMinTime", "inteQ"};
	for(int i0(0); i0<plots.size(); ++i0){
		TString ch(std::string(1, 'A' + i0%4));
		if(i0<4){
			TString pdfFile(pdfFiles[0]+"_ch"+ch+".pdf");
			saveInPDF(outputDirectory,pdfFile,plots[i0]->GetName(),
		           	  drawColorCanvas(plots[i0]->GetName(), plots[i0], 
		          	  "100 first waveforms for ch "+ch, 
		          	  maxValueTH2D(plots[i0]), minValueTH2D(plots[i0])));
		}else if(i0>=4 && i0<8){
			TString pdfFile(pdfFiles[1]+"_ch"+ch+".pdf");
			saveInPDF(outputDirectory,pdfFile,plots[i0]->GetName(),
						  drawCanvas(plots[i0]->GetName(),plots[i0],
						  " for ch "+ch));
		}else if(i0>=8 && i0<12){
			TString pdfFile(pdfFiles[2]+"_ch"+ch+".pdf");
			saveInPDF(outputDirectory,pdfFile,plots[i0]->GetName(),
						  drawCanvas(plots[i0]->GetName(),plots[i0],
						  " for ch "+ch));
		}else if(i0>=12 && i0<16){
			TString pdfFile(pdfFiles[3]+"_ch"+ch+".pdf");
			saveInPDF(outputDirectory,pdfFile,plots[i0]->GetName(),
						  drawCanvas(plots[i0]->GetName(),plots[i0],
						  " for ch "+ch));
		}
	}
}*/

  ///////////////////////////////////////////////////////////////////////////////
 ///                              Main function                              ///
///////////////////////////////////////////////////////////////////////////////

int main(int argc , char** argv){
	if (argc != 3){
		std::cerr<<"ERROR: you should have 2 parameters..."<<std::endl;
		return 1;
	}
	std::string analysisType(argv[1]);
	std::string fileName(argv[2]);
	std::ifstream fileList{fileName};
	if(isExisting(fileName) != true){
		std::cerr<<"ERROR: the file '" + fileName + "' does not exist..."<<std::endl;
		return 1;
	}
	for(std::string fileToProcess; std::getline(fileList,fileToProcess);){	
		if(isExisting(fileToProcess) != true){
			std::cerr<<"ERROR: the file '" + fileToProcess + "' does not exist..."<<std::endl;
			return 1;
		}else{
			std::ifstream file(fileToProcess, std::ios::binary);
    		if (!file.is_open()) {
        		std::cerr << "Error opening file." << std::endl;
        		return 1;
    		}
    		clock_t startTime, endTime;
    		startTime = clock();
    		std::cout<<"\n######### Starting pre-analysis: "<<fileToProcess<<std::endl;
    		std::cout<<"###### Starting extraction..."<<std::endl;
    		auto header = readHeader(file);
    		if(showHeader == true){
    			printHeader(header);
    		}
    		auto data = readData(file, header);
    		file.close();
    		std::cout<<"###### Finished extraction..."<<std::endl;
    		std::cout<<"###### Starting plotting..."<<std::endl;
    		std::vector<THn*> plots;
    		int nSamples(getNumSamples(header));
    		double timeBase(getTimeBase(header));
    		int nBinsT(nSamples), nBinsV(100), nBinsMinimumTime(nSamples), nBinsPulseMinimum(300), nBinsIntegrated(300), nHalfAverage(10), windowLowerEdge(100), windowUpperEdge(130);
    		plottingFirstWaveforms(data, plots, nBinsT, nBinsV, nSamples, timeBase);
    		std::cout<<"### Plotted first waveforms..."<<std::endl;
    		plottingMinimum(data, plots, nBinsMinimumTime, nBinsPulseMinimum, nSamples, timeBase);
    		std::cout<<"### Plotted pulse minimum and time..."<<std::endl;
    		if(doMovingAverage == true){
    			plottingMinimumMovingAverage(data, plots, nBinsPulseMinimum, windowLowerEdge, windowUpperEdge, nHalfAverage, nSamples, timeBase);
    			std::cout<<"### Plotted moving averaged minimum peaks value..."<<std::endl;
    		}
    		plottingIntegratedCharge(data, plots, nBinsIntegrated, windowLowerEdge, windowUpperEdge, timeBase);
    		std::cout<<"### Plotted integrated charge..."<<std::endl;
    		std::cout<<"###### Finished Plotting..."<<std::endl;
    		std::cout<<"###### Starting saving..."<<std::endl;
    		//savingPlots(plots);
    		std::cout<<"###### Finished saving..."<<std::endl;
    		endTime = clock();
    		std::cout<<"###### Finished pre-analysis: "<< (float) (endTime - startTime)/CLOCKS_PER_SEC <<"s"<<std::endl;
		}
	}
	std::cout<<std::endl;
	return 0;
}

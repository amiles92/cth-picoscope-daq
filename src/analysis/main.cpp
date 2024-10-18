#include "constants.h"

///////////////////////////////////////////////////////////////////////////////
///                            General functions                            ///
///////////////////////////////////////////////////////////////////////////////

bool isExisting(const std::string &name)
{
	return (access(name.c_str(), F_OK) != -1);
}

std::vector<std::string> stringComponents(const std::string &name, const char delim)
{
	std::stringstream ss(name);
	std::string tmp;
	std::vector<std::string> outVec;
	while (std::getline(ss, tmp, delim))
	{
		outVec.push_back(tmp);
	}
	return outVec;
}

std::string combineComponents(const std::string delim, const std::vector<std::string> strings)
{
	if (strings.size() == 0)
		return "";

	std::string outString;
	std::stringstream ss;
	ss << strings.at(0);
	for (uint i0 = 1 ; i0 < strings.size() ; i0++)
	{
		ss << delim;
		ss << strings.at(i0);
	}
	ss >> outString;
	return outString;
}

///////////////////////////////////////////////////////////////////////////////
///                          Extraction functions                           ///
///////////////////////////////////////////////////////////////////////////////

std::string byteBin(const char byte)
{
	return std::bitset<8>(static_cast<unsigned char>(byte)).to_string();
}

std::string byteHex(const char byte)
{
	std::stringstream ss;
	ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(byte));
	return ss.str();
}

std::string bytesBin(std::ifstream &f,
					 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteBin(byte);
	}
	return s;
}

int bytesInt(std::ifstream &f,
			 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteHex(byte);
	}
	return std::stoi(s, nullptr, 16);
}

std::string bytesHex(std::ifstream &f,
					 const int n)
{
	std::string s;
	for (int i0(0); i0 < n; ++i0)
	{
		char byte;
		f.read(&byte, 1);
		s += byteHex(byte);
	}
	return s;
}

int64_t bytesTwos(std::ifstream &f,
			  	  const int n)
{
	std::string hexStr(bytesHex(f, n));
	int64_t value(std::stoi(hexStr, nullptr, 16));
	if (value & (1 << (n * 8 - 1)))
	{
		value -= 1 << (n * 8);
	}
	return value;
}

std::string bytesString(std::ifstream &f,
						const int n = 0)
{
	char c;
	std::string s;
	f.read(&c, 1);
	while (c != '\0')
	{
		s += c;
		f.read(&c, 1);
	}
	return s;
}

float adc2mv(const int16_t value, const int range)
{
	return (value / 32512.0f) * ps6000VRanges[range];
}

dataHeader readHeader(std::ifstream &f)
{
	dataHeader d;
	int nCh(4);
	char b;
	int numActive(0);
	f.read(&b, 1);
	d.timebase = b >> 4;
	d.activeChannels = byteBin(b).substr(4);

	for (char c : d.activeChannels)
		if (c == '1') 
			numActive++;
	d.numActive = numActive;

	d.activeTriggers = byteBin(f.get()).substr(3);
	d.auxTriggerThreshold = adc2mv(bytesTwos(f, 2), 6);
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chTriggerThreshold.push_back(adc2mv(bytesTwos(f, 2), 6));
	}
	std::string vRanges = bytesBin(f, 2);
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chVRanges.push_back(std::stoi(vRanges.substr(4 * i0, 4), nullptr, 2));
	}
	for (int i0(0); i0 < nCh; ++i0)
	{
		d.chSamples.push_back(bytesInt(f, 2));
	}
	d.preTriggerSamples = bytesInt(f, 2);
	d.numWaveforms = bytesInt(f, 4);
	d.timestamp = bytesTwos(f, 4);
	d.modelNumber = bytesString(f);
	d.serialNumber = bytesString(f);
	return d;
}

void printHeader(const dataHeader &header)
{
	int nCh(4);

	std::cout << "Header Information:\n";
	std::cout << "timebase:             " << header.timebase << std::endl;
	std::cout << "activeChannels:       " << header.activeChannels << std::endl;
	std::cout << "activeTriggers:       " << header.activeTriggers << std::endl;
	std::cout << "auxTriggerThreshold:  " << header.auxTriggerThreshold << std::endl;
	std::cout << "preTriggerSamples:    " << header.preTriggerSamples << std::endl;
	std::cout << "numWaveforms:         " << header.numWaveforms << std::endl;
	std::cout << "timestamp:            " << header.timestamp << std::endl;
	std::cout << "modelNumber:          " << header.modelNumber << std::endl;
	std::cout << "serialNumber:         " << header.serialNumber << std::endl;
	for (int i0(0); i0 < nCh; ++i0)
	{
		std::cout << "\nChannel " << (char) ('A' + i0) << std::endl;
		std::cout << "chTriggerThreshold:   " << header.chTriggerThreshold.at(i0) << std::endl;
		std::cout << "chVRanges:            " << header.chVRanges.at(i0) << std::endl;
		std::cout << "chSamples:            " << header.chSamples.at(i0) << std::endl;
	}
	std::cout << "" << std::endl;
}

bool isLittleEndian()
{
	uint32_t i(1);
	char *c = (char *)&i;
	return bool(*c);
}

std::vector<std::vector<std::vector<sample>>> readData(std::ifstream &f, dataHeader &d)
{
	std::vector<std::vector<std::vector<sample>>> data;
	bool little(isLittleEndian());
	int nWf = d.numWaveforms;
	double timeBase = pow(2, d.timebase) * 0.2;
	for (int ch(0); ch < 4; ++ch)
	{
		if (d.activeChannels[ch] == '0')
		{
			continue;
		}
		int nSamples = d.chSamples.at(ch);
		std::vector<int16_t> chADCData(nWf * nSamples);
		int16_t *chADCDataPter = chADCData.data();
		if (little)
		{
			for (int i0(0); i0 < nSamples * nWf; ++i0)
			{
				int16_t tmp;
				f.read(reinterpret_cast<char *>(&tmp), sizeof(int16_t));
				*chADCDataPter = __builtin_bswap16(tmp);
				chADCDataPter++;
			}
		}
		else
		{
			f.read(reinterpret_cast<char *>(chADCData.data()), nWf * nSamples * sizeof(int));
		}
		std::vector<std::vector<sample>> chData(nWf);
		for (int i0(0); i0 < nWf; ++i0)
		{
			std::vector<sample> wfChData(nSamples);
			for (int i1(0); i1 < nSamples; ++i1)
			{
				sample newSample{adc2mv(chADCData[i0 * nSamples + i1], d.chVRanges.at(ch)) * (positiveSignals ? -1 : 1), timeBase * i1};
				wfChData.at(i1) = (newSample);
				// std::cout << newSample.time << " / " << newSample.voltage << std::endl;
			}
			chData.at(i0) = (wfChData);
		}
		data.push_back(chData);
	}
	return data;
}

int getNumSamples(dataHeader &d)
{
	int numSamples(d.chSamples.at(0));
	for (int ch(0); ch < 4; ++ch)
	{
		if (d.chSamples.at(ch) != numSamples && d.chSamples.at(ch) != 0)
		{
			std::cout << "WARNING: all channels do not have the same number of samples, default value of '1' retuned...";
			return 1;
		}
	}
	return numSamples;
}

double getTimebase(const dataHeader &d)
{
	double timebase(pow(2, d.timebase) * 0.2);
	return timebase;
}

environmentSample getSample(std::vector<environmentSample> &data, int32_t &timestamp)
{
	return *(std::min_element(data.begin(), data.end(), 
		[&](environmentSample x, environmentSample y)
	{
		return std::abs(x.timestamp - timestamp) < std::abs(y.timestamp - timestamp);
	}));
}

bool readEnvLine(const std::string line, environmentSample &s)
{
	std::vector<std::string> vecLineQuotes = stringComponents(line, ',');
	std::vector<std::string> vecLine;
	double secondsPerDay = 86400;
	int daysBetween1900And1970 = 25569;
	
	for (std::string line : vecLineQuotes)
	{
		vecLine.push_back(line.substr(1, line.size() - 2));
	}

	std::vector<int> expectedValues = {1, 2, 3, 4};
	for (int v : expectedValues) // NOTE: The last entry was incomplete, could be a problem
	{
		if (vecLine.at(v).empty()) 
			return false;
	}

	double timestampDays = std::stod(vecLine.at(1));
	int32_t timestamp = std::lround((timestampDays - daysBetween1900And1970) * secondsPerDay);

	double temperature = std::stod(vecLine.at(2));
	double humidity = std::stod(vecLine.at(3));
	double pressure = std::stod(vecLine.at(4));

	s = {timestamp, temperature, humidity, pressure};

	return true;
}

std::vector<environmentSample> readEnvData(const std::string filePath)
{
	std::ifstream file(filePath);
	std::string line;
	std::vector<environmentSample> envData;

	getline(file, line);
	std::cout << "title: " << line << std::endl;

	getline(file, line); // empty line
	getline(file, line);
	std::cout << "variables: " << line << std::endl;

	getline(file, line);
	if (line != "\"\",\"\",\"�C\",\"%RH\",\"hPa\",\"\",\"\",\"\",\"\",\"\"") // XXX: idk if this will work
	{
		std::cout << "Incorrect unit line!" << std::endl;
		std::cout << "file units: " << line << std::endl;
		std::cout << "hardcoded : \"\",\"\",\"�C\",\"%RH\",\"hPa\",\"\",\"\",\"\",\"\",\"\"" << std::endl;
	}

	while (getline(file, line))
	{
		environmentSample s;
		if (readEnvLine(line, s))
			envData.push_back(s);
	}

	file.close();

	return envData;
}

///////////////////////////////////////////////////////////////////////////////
///                         Pre-analysis functions                          ///
///////////////////////////////////////////////////////////////////////////////

points getMinData(const std::vector<sample> &data)
{
	std::vector<sample> sortedData(data);
	std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b)
			  { return a.voltage < b.voltage; });
	double minimum = sortedData[0].voltage;
	sortedData.clear();
	std::vector<int> minimumIndexes;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		if (data[i0].voltage == minimum)
		{
			minimumIndexes.push_back(i0);
		}
	}
	points result{minimum, minimumIndexes};
	return result;
}

points getMaxData(const std::vector<sample> &data)
{
	std::vector<sample> sortedData(data);
	std::sort(sortedData.begin(), sortedData.end(), [](sample a, sample b)
			  { return a.voltage > b.voltage; });
	double maximum = sortedData[0].voltage;
	sortedData.clear();
	std::vector<int> maximumIndexes;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		if (data[i0].voltage == maximum)
		{
			maximumIndexes.push_back(i0);
		}
	}
	points result{maximum, maximumIndexes};
	return result;
}

sample getMinDataSingle(const std::vector<sample> &data)
{
	return *(std::min_element(data.begin(), data.end()));
}

sample getMaxDataSingle(const std::vector<sample> &data)
{
	return *(std::max_element(data.begin(), data.end()));
}

void plottingFirstWaveforms(const std::string fileNameBase,
							const std::vector<std::vector<std::vector<sample>>> &data,
							std::vector<plot2D> &plots2D,
							const int nBinsT,
							const int nBinsV,
							const int nSamples,
							const double timeBase)
{
	int nBins[2] = {nBinsT, nBinsV};
	double lower[2] = {0, 0};
	double upper[2] = {nSamples * timeBase, 0};
	int numWf(100);
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		if (numWf > (int)data[i0].size())
		{
			numWf = data[i0].size();
		}
	}
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		for (int i1(0); i1 < numWf; ++i1)
		{
			points minimum(getMinData(data.at(i0).at(i1)));
			points maximum(getMaxData(data.at(i0).at(i1)));
			if (minimum.value < lower[1])
			{
				lower[1] = minimum.value;
			}
			if (maximum.value > upper[1])
			{
				upper[1] = maximum.value;
			}
		}
	}
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0)), nWf(std::to_string(numWf));
		TH2D *h = new TH2D(fileNameBase + "_" + nWf + "Wf_ch" + ch, ";time [ns];charge [mV]", nBins[0], lower[0], upper[0], nBins[1], lower[1], upper[1]);
		for (int i1(0); i1 < numWf; ++i1)
		{
			for (int i2(0); i2 < (int)data[i0][i1].size(); ++i2)
			{
				h->Fill(data[i0][i1][i2].time, data[i0][i1][i2].voltage);
			}
		}
		plot2D newPlot{h, "waveforms"};
		plots2D.push_back(newPlot);
	}
}

void plottingMinimum(const std::string fileNameBase,
					 const std::vector<std::vector<std::vector<sample>>> &data,
					 std::vector<plot1D> &plots1D,
					 std::vector<std::vector<double>> &pulseMinimumMatrix,
					 std::vector<std::vector<int>> &pulseMinimumSampleNumberMatrix,
					 std::vector<std::vector<double>> &pulseMinimumTimeMatrix,
					 const int nBinsMinimumTime,
					 const int nBinsPulseMinimum,
					 const int nSamples,
					 const double timeBase)
{
	int nBinsTime(nBinsMinimumTime);
	double lowerTime(0), upperTime(nSamples * timeBase);
	int nBinsMini(nBinsPulseMinimum);
	double lowerMini(0), upperMini(0);
	std::vector<std::vector<points>> matrixMinimum;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		std::vector<points> vecMinimum;
		for (int i1(0); i1 < (int)data[i0].size(); ++i1)
		{
			points minimum(getMinData(data[i0][i1]));
			points maximum(getMaxData(data[i0][i1]));
			if (minimum.value < lowerMini)
			{
				lowerMini = minimum.value;
			}
			if (maximum.value > upperMini)
			{
				upperMini = maximum.value;
			}
			vecMinimum.push_back(minimum);
		}
		matrixMinimum.push_back(vecMinimum);
	}
	std::vector<TH1D *> plotsMini;
	std::vector<TH1D *> plotsTime;
	for (int i0(0); i0 < (int)matrixMinimum.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0));
		TH1D *h = new TH1D(fileNameBase + "_pulseMinimum_ch" + ch, ";charge [mV]", nBinsMini, lowerMini, upperMini);
		TH1D *h1 = new TH1D(fileNameBase + "_pulseMinimumTime_ch" + ch, ";time [ns]", nBinsTime, lowerTime, upperTime);
		for (int i1(0); i1 < (int)matrixMinimum[i0].size(); ++i1)
		{
			h->Fill(matrixMinimum[i0][i1].value);
			for (int i2(0); i2 < (int)matrixMinimum[i0][i1].index.size(); ++i2)
			{
				h1->Fill(matrixMinimum[i0][i1].index[i2] * timeBase);
				pulseMinimumMatrix[i0].push_back(matrixMinimum[i0][i1].value);
				pulseMinimumSampleNumberMatrix[i0].push_back(matrixMinimum[i0][i1].index[i2]);
				pulseMinimumTimeMatrix[i0].push_back(matrixMinimum[i0][i1].index[i2] * timeBase);
			}
		}
		plotsMini.push_back(h);
		plotsTime.push_back(h1);
	}
	for (int i0(0); i0 < (int)plotsMini.size(); ++i0)
	{
		plot1D newPlot{plotsMini[i0], "pulseMinimum"};
		plots1D.push_back(newPlot);
	}
	for (int i0(0); i0 < (int)plotsTime.size(); ++i0)
	{
		plot1D newPlot{plotsTime[i0], "pulseTime"};
		plots1D.push_back(newPlot);
	}
	matrixMinimum.clear();
}

std::vector<sample> movingAverage(const std::vector<sample> &data,
								  const int windowLowerEdge,
								  const int windowUpperEdge,
								  const int nHalfAverage)
{
	std::vector<sample> newData;
	int lowerEdge, upperEdge;
	if (windowLowerEdge - nHalfAverage < 0 && windowUpperEdge + nHalfAverage > (int)data.size() - 1)
	{
		lowerEdge = nHalfAverage;
		upperEdge = data.size() - 1 - nHalfAverage;
	}
	else if (windowLowerEdge - nHalfAverage < 0)
	{
		lowerEdge = nHalfAverage;
		upperEdge = windowUpperEdge;
	}
	else if (windowUpperEdge + nHalfAverage > (int)data.size() - 1)
	{
		lowerEdge = windowLowerEdge;
		upperEdge = data.size() - 1 - nHalfAverage;
	}
	else
	{
		lowerEdge = windowLowerEdge;
		upperEdge = windowUpperEdge;
	}
	for (int i0(lowerEdge); i0 <= upperEdge; ++i0)
	{
		newData.push_back(data[i0]);
		for (int i1(1); i1 <= nHalfAverage; ++i1)
		{
			newData[i0].voltage += newData[i0 - i1].voltage + newData[i0 + i1].voltage;
		}
		newData[i0].voltage /= 2 * nHalfAverage + 1;
	}
	return newData;
}

gaussParams baseLine(const std::vector<sample> &data,
					 const int windowLowerEdge,
					 const int windowUpperEdge)
{
	double x(0), x2(0);
	for (int i0(windowLowerEdge); i0 <= windowUpperEdge; ++i0)
	{
		x += data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
		x2 += data[i0].voltage * data[i0].voltage / (windowUpperEdge - windowLowerEdge + 1);
	}
	gaussParams parameters{x, sqrt(((windowUpperEdge - windowLowerEdge + 1) / (windowUpperEdge - windowLowerEdge)) * (x2 - x * x))};
	return parameters;
}

double sigmoidExpDecay(double x, double ampl, double step, double lb)
{
	return ampl * exp(-lb * (x - step)) / (1 + exp(-(x - step)));
}

double singlePulse(double *x, double *p)
{
	return p[0] + sigmoidExpDecay(x[0], p[1], p[2], p[3]);//0.05);
}

double doublePulse(double *x, double *p)
{
	if (abs(p[2] - p[4]) < 3) return DBL_MAX;
	return p[0] + sigmoidExpDecay(x[0], p[1], p[2], p[5])//0.05) 
				+ sigmoidExpDecay(x[0], p[3], p[4], p[6]);//0.05);
}

double dc(double *x, double *p)
{
	return p[0];
}

gaussParams baseLineLandau(const std::vector<sample> &data,
						   const int windowLowerEdge,
						   const int windowUpperEdge)
{
	int nbins = windowUpperEdge - windowLowerEdge + 1;
	double xmin = windowLowerEdge - 0.5;
	double xmax = windowUpperEdge + 0.5;

	double minPulseHeight = -3;
	double minLb = 0.04;
	double maxLb = 0.08;

	const char* fitOptions = "SNMQ";

	TH1D* hist = new TH1D("hist", "baseline", nbins, xmin, xmax);
	
	TF1* fnSingle = new TF1("singlePulse", singlePulse, xmin, xmax, 5);
	fnSingle->SetParameter(0, 0);
	fnSingle->SetParameter(1, -5.);
	fnSingle->SetParLimits(1, -200, minPulseHeight);
	fnSingle->SetParameter(2, nbins / 2 - 5);
	fnSingle->SetParLimits(2, xmin - 10, xmax - 5);
	fnSingle->SetParameter(3, 0.05);
	fnSingle->SetParLimits(3, minLb, maxLb);

	TF1* fnDouble = new TF1("doublePulse", doublePulse, xmin, xmax, 7);
	fnDouble->SetParameter(0, 0);
	fnDouble->SetParameter(1, -5.);
	fnDouble->SetParLimits(1, -200, minPulseHeight);
	fnDouble->SetParameter(2, xmin + 0.1 * nbins);
	fnDouble->SetParLimits(2, xmin - 10, xmax - 5);
	fnDouble->SetParameter(3, -5.);
	fnDouble->SetParLimits(3, -200, minPulseHeight);
	fnDouble->SetParameter(4, xmax - 0.1 * nbins);
	fnDouble->SetParLimits(4, xmin - 10, xmax - 5);
	fnDouble->SetParameter(5, 0.05);
	fnDouble->SetParLimits(5, minLb, maxLb);
	fnDouble->SetParameter(6, 0.05);
	fnDouble->SetParLimits(6, minLb, maxLb);

	TF1* fnDc = new TF1("dc", dc, xmin, xmax, 1);
	fnDc->SetParameter(0,0);

	for (int i = windowLowerEdge ; i <= windowUpperEdge ; i++)
	{
		hist->SetBinContent(i, data.at(i).voltage);
	}

	TFitResultPtr fitSingle = hist->Fit(fnSingle, fitOptions);
	TFitResultPtr fitDc = hist->Fit(fnDc, fitOptions);

	if (fitDc->Chi2() < fitSingle->Chi2())// && fitDc->Chi2() < fitDouble->Chi2())
	{
		delete hist;
		delete fnSingle;
		delete fnDouble;
		delete fnDc;
		return gaussParams{fitDc->Parameter(0), fitDc->ParError(0)};
	}

	TFitResultPtr fitDouble = hist->Fit(fnDouble, fitOptions);
	delete hist;
	delete fnSingle;
	delete fnDouble;
	delete fnDc;
	if (fitSingle->Chi2() < fitDouble->Chi2())
	{
		return gaussParams{fitSingle->Parameter(0), fitSingle->ParError(0)};
	}

	return gaussParams{fitDouble->Parameter(0), fitDouble->ParError(0)};
}

double chargeIntegrationPeak(const std::vector<sample> &data,
						 	 const int sampleNumber,
						 	 const int windowLowerEdge,
						 	 const int windowUpperEdge,
						 	 const double timeBase,
						 	 const double baseLineValue)
{
	double integratedCharge(0);
	int lowerEdge(0), upperEdge(0);
	if (sampleNumber - windowLowerEdge < 0 && sampleNumber + windowUpperEdge > (int)data.size())
	{
		lowerEdge = 0;
		upperEdge = data.size() - 1;
	}
	else if (sampleNumber - windowLowerEdge < 0)
	{
		lowerEdge = 0;
		upperEdge = sampleNumber + windowUpperEdge;
	}
	else if (sampleNumber + windowUpperEdge > (int)data.size())
	{
		lowerEdge = sampleNumber - windowLowerEdge;
		upperEdge = data.size() - 1;
	}
	else
	{
		lowerEdge = sampleNumber - windowLowerEdge;
		upperEdge = sampleNumber + windowUpperEdge;
	}
	for (int i0(lowerEdge); i0 <= upperEdge; ++i0)
	{
		integratedCharge += (data[i0].voltage - baseLineValue) * timeBase;
	}
	return integratedCharge;
}

double chargeIntegrationFixed(const std::vector<sample> &data,
						 	  const double timebase,
						 	  const double baseline,
							  const uint32_t lowerWindow,
							  const uint32_t upperWindow)
{
	double integratedChargeRaw(0);
	int lowerEdge(std::max((uint32_t) 0,lowerWindow));
	int upperEdge(std::min((uint32_t) data.size() - 1,upperWindow));
	double totalBaseline(baseline * (upperEdge - lowerEdge));

	for (int i0(lowerEdge) ; i0 <= upperEdge ; ++i0)
	{
		integratedChargeRaw += data.at(i0).voltage;
	}

	double integratedCharge((integratedChargeRaw - totalBaseline) * timebase);

	return integratedCharge;
}

void getWaveformProperties(const std::vector<std::vector<sample>> &dataChannel,
						   Double_t* integratedChargeChannel,
						   Double_t* minimumTimeChannel,
						   Double_t* minimumVoltageChannel,
						   const double timebase,
						   const uint32_t lowerWindow,
						   const uint32_t upperWindow)
{
	for (uint i0(0) ; i0 < dataChannel.size() ; ++i0)
	{
		gaussParams baseLineValue = baseLineLandau(dataChannel.at(i0), g_baselineLowerWindow, g_baselineUpperWindow);
		Double_t charge = chargeIntegrationFixed(dataChannel.at(i0), timebase, baseLineValue.mean, lowerWindow, upperWindow);
		sample minSample = getMinDataSingle(dataChannel.at(i0));

		integratedChargeChannel[i0] = charge;
		minimumTimeChannel[i0] = minSample.time;
		minimumVoltageChannel[i0] = minSample.voltage;
		// XXX: Just getting the first one, maybe should change?
	}
}

void processLedPreAnalysis(const dataHeader &header,
							const std::vector<std::vector<std::vector<sample>>> &data,
							Double_t* outData,
							const uint32_t lowerWindow = g_integratedLowerWindow,
							const uint32_t upperWindow = g_integratedUpperWindow)
{
	for (int i0(0) ; i0 < (int) header.activeChannels.length() ; ++i0)
	{
		if (header.activeChannels.at(i0) == '0')
		{
			continue;
		}

		const int wfs = header.numWaveforms;
		Double_t *outDataCh = outData + i0 * 3 * wfs;

		std::cout << "###### Analysing Channel " << (char) ('A' + i0) << std::endl;

		getWaveformProperties(data.at(i0), outDataCh, outDataCh + wfs, outDataCh + 2 * wfs,
							  getTimebase(header), lowerWindow, upperWindow);
	}
}

void processDarkPreAnalysis(const dataHeader &header,
							const std::vector<std::vector<std::vector<sample>>> &data,
							Double_t* outData)
{
	double timebase = getTimebase(header);

	for (uint i0(0) ; i0 < header.activeChannels.length() ; ++i0)
	{
		if (header.activeChannels.at(i0) == '0')
		{
			continue;
		}

		uint32_t upperWindow = header.chSamples.at(i0);
		const int wfs = header.numWaveforms;

		Double_t *outDataCh = outData + i0 * wfs;
		std::vector<std::vector<sample>> dataChannel = data.at(i0);

		gaussParams baseLineValue(baseLine(dataChannel.at(i0), 0, dataChannel.size() - 1));

		std::cout << "###### Analysing Channel " << (char) ('A' + i0) << std::endl;

		for (uint i1(0) ; i1 < dataChannel.size() ; ++i1)
		{
			// Double_t charge = chargeIntegrationFixed(dataChannel.at(i1), timebase, baseLineValue.mean, 0, upperWindow);
			Double_t charge = chargeIntegrationFixed(dataChannel.at(i1), timebase, 0, 0, upperWindow);

			outDataCh[i1] = charge;
		}
	}
}

void plottingIntegratedCharge(const std::string fileNameBase,
							  const std::vector<std::vector<std::vector<sample>>> &data,
							  std::vector<std::vector<double>> &integratedChargeMatrix,
							  std::vector<plot1D> &plots1D,
							  const int nBinsIntegrated,
							  const int windowLowerEdge,
							  const int windowUpperEdge,
							  const double timeBase)
{
	int nBins(nBinsIntegrated);
	double lower(0), upper(0);
	std::vector<std::vector<double>> matrixInteQ;
	for (int i0(0); i0 < (int)data.size(); ++i0)
	{
		std::vector<double> vecInteQ;
		for (int i1(0); i1 < (int)data[i0].size(); ++i1)
		{
			gaussParams baseLineValue(baseLine(data[i0][i1], 0, 50));
			points minimums(getMinData(data[i0][i1]));
			for (int i2(0); i2 < (int)minimums.index.size(); ++i2)
			{
				double inteQ(chargeIntegrationPeak(data[i0][i1], minimums.index[i2], windowLowerEdge, windowUpperEdge, timeBase, baseLineValue.mean));
				if (inteQ < -1000000)
				{
					std::cout << "ERROR: index " << minimums.index[i2] << "/ pulse value " << data[i0][i1][minimums.index[i2]].voltage << " / lower and upper edge " << windowLowerEdge << " " << windowUpperEdge << " / baseline " << baseLineValue.mean << " mV / integrated charge " << inteQ << " mV" << std::endl;
					inteQ = 0;
				}
				integratedChargeMatrix[i0].push_back(inteQ);
				vecInteQ.push_back(inteQ);
				if (inteQ < lower)
				{
					lower = inteQ;
				}
				if (inteQ > upper)
				{
					upper = inteQ;
				}
			}
		}
		matrixInteQ.push_back(vecInteQ);
	}
	for (int i0(0); i0 < (int)matrixInteQ.size(); ++i0)
	{
		TString ch(std::string(1, 'A' + i0));
		TH1D *h = new TH1D(fileNameBase + "_integratedCharge_ch" + ch, ";integrated charge [mV.ns]", nBins, lower, upper);
		for (int i1(0); i1 < (int)matrixInteQ[i0].size(); ++i1)
		{
			h->Fill(matrixInteQ[i0][i1]);
		}
		plot1D newPlot{h, "integratedCharge"};
		plots1D.push_back(newPlot);
	}
	matrixInteQ.clear();
}

///////////////////////////////////////////////////////////////////////////////
///                           Analysis functions                            ///
///////////////////////////////////////////////////////////////////////////////

double square(const double x)
{
	return x * x;
}

int64_t factorial(const int64_t n) // up to 20
{
	return factorialLookUp[n];
}

double factorialD(const int n) // up to 170
{
	return (n < 21) ? factorialLookUp[n] : factorial(n - 1) * n;
}

double gaussDistribution(double x, double mu, double sigma)
{
	return exp(-0.5 * square(x - mu) / square(sigma)) / (sigma * sqrt(g_2pi));
}

std::vector<double> initialGuesses(TH1D* hist)
{
	std::vector<double> outVec(0, 4); // 0th peak, 1st peak, sigma?
	return outVec;
}

double poissGaussDistribution(double x, double ampl, double lambda, double mu0, 
		double sigmaPe, double mu, double sigma, int nPeaks, double ampl0, double sigma0)
{
	// lambda is Poissonian distribution parameter
	// q0 is mean of Gaussian for 0th PE peak
	// sigma0 is std of Gaussian for each PE peak
	// mu and sigma are mean and std of separation between PE peaks
	double total = 0;
	if (nPeaks > 20)
	{
		std::cout << "NOTE: n! loses int64 precision at this level (n > 20)" << std::endl;
	}
	total += ampl0 * gaussDistribution(x, mu0, sigma0);
	for (int i = 1 ; i < nPeaks ; i++) // Fit 1st peak on
	{
		if (i < 21) // factorial loses int64 precision at this level
		{
			total += ampl * (pow(lambda, i) * exp(-lambda) / factorial(i) ) * 
			gaussDistribution(x, i * mu + mu0, sqrt(square(sigmaPe) + square(i*sigma)));
		}
		else if (i < 171) // factorial exceeds double exponent range at this level
		{
			total += ampl * (pow(lambda, i) * exp(-lambda) / factorialD(i) ) * 
			gaussDistribution(x, i * mu + mu0, sqrt(square(sigmaPe) + square(i*sigma)));
		}
		else
		{
			std::cout << "WARNING: i out of exponent range for factorialD" << std::endl;
			break;
		}
	}
	return total;
}

double pgdPtr(double *x, double *p) // pointer version of the Poiss-Gauss distribution function
{
	return poissGaussDistribution(x[0], p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8]);
}

individualPeResult individualPeAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins)
{
	TCanvas *c = new TCanvas("c", "c");
	TH1D* hist = new TH1D("hist", "Individual Photoelectron Signal", nbins, xmin, xmax);


	TF1* fn = new TF1("poissGaus", pgdPtr, xmin, xmax, 9);
	// fn->SetParNames("Amplitude", "\\lambda", "\\mu_{0}", "\\sigma_{PE}", "\\mu",
	// 		"\\sigma", "n", "Amplitude_{0}", "\\sigma_{0}");
	fn->SetParNames("Amplitude_Signal", "\\lambda", "Mean_Noise", "\\sigma_Signal", 
			"Peak Separation", "\\sigma_nth Peak", "n", "Amplitude_Noise", "\\sigma_Noise");

	for (int i = 0 ; i < nWfs ; i++)
	{
		hist->Fill(data[i]);
	}

	hist->Sumw2();

	fn->SetNpx(2 * nbins);
	individualPeResult bestRes;
	double bestChi2 = DBL_MAX;
	int bestIndex = 0;
	TH1D* bestHist;

	for (int i = 2 ; i < 10 ; i++)
	{
		fn->SetParameter(0, nWfs);
		fn->SetParameter(1, 3);
		fn->SetParameter(2, 0);
		fn->SetParLimits(2, -20, 15);
		fn->SetParameter(3, 1);
		fn->SetParLimits(3, 0.1, 25);
		fn->SetParameter(4, -50);
		fn->SetParameter(5, 0.5);
		fn->SetParLimits(5, 0.1, 8);
		fn->FixParameter(6, i);
		fn->SetParameter(7, 500);
		fn->SetParLimits(7, 50, nWfs);
		fn->SetParameter(8, 2);
		fn->SetParLimits(8, 0.1, 12);
		for (int j = 0 ; j < 9 ; j++) fn->SetParError(j, 0);

		TFitResultPtr res = hist->Fit(fn, "SWMQ");
		double tmpChi2 = res->Chi2();
		if (tmpChi2 < bestChi2)
		{
			bestRes = {res->Parameter(0), res->ParError(0),
				   	   res->Parameter(1), res->ParError(1),
				   	   res->Parameter(2), res->ParError(2),
				   	   res->Parameter(3), res->ParError(3),
				   	   res->Parameter(4), res->ParError(4),
				   	   res->Parameter(5), res->ParError(5), 
					   i};
			bestChi2 = tmpChi2;
			bestHist = (TH1D*) hist->Clone();
			bestIndex = i;
			g_maxPeaks = std::max(g_maxPeaks, i);
		}
		if (i - bestIndex > 5) {break;} // break if it is 5 attempts past the last improvement?
	}
	bestHist->Sumw2(kFALSE);
	bestHist->GetXaxis()->SetTitle("Integrated charge [mV ns]");
	bestHist->GetYaxis()->SetTitle("Counts");
	bestHist->Draw();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPoiss.pdf");

	delete hist;
	delete bestHist;
	delete c;
	delete fn;

	return bestRes;
}

highPeResult highPeAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins)
{
	TCanvas *c = new TCanvas("c", "c");
	TH1D* hist = new TH1D("hist", "High Photoelectron Signal", nbins, xmin, xmax);

	for (int i = 0 ; i < nWfs ; i++)
	{
		hist->Fill(data[i]);
	}

	hist->Sumw2();

	TFitResultPtr res = hist->Fit("gaus","SQ");
	// "S" returns resultptr, "Q" is to minimise printing at the end
	// XXX: Consider adding option "L", log likelihood method

	highPeResult a = {res->Parameter(0), res->ParError(0),
					  res->Parameter(1), res->ParError(1),
					  res->Parameter(2), res->ParError(2)};

	hist->Sumw2(kFALSE);
	hist->GetXaxis()->SetTitle("Integrated charge [mV ns]");
	hist->GetYaxis()->SetTitle("Counts");
	// hist->SetStats(0);
	hist->Draw();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpGauss.pdf");

	delete hist;
	delete c;

	return a;
}

///////////////////////////////////////////////////////////////////////////////
///                            Saving functions                             ///
///////////////////////////////////////////////////////////////////////////////

Double_t maxValueTH2D(TH2D *histo)
{
	Double_t max_value = 0;
	Int_t x_bins = histo->GetNbinsX();
	Int_t y_bins = histo->GetNbinsY();
	for (Int_t i = 0; i < x_bins; ++i)
	{
		for (Int_t j = 0; j < y_bins; ++j)
		{
			Double_t test_value = histo->GetBinContent(i + 1, j + 1);
			if (test_value > max_value)
			{
				max_value = test_value;
			}
		}
	}
	return max_value;
}

Double_t minValueTH2D(TH2D *histo)
{
	Double_t min_value = maxValueTH2D(histo);
	Int_t x_bins = histo->GetNbinsX();
	Int_t y_bins = histo->GetNbinsY();
	for (Int_t i = 0; i < x_bins; ++i)
	{
		for (Int_t j = 0; j < y_bins; ++j)
		{
			Double_t test_value = histo->GetBinContent(i + 1, j + 1);
			if (test_value < min_value && test_value > 0)
			{
				min_value = test_value;
			}
		}
	}
	return min_value;
}

void setStyle(TCanvas *canvas)
{
	canvas->SetTicks();
	canvas->SetTopMargin(0.05f);
	canvas->SetRightMargin(0.15f);
	canvas->SetLeftMargin(0.15f);
}

void addTitle(TCanvas *canvas,
			  const TString title)
{
	canvas->cd();
	TLatex Tl;
	Tl.SetNDC();
	Tl.SetTextColor(kBlack);
	Tl.SetTextAlign(31);
	Tl.SetTextSize(0.03f);
	Tl.DrawLatex(0.85, 0.96, title);
}

TCanvas *drawCanvas(TString canvasName,
					TH1D *histo,
					const TString &title)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1000, 1000);
	canvas->cd();
	setStyle(canvas);
	histo->Draw();
	addTitle(canvas, title);
	return canvas;
}

TCanvas *drawColorCanvas(TString canvasName,
						 TH2D *histo,
						 const TString &title,
						 const Double_t maxValue,
						 const Double_t minValue)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1238, 1000);
	canvas->cd();
	setStyle(canvas);
	histo->SetMaximum(maxValue);
	histo->SetMinimum(minValue);
	histo->Draw("colz");
	gPad->Update();
	addTitle(canvas, title);
	return canvas;
}

TCanvas *drawStackCanvas(TString canvasName,
						 THStack *histos,
						 const TString &title)
{
	TCanvas *canvas = new TCanvas(canvasName, canvasName, 1000, 1000);
	canvas->cd();
	setStyle(canvas);
	histos->Draw("nostack");
	addTitle(canvas, title);
	return canvas;
}

void saveInPDF(const TString directory,
			   const TString pdfName,
			   TCanvas *canvas,
			   const TString logScale)
{
	gStyle->SetOptStat(0);
	TCanvas *C = canvas;
	if ("logX" == logScale)
	{
		C->SetLogx();
	}
	else if ("logY" == logScale)
	{
		C->SetLogy();
	}
	else if ("logZ" == logScale)
	{
		C->SetLogz();
	}
	else if ("logXY" == logScale)
	{
		C->SetLogx();
		C->SetLogy();
	}
	else if ("logXZ" == logScale)
	{
		C->SetLogx();
		C->SetLogz();
	}
	else if ("logYZ" == logScale)
	{
		C->SetLogy();
		C->SetLogz();
	}
	else if ("logXYZ" == logScale)
	{
		C->SetLogx();
		C->SetLogy();
		C->SetLogz();
	}
	C->SaveAs(directory + pdfName);
	delete C;
}

void savingPlots(const std::vector<plot2D> &plots2D,
				 const std::vector<plot1D> &plots1D,
				 const std::string pdfFileNameBase,
				 const std::string outputDirectory)
{
	int numberPlots2DPerChannel(plots2D.size() / 4), numberPlots1DPerChannel(plots1D.size() / 4);
	for (int i0(0); i0 < 4; ++i0)
	{
		TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
		TString ch(std::string(1, 'A' + i0));
		TString pdfFile(pdfFileNameBase + "_ch" + ch + ".pdf");
		Ctmp->SaveAs(outputDirectory + pdfFile + "[");
		for (int i1(0); i1 < numberPlots2DPerChannel; ++i1)
		{
			for (int i2(0); i2 < (int)titles.size(); ++i2)
			{
				if (plots2D[i1 * 4 + i0].name == titles[i2][1])
				{
					saveInPDF(outputDirectory, pdfFile,
							  drawColorCanvas(plots2D[i1 * 4 + i0].histo->GetName(), plots2D[i1 * 4 + i0].histo,
											  titles[i2][0] + ch,
											  maxValueTH2D(plots2D[i1 * 4 + i0].histo), minValueTH2D(plots2D[i1 * 4 + i0].histo)),
							  "logNot");
				}
			}
		}
		for (int i1(0); i1 < numberPlots1DPerChannel; ++i1)
		{
			for (int i2(0); i2 < (int)titles.size(); ++i2)
			{
				if (plots1D[i1 * 4 + i0].name == titles[i2][1])
				{
					std::string histoName(plots1D[i1 * 4 + i0].histo->GetName());
					saveInPDF(outputDirectory, pdfFile,
							  drawCanvas(plots1D[i1 * 4 + i0].histo->GetName(), plots1D[i1 * 4 + i0].histo,
										 titles[i2][0] + ch),
							  "logNot");
				}
			}
		}
		Ctmp->SaveAs(outputDirectory + pdfFile + "]");
		delete Ctmp;
	}
}

void fillingAndSavingTree(const std::string fileNameBase,
						  const std::string outputDirectory,
						  const std::vector<std::vector<double>> &pulseMinimumMatrix,
						  const std::vector<std::vector<int>> &pulseMinimumSampleNumberMatrix,
						  const std::vector<std::vector<double>> &pulseMinimumTimeMatrix,
						  const std::vector<std::vector<double>> &pulseMinimumMovingAverageMatrix,
						  const std::vector<std::vector<double>> &integratedChargeMatrix)
{
	TFile *file = TFile::Open((TString)(outputDirectory + fileNameBase) + ".root", "RECREATE");
	tree = new TTree("tree", "Charateristics of MPPC waveforms");
	tree->Branch("channel", &channel, "channel/I");
	tree->Branch("pulseMinimum", &pulseMinimum, "pulseMinimum/D");
	tree->Branch("pulseMinimumSampleNumber", &pulseMinimumSampleNumber, "pulseMinimumSampleNumber/I");
	tree->Branch("pulseMinimumTime", &pulseMinimumTime, "pulseMinimumTime/D");
	// if (doMovingAverage == true)
	// {
	// 	tree->Branch("pulseMinimumMovingAverage", &pulseMinimumMovingAverage, "pulseMinimumMovingAverage/D");
	// }
	tree->Branch("integratedCharge", &integratedCharge, "integratedCharge/D");
	for (int i0(0); i0 < 4; ++i0)
	{
		int n0(pulseMinimumMatrix[i0].size());
		int n1(pulseMinimumSampleNumberMatrix[i0].size());
		int n2(pulseMinimumTimeMatrix[i0].size());
		// int n3(pulseMinimumMovingAverageMatrix[i0].size());
		int n4(integratedChargeMatrix[i0].size());
		// if (doMovingAverage == true)
		// {
		// 	if (n0 != n1 || n0 != n2 || n0 != n3 || n0 != n4)
		// 	{
		// 		std::cout << "ERROR: vectors should all have the same size..." << std::endl;
		// 		continue;
		// 	}
		// }
		// else
		// {
		if (n0 != n1 || n0 != n2 || n0 != n4)
		{
			std::cout << "ERROR: vectors should all have the same size (except for moving average)..." << std::endl;
			continue;
		}
		// }
		for (int i1(0); i1 < n0; ++i1)
		{
			channel = i0;
			pulseMinimum = pulseMinimumMatrix[i0][i1];
			pulseMinimumSampleNumber = pulseMinimumSampleNumberMatrix[i0][i1];
			pulseMinimumTime = pulseMinimumTimeMatrix[i0][i1];
			// if (doMovingAverage == true)
			// {
			// 	pulseMinimumMovingAverage = pulseMinimumMovingAverageMatrix[i0][i1];
			// }
			// else
			// {
			// pulseMinimumMovingAverage = 0;
			// }
			integratedCharge = integratedChargeMatrix[i0][i1];
			tree->Fill();
		}
	}
	tree->Write();
	file->Close();
}

///////////////////////////////////////////////////////////////////////////////
///                         Analysis mode functions                         ///
///////////////////////////////////////////////////////////////////////////////

int preAnalyseFile(std::string directory, std::string filePath)
{
	if (directory.back() != '/')
	{
		directory += "/";
	}

	std::string fileBaseExt = filePath.substr(filePath.find_last_of("/") + 1);
	std::string::size_type const p(fileBaseExt.find_last_of('.'));
	std::string fileBasename = fileBaseExt.substr(0, p);

	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "ERROR: can not open file." << std::endl;
		return 1;
	}
	clock_t startTime, endTime;
	std::cout << "\n######### Starting pre-analysis: " << filePath << std::endl;
	startTime = clock();
	std::cout << "###### Starting extraction..." << std::endl;
	dataHeader header = readHeader(file);
	if (showHeader == true)
	{
		printHeader(header);
	}
	std::vector<std::vector<std::vector<sample>>> data = readData(file, header);
	file.close();
	std::cout << "###### Finished extraction..." << std::endl;
	std::cout << "###### Starting plotting..." << std::endl;

	std::vector<plot2D> plots2D;
	std::vector<plot1D> plots1D;
	std::vector<double> emptyVecDouble(0);
	std::vector<int> emptyVecInt(0);
	std::vector<std::vector<double>> pulseMinimumMatrix(4, emptyVecDouble);
	std::vector<std::vector<int>> pulseMinimumSampleNumberMatrix(4, emptyVecInt);
	std::vector<std::vector<double>> pulseMinimumTimeMatrix(4, emptyVecDouble);
	std::vector<std::vector<double>> pulseMinimumMovingAverageMatrix(4, emptyVecDouble);
	std::vector<std::vector<double>> integratedChargeMatrix(4, emptyVecDouble);

	int nSamples(getNumSamples(header));
	double timeBase(getTimebase(header));
	int nBinsT(nSamples), nBinsV(50), nBinsMinimumTime(nSamples), nBinsPulseMinimum(200), nBinsIntegrated(200),  windowLowerEdge(10), windowUpperEdge(50);// nHalfAverage(10);

	plottingFirstWaveforms(fileBasename, data, plots2D, nBinsT, nBinsV, nSamples, timeBase);
	std::cout << "### Plotted first waveforms..." << std::endl;
	plottingMinimum(fileBasename, data, plots1D, pulseMinimumMatrix, pulseMinimumSampleNumberMatrix, pulseMinimumTimeMatrix, nBinsMinimumTime, nBinsPulseMinimum, nSamples, timeBase);
	std::cout << "### Plotted pulse minimum and time..." << std::endl;
	// if (doMovingAverage == true)
	// {
	// 	plottingMinimumMovingAverage(fileBasename, data, plots1D, pulseMinimumMovingAverageMatrix, nBinsPulseMinimum, windowLowerEdge, windowUpperEdge, nHalfAverage, nSamples, timeBase);
	// 	std::cout << "### Plotted moving averaged minimum peaks value..." << std::endl;
	// }
	plottingIntegratedCharge(fileBasename, data, integratedChargeMatrix, plots1D, nBinsIntegrated, windowLowerEdge, windowUpperEdge, timeBase);
	std::cout << "### Plotted integrated charge..." << std::endl;
	std::cout << "###### Finished plotting..." << std::endl;
	std::cout << "###### Starting saving plots..." << std::endl;
	std::string outputDirectory = directory + "plots/";
	savingPlots(plots2D, plots1D, fileBasename, outputDirectory);
	std::cout << "### Plots saved..." << std::endl;
	outputDirectory = directory + "root-files/";
	fillingAndSavingTree(fileBasename, outputDirectory, pulseMinimumMatrix, pulseMinimumSampleNumberMatrix, pulseMinimumTimeMatrix, pulseMinimumMovingAverageMatrix, integratedChargeMatrix);
	std::cout << "### ROOT file saved..." << std::endl;
	std::cout << "###### Finished saving..." << std::endl;
	endTime = clock();
	std::cout << "###### Finished pre-analysis: " << (float)(endTime - startTime) / CLOCKS_PER_SEC << "s\n"
			  << std::endl;
	return 0;
}

void batchPreAnalysis(std::string directory, std::vector<std::string> files)
{
	for (int i0(0); i0 < (int)files.size(); ++i0)
	{
		std::string filePath = files.at(i0);
		if (isExisting(filePath) != true)
		{
			std::cerr << "WARNING: the file '" + filePath + "' does not exist..." << std::endl;
			continue;
		}
		preAnalyseFile(directory, filePath);
	}
}

void darkPreAnalysis(std::string directory, std::string date, std::string mppcStr,
					 std::vector<TTree *> forest)
{
	for (const std::string &bias : g_dcp.biasFullVec)
	{
		for (const std::string &pico : picoscopeNames)
		{
			std::vector<std::string> fileVec{date, bias + "V", "Dark", g_pmt, mppcStr, pico};
			std::string fileBasename(combineComponents("_", fileVec));
			std::string filePath(directory + "/" + fileBasename + ".dat");

			std::cout << "### Next file: " << filePath << std::endl;

			std::ifstream file(filePath, std::ios::binary);
			if (!file.is_open())
			{
				std::cerr << "ERROR: can not open file." << std::endl;
				continue;
			}

			std::cout << "###### Starting extraction..." << std::endl;
			dataHeader header = readHeader(file);
			if (showHeader == true)
			{
				printHeader(header);
			}
			std::vector<std::vector<std::vector<sample>>> data = readData(file, header);
			file.close();
			
			int wfs = (const int) header.numWaveforms;
			const int elems = 4 * wfs;

			Double_t *outData = (Double_t*) calloc(elems, sizeof(Double_t));

			std::cout << "###### Starting analysis..." << std::endl;

			processDarkPreAnalysis(header, data, outData);

			for (int i0(0) ; i0 < 4 ; ++i0)
			{
				if (header.activeChannels.at(i0) == '0')
				{
					continue;
				}

				std::string branchNameCharge = combineComponents("_", {bias, "Dark", pico, "Charge"});
				char *leaflist;

				asprintf(&leaflist, "data[%i]/D", wfs);

				// TODO: FINISH THIS
				TBranch *b = forest.at(i0)->Branch(branchNameCharge.c_str(), 
							 outData + i0 * wfs, (const char *) leaflist);
				b->Fill();
			}
			free(outData);

			std::cout << "###### Written branches to trees\n" << std::endl;
		}
	}
}

void ledPreAnalysisFile(std::string filePath, std::string bias,
	std::string led, std::string pico, std::vector<TTree *> forest)
{
	std::cout << "### Next file: " << filePath << std::endl;

	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "ERROR: can not open file." << std::endl;
		throw;
	}

	std::cout << "###### Starting extraction..." << std::endl;
	dataHeader header = readHeader(file);
	if (showHeader == true)
	{
		printHeader(header);
	}
	std::vector<std::vector<std::vector<sample>>> data = readData(file, header);
	file.close();

	const int wfs = (const int) header.numWaveforms;
	const int elems = 4 * 3 * wfs;

	Double_t *outData = (Double_t*) calloc(elems, sizeof(Double_t));

	std::cout << "###### Starting analysis..." << std::endl;

	processLedPreAnalysis(header, data, outData);

	for (int i0(0) ; i0 < 4 ; ++i0)
	{
		if (header.activeChannels.at(i0) == '0')
		{
			continue;
		}

		std::string branchNameCharge = combineComponents("_", {bias, led, pico, "Charge"});
		std::string branchNameTime = combineComponents("_", {bias, led, pico, "Time"});
		std::string branchNameVoltage = combineComponents("_", {bias, led, pico, "Voltage"});
		char *leaflist;

		asprintf(&leaflist, "data[%i]/D", wfs);

		TBranch *bC = forest.at(i0)->Branch(branchNameCharge.c_str(),
					  outData + (i0 * 3 + 0) * wfs, (const char *) leaflist);
		TBranch *bT = forest.at(i0)->Branch(branchNameTime.c_str(),
					  outData + (i0 * 3 + 1) * wfs, (const char *) leaflist);
		TBranch *bV = forest.at(i0)->Branch(branchNameVoltage.c_str(),
					  outData + (i0 * 3 + 2) * wfs, (const char *) leaflist);
		bC->Fill();
		bT->Fill();
		bV->Fill();
	}
	free(outData);

	std::cout << "###### Written branches to trees\n" << std::endl;

	return;
}

void ledPreAnalysis(std::string directory, std::string date, std::string mppcStr,
					std::vector<TTree *> forest)
{
	for (const std::string &bias : g_dcp.biasFullVec)
	{
		for (const std::string &led : g_dcp.ledShortVec)
		{
			for (const std::string &pico : picoscopeNames)
			{
				std::vector<std::string> fileVec{date, bias + "V", led + "mV", g_pmt, mppcStr, pico};
				std::string fileBasename(combineComponents("_", fileVec));
				std::string filePath(directory + "/" + fileBasename + ".dat");

				ledPreAnalysisFile(filePath, bias, led, pico, forest);
			}
		}
	}

	for (const std::string &bias : g_dcp.biasShortVec)
	{
		for (const std::string &led : g_dcp.ledFullVec)
		{
			for (const std::string &pico : picoscopeNames)
			{
				std::vector<std::string> fileVec{date, bias + "V", led + "mV", g_pmt, mppcStr, pico};
				std::string fileBasename(combineComponents("_", fileVec));
				std::string filePath(directory + "/" + fileBasename + ".dat");

				ledPreAnalysisFile(filePath, bias, led, pico, forest);
			}
		}
	}
}

void preAnalyseFolder(std::string directory, std::string date, std::string outputDirectory)
{
	while (directory.back() == '/')
	{
		directory.pop_back();
	}

	if (isExisting(directory) != true)
	{
		std::cerr << "WARNING: the path '" + directory + "' does not exist..." << std::endl;
		return;
	}

	std::string mppcStr = directory.substr(directory.find_last_of("/") + 1);
	std::vector<std::string> mppcNotesVec = stringComponents(mppcStr, '_');
	std::vector<std::string> mppcVec = stringComponents(mppcNotesVec.at(0), '-');
	std::string outputFile = outputDirectory + "/" + mppcStr;

	/* Structure of root files
	 * header folder? - metadata
	 * tree for mppc a/b/c and pmt
	 * * Branch for each file with integrated charge...
	 */

	const char *mppcTitle = "Charateristics of MPPC ";
	char *mppcBack;
	char *mppcMiddle;
	char *mppcFront;

    asprintf(&mppcBack, "%s%s", mppcTitle, mppcVec.at(0).c_str());
	asprintf(&mppcMiddle, "%s%s", mppcTitle, mppcVec.at(1).c_str());
	asprintf(&mppcFront, "%s%s", mppcTitle, mppcVec.at(2).c_str());

	TFile *file = TFile::Open((TString)outputFile + "_test.root", "RECREATE");

	TTree *treeBack = new TTree("treeBack", mppcBack);
	TTree *treeMiddle = new TTree("treeMiddle", mppcMiddle);
	TTree *treeFront = new TTree("treeFront", mppcFront);
	TTree *treePmt = new TTree("treePmt", "Charateristics of PMT");
	// tree->Branch("channel", &channel, "channel/I");
	std::vector<TTree *> forest{treeBack, treeMiddle, treeFront, treePmt};

	// TODO: Header/metadata info

	file->WriteObject(&(g_dcp.biasFullVec), "biasFullVec");
	file->WriteObject(&(g_dcp.biasShortVec), "biasShortVec");
	file->WriteObject(&(g_dcp.ledFullVec), "ledFullVec");
	file->WriteObject(&(g_dcp.ledShortVec), "ledShortVec");
	file->WriteObject(&g_pmt, "pmtVoltage");
	file->WriteObject(&picoscopeNames, "picoscopeNames");
	file->WriteObject(&mppcVec, "mppcNumbers");


	// TCanvas *c = new TCanvas();
	// c->SaveAs((g_tmpPdf + "[").c_str());
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	darkPreAnalysis(directory, date, mppcStr, forest);

	std::chrono::steady_clock::time_point endDark = std::chrono::steady_clock::now();
	int diffDark = std::chrono::duration_cast<std::chrono::seconds>(endDark-start).count();
	std::cout << "Dark pre-analysis time: " << diffDark << "s" << std::endl;

	ledPreAnalysis(directory, date, mppcStr, forest);

	std::chrono::steady_clock::time_point endLed = std::chrono::steady_clock::now();
	int diffLed = std::chrono::duration_cast<std::chrono::seconds>(endLed-endDark).count();
	std::cout << "Dark pre-analysis time: " << diffDark << "s" << std::endl;
	std::cout << "LED pre-analysis time: " << diffLed << "s" << std::endl;
	std::cout << "Total pre-analysis time: " << diffLed + diffDark << "s" << std::endl;

	// c->SaveAs((g_tmpPdf + "]").c_str());
	for (TTree *t : forest)
	{
		t->SetEntries(1);
	}

	// populate trees with branches per file input

	std::cout << "### Writing trees to file\n" << std::endl;

	treeBack->Write();
	treeMiddle->Write();
	treeFront->Write();
	treePmt->Write();
	file->Close();

	std::cout << "### File closed" << std::endl;
}

highPeResult singleSetGaussFitting(TTree *t, std::string bias, std::string led, 
		std::vector<std::string> pico)
{
	TTreeReader reader(t);
	std::string str1 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	std::string str2 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	TTreeReaderArray<Double_t> dataPico1(reader, str1.c_str());
	TTreeReaderArray<Double_t> dataPico2(reader, str2.c_str());
	reader.Next();

	if (dataPico1.GetSize() != dataPico2.GetSize())
	{
		std::cout << "ERROR: inconsistent array sizes" << std::endl;
		std::cout << " - dataPico1 Size: " << dataPico1.GetSize() << std::endl;
		std::cout << " - dataPico2 Size: " << dataPico2.GetSize() << std::endl;
	}
	const int n = dataPico1.GetSize() + dataPico2.GetSize();
	double fullData[n];
	double chargeMin = dataPico1[0];
	double chargeMax = dataPico1[0];
	for (uint32_t i = 0 ; i < dataPico1.GetSize() ; i++)
	{
		fullData[i] = dataPico1[i];
		fullData[i + dataPico1.GetSize()] = dataPico2[i];

		if (dataPico1[i] < chargeMin) chargeMin = dataPico1[i];
		if (dataPico2[i] < chargeMin) chargeMin = dataPico2[i];
		if (dataPico1[i] > chargeMax) chargeMax = dataPico1[i];
		if (dataPico2[i] > chargeMax) chargeMax = dataPico2[i];
	}

	double padding = (chargeMax - chargeMin) * 0.05;

	return highPeAnalysis(fullData, n, chargeMin - padding, chargeMax + padding, 500);
	// XXX: global parameters for some of this should be used instead of hardcoded
}

std::vector<std::vector<std::vector<highPeResult>>> gaussFitting(
		dataCollectionParameters &dcp, std::vector<TTree*> forest,
		std::vector<std::string> pico)
{
	TCanvas *c = new TCanvas();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpGauss.pdf[");
	std::vector<std::vector<std::vector<highPeResult>>> gaussFits;
	for (TTree* t : forest)
	{
		std::vector<std::vector<highPeResult>> chGaussFits;
		for (std::string bias : dcp.biasFullVec)
		{
			std::vector<highPeResult> biasGaussFits;
			for (std::string led : dcp.ledShortVec)
			{
				biasGaussFits.push_back(singleSetGaussFitting(t, bias, led, pico));
			}
			chGaussFits.push_back(biasGaussFits);
		}

		for (std::string bias : dcp.biasShortVec)
		{
			std::vector<highPeResult> biasGaussFits;
			for (std::string led : dcp.ledFullVec)
			{
				biasGaussFits.push_back(singleSetGaussFitting(t, bias, led, pico));
			}
			chGaussFits.push_back(biasGaussFits);
		}
		gaussFits.push_back(chGaussFits);
	}
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpGauss.pdf]");
	delete c;
	return gaussFits;
}

individualPeResult singleSetPoissFitting(TTree *t, std::string bias, std::string led, 
		std::vector<std::string> pico)
{
	TTreeReader reader(t);
	std::string str1 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	std::string str2 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	TTreeReaderArray<Double_t> dataPico1(reader, str1.c_str());
	TTreeReaderArray<Double_t> dataPico2(reader, str2.c_str());
	reader.Next();

	if (dataPico1.GetSize() != dataPico2.GetSize())
	{
		std::cout << "ERROR: inconsistent array sizes" << std::endl;
		std::cout << " - dataPico1 Size: " << dataPico1.GetSize() << std::endl;
		std::cout << " - dataPico2 Size: " << dataPico2.GetSize() << std::endl;
	}
	const int n = dataPico1.GetSize() + dataPico2.GetSize();
	double fullData[n];
	double chargeMin = dataPico1[0];
	double chargeMax = dataPico1[0];
	for (uint32_t i = 0 ; i < dataPico1.GetSize() ; i++)
	{
		fullData[i] = dataPico1[i];
		fullData[i + dataPico1.GetSize()] = dataPico2[i];

		if (dataPico1[i] < chargeMin) chargeMin = dataPico1[i];
		if (dataPico2[i] < chargeMin) chargeMin = dataPico2[i];
		if (dataPico1[i] > chargeMax) chargeMax = dataPico1[i];
		if (dataPico2[i] > chargeMax) chargeMax = dataPico2[i];
	}

	double padding = (chargeMax - chargeMin) * 0.05;

	return individualPeAnalysis(fullData, n, chargeMin - padding, chargeMax + padding, g_nBins);
	// XXX: global parameters for some of this should be used instead of hardcoded
}

std::vector<std::vector<std::vector<individualPeResult>>> poissFitting(
		dataCollectionParameters &dcp, std::vector<TTree*> forest,
		std::vector<std::string> pico)
{
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits;
	TCanvas *c = new TCanvas();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPoiss.pdf[");
	for (TTree* t : forest)
	{
		if (t == forest.at(3)) continue;
		std::vector<std::vector<individualPeResult>> chPoissFits;
		for (std::string bias : dcp.biasFullVec)
		{
			std::vector<individualPeResult> biasPoissFits;
			for (std::string led : dcp.ledShortVec)
			{
				if (std::stoi(led) > g_highPeCutoff) continue;
				biasPoissFits.push_back(singleSetPoissFitting(t, bias, led, pico));
			}
			chPoissFits.push_back(biasPoissFits);
		}

		for (std::string bias : dcp.biasShortVec)
		{
			std::vector<individualPeResult> biasPoissFits;
			for (std::string led : dcp.ledFullVec)
			{
				if (std::stoi(led) > g_highPeCutoff) continue;
				biasPoissFits.push_back(singleSetPoissFitting(t, bias, led, pico));
			}
			chPoissFits.push_back(biasPoissFits);
		}
		poissFits.push_back(chPoissFits);
	}
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPoiss.pdf]");
	delete c;
	return poissFits;
}

fileResults genericAnalysis(std::string filePath, std::string outputPath)
{
	TFile *file = TFile::Open((TString) filePath, "READ");

	std::vector<std::string>* biasFullVec = (std::vector<std::string>*)file->Get("biasFullVec");
	std::vector<std::string>* biasShortVec = (std::vector<std::string>*)file->Get("biasShortVec");
	std::vector<std::string>* ledFullVec = (std::vector<std::string>*)file->Get("ledFullVec");
	std::vector<std::string>* ledShortVec = (std::vector<std::string>*)file->Get("ledShortVec");
	std::vector<std::string>* picoscopeNames = (std::vector<std::string>*)file->Get("picoscopeNames");
	std::vector<std::string>* mppcNumbers = (std::vector<std::string>*)file->Get("mppcNumbers");

	std::string fileName = filePath.substr(filePath.find_last_of("/") + 1);
	std::string mppcStr = fileName.substr(0, fileName.size() - 5);
	std::cout << mppcStr << std::endl;

	TTree *treeBack = (TTree*) file->Get("treeBack");
	TTree *treeMiddle = (TTree*) file->Get("treeMiddle");
	TTree *treeFront = (TTree*) file->Get("treeFront");
	TTree *treePmt = (TTree*) file->Get("treePmt");

	std::vector<TTree*> forest = {treeBack, treeMiddle, treeFront, treePmt};

	std::vector<std::string> allBias(*biasFullVec);
	allBias.insert(allBias.end(), biasShortVec->begin(), biasShortVec->end());

	std::vector<double> ledShortX;
	for (std::string led : *ledShortVec) ledShortX.push_back(std::stod(led));
	std::vector<double> ledFullX;
	for (std::string led : *ledFullVec) ledFullX.push_back(std::stod(led));

	// 3D vectors: num channels (4) x num bias voltages x num applicable LED voltages
	std::vector<std::vector<std::vector<highPeResult>>> gaussFits; // ALL LED V fits
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits; // only low PE LED V'
	std::vector<std::vector<std::vector<darkResult>>> darkFits;
	
	while (outputPath.back() == '/')
	{
		outputPath.pop_back();
	}

	TString pdfFile(outputPath + "/" + mppcStr + ".pdf");
	TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);

	dataCollectionParameters dcp = {*biasFullVec, *ledFullVec, *biasShortVec, *ledShortVec};

	ROOT::Math::MinimizerOptions* minOpt = new ROOT::Math::MinimizerOptions();
	minOpt->SetTolerance(0.001);

	gStyle->SetStatW(0.14);
	gStyle->SetStatH(0.14);
	gStyle->SetStatX(0.4);
	gStyle->SetStatY(0.85);
	gStyle->SetOptStat(0);
	gStyle->SetOptFit(11);
	gaussFits = gaussFitting(dcp, forest, *picoscopeNames);

	poissFits = poissFitting(dcp, forest, *picoscopeNames);

	std::vector<std::vector<highPeResult>> pmtGauss = gaussFits.at(3);

	fileResults res = {*mppcNumbers, dcp, gaussFits, poissFits, darkFits};

	Ctmp->SaveAs(pdfFile + "[");

	for (int i = 0 ; i < 3 ; i++)
	{
		TString title = TString("MPPC " + (*mppcNumbers).at(i) + " Intensity Dependence");
		TCanvas *c = new TCanvas((TString)(*mppcNumbers).at(i), title);
		c->cd();
		TMultiGraph *mg = new TMultiGraph("mg", title.Data());
		TLegend *leg = new TLegend(0.13, 0.48, 0.23, 0.88);

		std::vector<std::vector<highPeResult>> chGauss = gaussFits.at(i);
		std::vector<std::vector<individualPeResult>> chPoiss = poissFits.at(i);

		for (int j = 0 ; j < (int) chGauss.size() ; j++)
		{
			std::string bias = allBias.at(j);
			std::vector<highPeResult> biasGauss = chGauss.at(j);
			std::vector<individualPeResult> biasPoiss = chPoiss.at(j);
			std::vector<double> x; // mean pmt
			std::vector<double> y1; // mean mppc [mV ns]
			// std::vector<double> y2; // mean mppc [PE]
			// double totalSep = 0;

			// for (individualPeResult res : chPoiss)
			// {
			// 	totalSep += res.pePeakSeparation;
			// }

			// double meanSep = abs(totalSep) / chPoiss.size();

			// if (biasGauss.size() == ledShortX.size())
			// {
			// 	x = ledShortX;
			// }
			// else
			// {
			// 	x = ledFullX;
			// }

			for (int k = 0 ; k < (int) biasGauss.size() ; k++)
			{
				y1.push_back(biasGauss.at(k).mean * -1);
				// y2.push_back(biasGauss.at(k).mean * -1 / meanSep);
				x.push_back(pmtGauss.at(j).at(k).mean * -1);
			}

			TGraph *gr = new TGraph(x.size(), x.data(), y1.data());
			gr->SetLineColor(j + 1);
			gr->SetMarkerColor(j + 1);
			gr->SetMarkerSize(0.75);
			gr->SetMarkerStyle(8);
			gr->SetFillColor(j + 1);
			mg->Add(gr, "PL");
			leg->AddEntry(gr, bias.c_str(), "lp");
		}
		mg->GetXaxis()->SetTitle("PMT Signal [mV ns]");
		mg->GetYaxis()->SetTitle("MPPC Signal [mV ns]");
		mg->Draw("ALP");
		leg->Draw();
		c->Modified();
		c->Update();
		c->SaveAs(pdfFile);
	}
	Ctmp->SaveAs(pdfFile + "]");
	std::cout << g_maxPeaks << std::endl;
	return res;
}

void cycleAnalysis(std::string file1, std::string file2, std::string file3, 
		std::string outputPath)
{
	// We assume file 1 is a-b-c, file 2 c-a-b, file 3 is b-c-a

	TCanvas *Ctmp = new TCanvas();

	fileResults r1 = genericAnalysis(file1, outputPath);
	fileResults r2 = genericAnalysis(file2, outputPath);
	fileResults r3 = genericAnalysis(file3, outputPath);

	while (outputPath.back() == '/')
	{
		outputPath.pop_back();
	}

	std::string pdfFile = outputPath + "/" + 
			combineComponents("-", r1.mppcNumbers) + "_CycleAnalysis.pdf";

	std::vector<std::string> allBias(r1.dcp.biasFullVec);
	allBias.insert(allBias.end(), r1.dcp.biasShortVec.begin(), r1.dcp.biasShortVec.end());

	Ctmp->SaveAs((pdfFile + "[").c_str());

	for (int i = 0 ; i < 3 ; i++)
	{
		std::string mppcNumber = r1.mppcNumbers.at(i);
		int mppcIndex1 = i;
		int mppcIndex2;
		int mppcIndex3;
		auto checkR2 = std::find(r2.mppcNumbers.begin(), r2.mppcNumbers.end(), mppcNumber);
		auto checkR3 = std::find(r3.mppcNumbers.begin(), r3.mppcNumbers.end(), mppcNumber);

		if (checkR2 != r2.mppcNumbers.end())
		{
			mppcIndex2 = std::distance(r2.mppcNumbers.begin(), checkR2);
		}
		else
		{
			std::cout << "ERROR: MPPC Number " << mppcNumber << " is not found in "
					<< file2 << std::endl;
			exit(0);
		}

		if (checkR3 != r3.mppcNumbers.end())
		{
			mppcIndex3 = std::distance(r3.mppcNumbers.begin(), checkR3);
		}
		else
		{
			std::cout << "ERROR: MPPC Number " << mppcNumber << " is not found in "
					<< file3 << std::endl;
			exit(0);
		}

		// index 1 will be centre, 0 is furthest/mppc A, 2 is closest/mppc B
		std::vector<fileResults> fVec(3);
		fVec.at(mppcIndex1) = r1;
		fVec.at(mppcIndex2) = r2;
		fVec.at(mppcIndex3) = r3;

		std::string titleA = "MPPC " + mppcNumber + " Further / Centre";
		std::string titleC = "MPPC " + mppcNumber + " Closer / Centre";

		TCanvas *c = new TCanvas();
		TMultiGraph *mgA = new TMultiGraph("mgA", titleA.c_str());
		TMultiGraph *mgC = new TMultiGraph("mgC", titleC.c_str());
		TLegend *legA = new TLegend(0.78, 0.48, 0.88, 0.88);
		TLegend *legC = new TLegend(0.78, 0.48, 0.88, 0.88);

		std::vector<std::vector<highPeResult>> chGaussA = fVec.at(0).gaussFits.at(0);
		std::vector<std::vector<highPeResult>> chGaussB = fVec.at(1).gaussFits.at(1);
		std::vector<std::vector<highPeResult>> chGaussC = fVec.at(2).gaussFits.at(2);

		std::vector<std::vector<highPeResult>> chPmtA = fVec.at(0).gaussFits.at(3);
		std::vector<std::vector<highPeResult>> chPmtB = fVec.at(1).gaussFits.at(3);
		std::vector<std::vector<highPeResult>> chPmtC = fVec.at(2).gaussFits.at(3);
		// std::vector<std::vector<individualPeResult>> chPoiss = poissFits.at(i);

		for (int j = 0 ; j < (int) chGaussB.size() ; j++)
		{
			std::string bias = allBias.at(j);
			std::vector<highPeResult> biasGaussA = chGaussA.at(j);
			std::vector<highPeResult> biasGaussB = chGaussB.at(j);
			std::vector<highPeResult> biasGaussC = chGaussC.at(j);

			std::vector<double> x; // mean pmt of mppc B
			std::vector<double> yA; // mean mppc A / mppc B * pmt B / pmt A
			std::vector<double> yC; // mean mppc C / mppc B * pmt B / pmt C

			for (int k = 0 ; k < (int) biasGaussB.size() ; k++)
			{
				// y1.push_back(biasGaussB.at(k).mean * -1);
				// y2.push_back(biasGauss.at(k).mean * -1 / meanSep);
				yA.push_back(biasGaussA.at(k).mean / biasGaussB.at(k).mean * 
						chPmtB.at(j).at(k).mean / chPmtA.at(j).at(k).mean);
				yC.push_back(biasGaussC.at(k).mean / biasGaussB.at(k).mean * 
						chPmtB.at(j).at(k).mean / chPmtC.at(j).at(k).mean);
				x.push_back(chPmtB.at(j).at(k).mean * -1);
			}

			TGraph *grA = new TGraph(x.size(), x.data(), yA.data());
			TGraph *grC = new TGraph(x.size(), x.data(), yC.data());
			grA->SetLineColor(j + 1);
			grA->SetMarkerColor(j + 1);
			grA->SetMarkerSize(0.75);
			grA->SetMarkerStyle(8);
			grA->SetFillColor(j + 1);
			grC->SetLineColor(j + 1);
			grC->SetMarkerColor(j + 1);
			grC->SetMarkerSize(0.75);
			grC->SetMarkerStyle(8);
			grC->SetFillColor(j + 1);

			mgA->Add(grA, "PL");
			mgC->Add(grC, "PL");
			legA->AddEntry(grA, bias.c_str(), "lp");
			legC->AddEntry(grC, bias.c_str(), "lp");
		}
		mgA->GetXaxis()->SetTitle("PMT Signal [mV ns]");
		mgA->GetYaxis()->SetTitle("Further signal / Centre signal");
		mgA->Draw("ALP");
		legA->Draw();
		c->Modified();
		c->Update();
		c->SaveAs(pdfFile.c_str());

		mgC->GetXaxis()->SetTitle("PMT Signal [mV ns]");
		mgC->GetYaxis()->SetTitle("Closer signal / Centre signal");
		mgC->Draw("ALP");
		legC->Draw();
		c->Modified();
		c->Update();
		c->SaveAs(pdfFile.c_str());

		delete mgA; delete mgC; delete legA; delete legC; delete c;
	}
	Ctmp->SaveAs((pdfFile + "]").c_str());
	delete Ctmp;
}

// void tempAnalysis(std::string inputFolder,
// 				  std::string filenamePattern,
// 				  std::string outputFolder)
// {
// 	const Double_t highLed = std::stod(g_highestLed);

// 	environmentSample temp;
// }


// XXX: Probably don't want to do this but it is good to keep in mind
// PYBIND11_MODULE(analysis, m)
// {
//     m.doc() = "Picoscope 6000a DAQ System";

//     m.def("runFullDAQ", &runFullDAQ, py::return_value_policy::copy);
//     m.def("seriesInitDaq", &seriesInitDaq, py::return_value_policy::copy);
//     m.def("seriesSetDaqSettings", &seriesSetDaqSettings, py::return_value_policy::copy);
//     m.def("seriesCollectData", &seriesCollectData, py::return_value_policy::copy);
//     m.def("seriesCloseDaq", &seriesCloseDaq, py::return_value_policy::copy);
//     m.def("multiSeriesInitDaq", &multiSeriesInitDaq, py::return_value_policy::copy);
//     m.def("multiSeriesSetDaqSettings", &multiSeriesSetDaqSettings, py::return_value_policy::copy);
//     m.def("multiSeriesCollectData", &multiSeriesCollectData, py::return_value_policy::copy);
//     m.def("multiSeriesCloseDaq", &multiSeriesCloseDaq, py::return_value_policy::copy);
//     m.def("getSerials", &getSerials, py::return_value_policy::copy);
// }

///////////////////////////////////////////////////////////////////////////////
///                              Main function                              ///
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	col->SetRGB(0.5,0.5,0.5);
	if (argc < 2)
	{
		std::cerr << "ERROR: you need at least 1 argument: 'batch-pre-analyse', 'pre-analyse' or 'analyse'..." << std::endl;
		return 1;
	}
	std::string analysisType(argv[1]);
	if (analysisType == "batch-pre-analyse")
	{
		if (argc < 3)
		{
			std::cerr << "ERROR: you should have at least 3 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::vector<std::string> datFiles(argv + 3, argv + argc);
		batchPreAnalysis(outputDir, datFiles);
	}
	else if (analysisType == "pre-analyse")
	{
		if (argc != 5)
		{
			std::cerr << "ERROR: you should have 4 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string inputDir(argv[2]);
		std::string date(argv[3]);
		std::string outputDir(argv[4]);
		preAnalyseFolder(inputDir, date, outputDir);
	}
	else if (analysisType == "analyse")
	{
		if (argc < 3)
		{
			std::cerr << "ERROR: you should have 3 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string inputFile(argv[3]);
		genericAnalysis(inputFile, outputDir);
	}
	else if (analysisType == "cycle-analyse")
	{
		if (argc < 3)
		{
			std::cerr << "ERROR: you should have 5 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string inputFile1(argv[3]);
		std::string inputFile2(argv[4]);
		std::string inputFile3(argv[5]);
		cycleAnalysis(inputFile1, inputFile2, inputFile3, outputDir);
	}
	return 0;
}

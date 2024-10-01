#include "constants.h"

///////////////////////////////////////////////////////////////////////////////
///                            General functions                            ///
///////////////////////////////////////////////////////////////////////////////

bool isExisting(const std::string &name)
{
	return (access(name.c_str(), F_OK) != -1);
}

std::vector<std::string> stringComponents(const std::string &name, const char delim = '_')
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
				sample newSample{adc2mv(chADCData[i0 * nSamples + i1], d.chVRanges.at(ch)), timeBase * i1};
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

// TODO: Do we really need it???
void plottingMinimumMovingAverage(const std::string fileNameBase,
								  const std::vector<std::vector<std::vector<sample>>> &data,
								  std::vector<plot1D> &plots1D,
								  std::vector<std::vector<double>> pulseMinimumMovingAverageMatrix,
								  const int nBins,
								  const int windowLowerEdge,
								  const int windowUpperEdge,
								  const int nHalfAverage,
								  const int nSamples,
								  const double timeBase)
{
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
		gaussParams baseLineValue(baseLine(dataChannel.at(i0), g_baselineLowerWindow, g_baselineUpperWindow));
		Double_t charge = chargeIntegrationFixed(dataChannel.at(i0), timebase, baseLineValue.mean, lowerWindow, upperWindow);
		// points minSample = getMinData(dataChannel.at(i0));
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
	for (uint i0(0) ; i0 < header.activeChannels.length() ; ++i0)
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

int64_t factorial(const int64_t n) // up to 
{
	return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

double factorialD(const int n)
{
	return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

double gaussDistribution(double x, double mu, double sigma)
{
	return exp(-0.5 * square(x - mu) / square(sigma)) / (sigma * sqrt(PI2_CONST));
}

double poissGaussDistribution(double x, double ampl, double lambda, double q0, 
		double sigma0, double mu, double sigma, int nPeaks)
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
	for (int i = 0 ; i < nPeaks ; i++)
	{
		if (i < 21) // factorial loses int64 precision at this level
		{
			total += (pow(lambda, i) * exp(-lambda) / factorial(i) ) * 
			gaussDistribution(x, i * mu + q0, sqrt(square(i*sigma) + square(sigma0)));
		}
		else if (i < 171) // factorial exceeds double exponent range at this level
		{
			total += (pow(lambda, i) * exp(-lambda) / factorialD(i) ) * 
			gaussDistribution(x, i * mu + q0, sqrt(square(i*sigma) + square(sigma0)));
		}
		else
		{
			std::cout << "WARNING: i out of exponent range for factorialD" << std::endl;
			break;
		}
	}
	return total * ampl;
}

individualPeResult individualPeAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins)
{
	TCanvas *c = new TCanvas("c", "c");
	TH1D* hist = new TH1D("hist", "Individual Photoelectron Signal", nbins, xmin, xmax);


	TF1* fn = new TF1("poissGaus", [](double *x, double *p){return
		poissGaussDistribution(x[0], p[0], p[1], p[2], p[3], p[4], p[5], p[6]);}, xmin, xmax, 7);

	for (int i = 0 ; i < nWfs ; i++)
	{
		hist->Fill(data[i]);
	}

	hist->Sumw2();

	individualPeResult bestRes;
	double bestChi2 = DBL_MAX;
	TH1D* bestHist;

	for (int i = 2 ; i < 17 ; i++)
	{
		fn->SetParameter(0, nWfs);
		fn->SetParameter(1, 3);
		fn->SetParameter(2, 0);
		fn->SetParLimits(2, -10, 10);
		fn->SetParameter(3, 5);
		fn->SetParameter(4, -50);
		fn->SetParameter(5, 5);
		fn->FixParameter(6, i);
		for (int j = 0 ; j < 7 ; j++) fn->SetParError(j, 0);

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
		}
	}
	bestHist->Sumw2(kFALSE);
	bestHist->Draw();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpPoiss.pdf");

	delete hist;
	delete bestHist;
	delete c;

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

	hist->Draw();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpGauss.pdf");
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
	if (doMovingAverage == true)
	{
		tree->Branch("pulseMinimumMovingAverage", &pulseMinimumMovingAverage, "pulseMinimumMovingAverage/D");
	}
	tree->Branch("integratedCharge", &integratedCharge, "integratedCharge/D");
	for (int i0(0); i0 < 4; ++i0)
	{
		int n0(pulseMinimumMatrix[i0].size());
		int n1(pulseMinimumSampleNumberMatrix[i0].size());
		int n2(pulseMinimumTimeMatrix[i0].size());
		int n3(pulseMinimumMovingAverageMatrix[i0].size());
		int n4(integratedChargeMatrix[i0].size());
		if (doMovingAverage == true)
		{
			if (n0 != n1 || n0 != n2 || n0 != n3 || n0 != n4)
			{
				std::cout << "ERROR: vectors should all have the same size..." << std::endl;
				continue;
			}
		}
		else
		{
			if (n0 != n1 || n0 != n2 || n0 != n4)
			{
				std::cout << "ERROR: vectors should all have the same size (except for moving average)..." << std::endl;
				continue;
			}
		}
		for (int i1(0); i1 < n0; ++i1)
		{
			channel = i0;
			pulseMinimum = pulseMinimumMatrix[i0][i1];
			pulseMinimumSampleNumber = pulseMinimumSampleNumberMatrix[i0][i1];
			pulseMinimumTime = pulseMinimumTimeMatrix[i0][i1];
			if (doMovingAverage == true)
			{
				pulseMinimumMovingAverage = pulseMinimumMovingAverageMatrix[i0][i1];
			}
			else
			{
				pulseMinimumMovingAverage = 0;
			}
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
	int nBinsT(nSamples), nBinsV(50), nBinsMinimumTime(nSamples), nBinsPulseMinimum(200), nBinsIntegrated(200), nHalfAverage(10), windowLowerEdge(10), windowUpperEdge(50);

	plottingFirstWaveforms(fileBasename, data, plots2D, nBinsT, nBinsV, nSamples, timeBase);
	std::cout << "### Plotted first waveforms..." << std::endl;
	plottingMinimum(fileBasename, data, plots1D, pulseMinimumMatrix, pulseMinimumSampleNumberMatrix, pulseMinimumTimeMatrix, nBinsMinimumTime, nBinsPulseMinimum, nSamples, timeBase);
	std::cout << "### Plotted pulse minimum and time..." << std::endl;
	if (doMovingAverage == true)
	{
		plottingMinimumMovingAverage(fileBasename, data, plots1D, pulseMinimumMovingAverageMatrix, nBinsPulseMinimum, windowLowerEdge, windowUpperEdge, nHalfAverage, nSamples, timeBase);
		std::cout << "### Plotted moving averaged minimum peaks value..." << std::endl;
	}
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

int darkPreAnalysis(std::string directory, std::string date, std::string mppcStr,
					 std::vector<TTree *> forest)
{
	int wfs;

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
			
			wfs = (const int) header.numWaveforms;
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

	return wfs;
}

int ledPreAnalysisFile(std::string filePath, std::string bias,
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

	return wfs;
}

int ledPreAnalysis(std::string directory, std::string date, std::string mppcStr,
					std::vector<TTree *> forest)
{
	int wfs;

	for (const std::string &bias : g_dcp.biasFullVec)
	{
		for (const std::string &led : g_dcp.ledShortVec)
		{
			for (const std::string &pico : picoscopeNames)
			{
				std::vector<std::string> fileVec{date, bias + "V", led + "mV", g_pmt, mppcStr, pico};
				std::string fileBasename(combineComponents("_", fileVec));
				std::string filePath(directory + "/" + fileBasename + ".dat");

				wfs = ledPreAnalysisFile(filePath, bias, led, pico, forest);
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

				wfs = ledPreAnalysisFile(filePath, bias, led, pico, forest);
			}
		}
	}

	return wfs;
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

	TFile *file = TFile::Open((TString)outputFile + ".root", "RECREATE");

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

	darkPreAnalysis(directory, date, mppcStr, forest);

	ledPreAnalysis(directory, date, mppcStr, forest);

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
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpGauss.pdf[");
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
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpGauss.pdf]");
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

	return individualPeAnalysis(fullData, n, chargeMin - padding, chargeMax + padding, 500);
	// XXX: global parameters for some of this should be used instead of hardcoded
}

std::vector<std::vector<std::vector<individualPeResult>>> poissFitting(
		dataCollectionParameters &dcp, std::vector<TTree*> forest,
		std::vector<std::string> pico)
{
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits;
	TCanvas *c = new TCanvas();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpPoiss.pdf[");
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
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/7-8-9_tmpPoiss.pdf]");
	delete c;
	return poissFits;
}

void genericAnalysis(std::string filePath, std::string outputPath)
{
	TFile *file = TFile::Open((TString) filePath, "READ");

	std::vector<std::string>* biasFullVec = (std::vector<std::string>*)file->Get("biasFullVec");
	std::vector<std::string>* biasShortVec = (std::vector<std::string>*)file->Get("biasShortVec");
	std::vector<std::string>* ledFullVec = (std::vector<std::string>*)file->Get("ledFullVec");
	std::vector<std::string>* ledShortVec = (std::vector<std::string>*)file->Get("ledShortVec");
	std::vector<std::string>* picoscopeNames = (std::vector<std::string>*)file->Get("picoscopeNames");
	std::vector<std::string>* mppcNumbers = (std::vector<std::string>*)file->Get("mppcNumbers");

	std::string mppcStr = combineComponents("-", *mppcNumbers);

	TTree *treeBack = (TTree*) file->Get("treeBack");
	TTree *treeMiddle = (TTree*) file->Get("treeMiddle");
	TTree *treeFront = (TTree*) file->Get("treeFront");
	TTree *treePmt = (TTree*) file->Get("treePmt");

	std::vector<TTree*> forest = {treeBack, treeMiddle, treeFront, treePmt};

	std::vector<std::string> allBias(*biasFullVec);
	allBias.insert(allBias.end(), biasShortVec->begin(), biasShortVec->end());
	// std::vector<std::string> allBias(biasFullVec->size() + biasShortVec->size());
	// std::merge(biasFullVec->begin(), biasFullVec->end(), biasShortVec->begin(),
	// 	biasShortVec->end(), allBias.begin());

// for (std::string bias : *biasFullVec) std::cout << bias << std::endl;
// for (std::string bias : *biasShortVec) std::cout << bias << std::endl;
// 	for (std::string bias : allBias) std::cout << bias << std::endl;
	
	std::vector<double> ledShortX;
	for (std::string led : *ledShortVec) ledShortX.push_back(std::stod(led));
	std::vector<double> ledFullX;
	for (std::string led : *ledFullVec) ledFullX.push_back(std::stod(led));

	// 3D vectors: num channels (4) x num bias voltages x num applicable LED voltages
	std::vector<std::vector<std::vector<highPeResult>>> gaussFits; // ALL LED V fits
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits; // only low PE LED V
	
	while (outputPath.back() == '/')
	{
		outputPath.pop_back();
	}

	TString pdfFile(outputPath + "/" + mppcStr + ".pdf");
	TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);

	dataCollectionParameters dcp = {*biasFullVec, *ledFullVec, *biasShortVec, *ledShortVec};

	ROOT::Math::MinimizerOptions* minOpt = new ROOT::Math::MinimizerOptions();
	minOpt->SetTolerance(0.001);

	gaussFits = gaussFitting(dcp, forest, *picoscopeNames);
	poissFits = poissFitting(dcp, forest, *picoscopeNames);

	std::vector<std::vector<highPeResult>> pmtGauss = gaussFits.at(3);

	Ctmp->SaveAs(pdfFile + "[");

	for (int i = 0 ; i < 3 ; i++)
	{
		TString title = TString("MPPC " + (*mppcNumbers).at(i) + " High PE");
		TCanvas *c = new TCanvas((TString)(*mppcNumbers).at(i), title);
		c->cd();
		TMultiGraph *mg = new TMultiGraph("mg", title.Data());
		TLegend *leg = new TLegend(0.13, 0.48, 0.23, 0.88);
		TGraph *gr;

		std::vector<std::vector<highPeResult>> chGauss = gaussFits.at(i);

		for (int j = 0 ; j < (int) chGauss.size() ; j++)
		{
			std::string bias = allBias.at(j);
			std::vector<highPeResult> biasGauss = chGauss.at(j);
			std::vector<double> x; // mean mppc / mean pmt
			std::vector<double> y; // LED

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
				y.push_back(biasGauss.at(k).mean * -1);
				x.push_back(pmtGauss.at(j).at(k).mean * -1);
			}

			gr = new TGraph((Int_t) x.size(), x.data(), y.data());
			gr->SetLineColor(j + 1);
			gr->SetMarkerColor(j + 1);
			gr->SetFillColor(j + 1);
			mg->Add(gr, "*PL");
			leg->AddEntry(gr, bias.c_str(), "lp");
		}
		mg->Draw("ALP");
		leg->Draw();
		c->Modified();
		c->Update();
		c->SaveAs(pdfFile);
	}
	Ctmp->SaveAs(pdfFile + "]");
}

void tempAnalysis(std::string inputFolder,
				  std::string filenamePattern,
				  std::string outputFolder)
{
	// const Double_t highLed = std::stod(g_highestLed);

	// environmentSample temp;
}

///////////////////////////////////////////////////////////////////////////////
///                              Main function                              ///
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	// testOutFile("/home/amiles/Documents/PhD/mppc-qc/pre-analyse/root-files/1-2-3.root");
	// readEnvData("../data/temp20240726.txt");
	// return 1;

	col->SetRGB(0.5,0.5,0.5);
	// genericAnalysis("/home/amiles/Documents/PhD/mppc-qc/pre-analyse/root-files/7-8-9.root",
	// 				"/home/amiles/Documents/PhD/mppc-qc/plots/");return 1;
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

	}
	else if (analysisType == "old-analyse")
	{
		if (argc != 7)
		{
			std::cerr << "ERROR: you should have 7 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string analysisSubType(argv[2]);
		if (analysisSubType == "maxPMT-maxMPPC")
		{
			std::string date(argv[3]);
			std::string MPPCsNumbers(argv[4]);
			std::string directory0(argv[5]);
			std::string directory1(argv[6]);
			for (int i0(0); i0 < (int)MPPCsHV.size(); ++i0)
			{
				for (int i1(0); i1 < (int)picoscopeNames.size(); ++i1)
				{
					clock_t startTime, endTime;
					std::cout << "\n######### Starting analysis: PMT maximum VS PMT voltage & MPPCs maximum VS LED voltage" << std::endl;
					startTime = clock();
					std::cout << "###### Starting setting up histograms..." << std::endl;
					std::vector<std::vector<std::vector<TH1D *>>> integratedChargeTensor;
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						std::vector<std::vector<TH1D *>> integratedChargeMatrix;
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							std::vector<TH1D *> integratedChargeVec;
							for (int i4(0); i4 < 4; ++i4)
							{
								std::string fileNameBase(date + "_" + MPPCsHV[i0] + "_" + LEDV[i2] + "_" + PMTV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
								TH1D *integratedChargeHisto = new TH1D();
								TString ch(std::string(1, 'A' + i4));
								integratedChargeHisto->SetName(fileNameBase + "_ch" + ch);
								integratedChargeHisto->SetTitle(";Q [mV.ns];count");
								integratedChargeHisto->SetBins(100, 0, 100);
								integratedChargeVec.push_back(integratedChargeHisto);
							}
							integratedChargeMatrix.push_back(integratedChargeVec);
						}
						integratedChargeTensor.push_back(integratedChargeMatrix);
					}
					std::vector<TH1D *> maxMPPCvsLEDVVec;
					std::vector<TH1D *> maxMPPCvsLEDVVecNorm;
					std::vector<TH1D *> maxPMTvsPMTVVec;
					for (int i2(0); i2 < 4; ++i2)
					{
						std::string ch(std::string(1, 'A' + i2));
						TString name("maxMPPCvsLEDV_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1] + "_ch" + ch);
						TH1D *maxMPPCvsLEDV = new TH1D();
						maxMPPCvsLEDV->SetName(name);
						maxMPPCvsLEDV->SetTitle(";U_{LED} [mV];max_{MPPC} [mV]");
						maxMPPCvsLEDV->SetBins(100, 800, 900);
						maxMPPCvsLEDVVec.push_back(maxMPPCvsLEDV);
						name = "maxMPPCvsLEDVNorm_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1] + "_ch" + ch;
						TH1D *maxMPPCvsLEDVNorm = new TH1D();
						maxMPPCvsLEDVNorm->SetName(name);
						maxMPPCvsLEDVNorm->SetTitle(";U_{LED} [mV];count");
						maxMPPCvsLEDVNorm->SetBins(100, 800, 900);
						maxMPPCvsLEDVVecNorm.push_back(maxMPPCvsLEDVNorm);
						if (i2 == 3)
						{
							for (int i3(0); i3 < (int)LEDV.size(); ++i3)
							{
								TString name("maxPMTvsPMTV_" + date + "_" + MPPCsHV[i0] + "_" + LEDV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
								TH1D *maxPMTvsPMTV = new TH1D();
								maxPMTvsPMTV->SetName(name);
								maxPMTvsPMTV->SetTitle(";U_{PMT} [kV];max_{PMT} [mV]");
								maxPMTvsPMTV->SetBins(300, 1.2, 1.5);
								maxPMTvsPMTVVec.push_back(maxPMTvsPMTV);
							}
						}
					}
					std::cout << "###### Finished setting up histograms..." << std::endl;
					std::cout << "###### Starting formating pre-analysed data..." << std::endl;
					bool canBeAnalysed(false);
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							std::string fileNameBase(date + "_" + MPPCsHV[i0] + "_" + LEDV[i2] + "_" + PMTV[i3] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
							std::string fileToProcess(directory0 + "root-files/" + fileNameBase + ".root");
							if (isExisting(fileToProcess) != true)
							{
								std::cout << "WARNING: the file '" + fileToProcess + "' does not exist..." << std::endl;
								continue;
							}
							else
							{
								canBeAnalysed = true;
								std::cout << "### Formating file: " + fileToProcess << std::endl;
								TFile *file = TFile::Open((TString)fileToProcess);
								if (!file)
								{
									std::cout << "ERROR: could not open '" << fileToProcess << "'..." << std::endl;
									return 1;
								}
								tree = dynamic_cast<TTree *>(file->Get("tree"));
								if (!tree)
								{
									std::cout << "ERROR: could not find 'tree'..." << std::endl;
									return 1;
								}
								tree->SetBranchAddress("channel", &channel);
								tree->SetBranchAddress("integratedCharge", &integratedCharge);
								Long64_t nRows(tree->GetEntries());
								double maxIntegratedCharge(0), minIntegratedCharge(0);
								for (int i4(0); i4 < nRows; ++i4)
								{
									tree->GetEntry(i4);
									if (integratedCharge > maxIntegratedCharge)
									{
										maxIntegratedCharge = integratedCharge;
									}
									if (integratedCharge < minIntegratedCharge)
									{
										minIntegratedCharge = integratedCharge;
									}
								}
								for (int i4(0); i4 < 4; ++i4)
								{
									integratedChargeTensor[i2][i3][i4]->SetBins(300, minIntegratedCharge, maxIntegratedCharge);
								}
								for (int i4(0); i4 < nRows; ++i4)
								{
									tree->GetEntry(i4);
									integratedChargeTensor[i2][i3][channel]->Fill(integratedCharge);
								}
							}
						}
					}
					std::cout << "###### Finished formating pre-analysed data..." << std::endl;
					if (canBeAnalysed == true)
					{
						std::cout << "###### Starting plotting analysis histograms..." << std::endl;
						for (int i2(0); i2 < (int)LEDV.size(); ++i2)
						{
							for (int i3(0); i3 < (int)PMTV.size(); ++i3)
							{
								for (int i4(0); i4 < 4; ++i4)
								{
									// double maximumY(0);
									double maximumX(0);
									/*for(int i5(1); i5<=integratedChargeTensor[i2][i3][i4]->GetNbinsX(); ++i5){
										if(maximumY < integratedChargeTensor[i2][i3][i4]->GetBinContent(i5)){
											maximumY = integratedChargeTensor[i2][i3][i4]->GetBinContent(i5);
											maximumX = integratedChargeTensor[i2][i3][i4]->GetXaxis()->GetBinCenter(i5);
										}
									}*/
									maximumX = integratedChargeTensor[i2][i3][i4]->GetMean();
									if (LEDV[i2] != "Dark")
									{
										std::string ledVoltage(LEDV[i2]);
										ledVoltage.erase(ledVoltage.end() - 2, ledVoltage.end());
										maxMPPCvsLEDVVec[i4]->Fill(std::stod(ledVoltage), maximumX);
										maxMPPCvsLEDVVecNorm[i4]->Fill(std::stod(ledVoltage));
									}
									if (LEDV[i2] != "Dark" && i4 == 3)
									{
										std::string pmtVoltage(PMTV[i3]);
										pmtVoltage.erase(pmtVoltage.end() - 2, pmtVoltage.end());
										maxPMTvsPMTVVec[i2]->Fill(std::stod(pmtVoltage), maximumX);
									} /*else{
										 std::cout<<"WARNING: no maximum found in the integrated charge distribution for file '"<<date+"_"+MPPCsHV[i0]+"_"+LEDV[i2]+"_"+PMTV[i3]+"_"+MPPCsNumbers+"_"+picoscopeNames[i1]<<".root'..."<<std::endl;
									 }*/
								}
							}
						}
						std::cout << "###### Finished plotting analysis histograms..." << std::endl;
						std::cout << "###### Starting saving..." << std::endl;
						for (int i2(0); i2 < (int)maxMPPCvsLEDVVec.size(); ++i2)
						{
							maxMPPCvsLEDVVec[i2]->Divide(maxMPPCvsLEDVVec[i2], maxMPPCvsLEDVVecNorm[i2], 1, 1);
							for (int i3(1); i3 <= maxMPPCvsLEDVVec[i2]->GetNbinsX(); ++i3)
							{
								maxMPPCvsLEDVVec[i2]->SetBinError(i3, 0.01 * maxMPPCvsLEDVVec[i2]->GetBinContent(i3));
							}
							TString outputDirectory(directory1);
							TString name(maxMPPCvsLEDVVec[i2]->GetName());
							TString pdfFile(name + ".pdf");
							TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
							Ctmp->SaveAs(outputDirectory + pdfFile + "[");
							saveInPDF(outputDirectory, pdfFile,
									  drawCanvas(maxMPPCvsLEDVVec[i2]->GetName(), maxMPPCvsLEDVVec[i2],
												 maxMPPCvsLEDVVec[i2]->GetName()),
									  "logNot");
							Ctmp->SaveAs(outputDirectory + pdfFile + "]");
							delete Ctmp;
						}
						THStack *maxPMTvsPMTV = new THStack();
						TString name("maxPMTvsPMTV_" + date + "_" + MPPCsHV[i0] + "_" + MPPCsNumbers + "_" + picoscopeNames[i1]);
						maxPMTvsPMTV->SetName(name);
						maxPMTvsPMTV->SetTitle(";U_{PMT} [kV];max_{PMT}");
						for (int i2(0); i2 < (int)maxPMTvsPMTVVec.size(); ++i2)
						{
							for (int i3(1); i3 <= maxPMTvsPMTVVec[i2]->GetNbinsX(); ++i3)
							{
								maxPMTvsPMTVVec[i2]->SetBinError(i3, 0.05 * maxPMTvsPMTVVec[i2]->GetBinContent(i3));
								if (maxPMTvsPMTVVec[i2]->GetBinContent(i3) != 0)
								{
									std::cout << maxPMTvsPMTVVec[i2]->GetBinContent(i3) << " / ";
								}
							}
							std::cout << std::endl;
							maxPMTvsPMTV->Add(maxPMTvsPMTVVec[i2]);
						}
						TString outputDirectory(directory1);
						TString pdfFile(name + ".pdf");
						TCanvas *Ctmp = new TCanvas("temp", "temp", 100, 100);
						Ctmp->SaveAs(outputDirectory + pdfFile + "[");
						saveInPDF(outputDirectory, pdfFile,
								  drawStackCanvas(maxPMTvsPMTV->GetName(), maxPMTvsPMTV,
												  maxPMTvsPMTV->GetName()),
								  "logNot");
						Ctmp->SaveAs(outputDirectory + pdfFile + "]");
						delete Ctmp;
						std::cout << "###### Finished saving..." << std::endl;
					}
					else
					{
						std::cout << "###### No analysis needed..." << std::endl;
					}
					endTime = clock();
					std::cout << "###### Finished analysis: " << (float)(endTime - startTime) / CLOCKS_PER_SEC << "s\n"
							  << std::endl;
				}
			}
		}
		else if (analysisSubType == "reproducibility")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "position_dependency")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "temperature_dependency")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
		}
		else if (analysisSubType == "QC")
		{
			std::cout << "Sorry this is not supported yet..." << std::endl;
			std::string datesList(argv[3]);
			std::string MPPCsNumbersList(argv[4]);
			std::string directory0(argv[5]);
			std::string directory1(argv[6]);
			std::vector<std::string> dates;
			std::vector<std::string> mppcs;
			std::vector<TH2D *> maxMPPCvsmaxPMT;
			for (int i0(0); i0 < (int)dates.size(); ++i0)
			{
				for (int i1(0); i1 < (int)MPPCsHV.size(); ++i1)
				{
					for (int i2(0); i2 < (int)LEDV.size(); ++i2)
					{
						for (int i3(0); i3 < (int)PMTV.size(); ++i3)
						{
							for (int i4(0); i4 < (int)mppcs.size(); ++i4)
							{
								for (int i5(0); i5 < (int)picoscopeNames.size(); ++i5)
								{
								}
							}
						}
					}
				}
			}
		}
		else
		{
			std::cout << "Analysis type: " << analysisSubType << " is not a valid type" << std::endl;
			std::cout << "Valid analysis types: analyse <maxPMT-maxMPPC | reproducibility"
					  << " | position_dependency | temperature_dependency | QC>" << std::endl;
		}
	}
	return 0;
}

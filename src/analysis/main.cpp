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

template <typename T>
std::vector<std::vector<T>> transpose2DVector(std::vector<std::vector<T>> arr)
{
	int outD1 = arr.at(0).size();
	int outD2 = arr.size();
	std::vector<std::vector<T>> outArr(outD1, std::vector<T>(outD2));
	
	for (int i = 0 ; i < outD1 ; i++)
	{
		for (int j = 0 ; j < outD2 ; j++)
		{
			outArr.at(i).at(j) = arr.at(j).at(i);
		}
	}
	return outArr;
}

template <typename T>
int getIndex(std::vector<T> arr, T obj)
{
	auto objPtr = std::find(arr.begin(), arr.end(), obj);
	const int objIndex = std::distance(arr.begin(), objPtr);
	return objIndex;
}

void progressBar(int current, int total, std::string label)
{
	const int barSize = 20;
	float frac = (1.0 * current) / total;
	if (frac > 1) 
	{
		std::cout << "WARNING: Progress bar above 100%" << std::endl;
		std::cout << " - CURRENT: " << current << std::endl;
		std::cout << " - TOTAL: " << total << std::endl;
		return;
	}
	std::cout << label << ": [";
	const int pos = (int) (barSize * frac);
	for (int i = 0 ; i < barSize ; i++)
	{
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] "
	<< std::string(std::to_string(total).length() - std::to_string(current).length(), '0')
	<< current << "/" << total << " (" << (int) (frac * 100.0) << "%)\r";
	std::cout.flush();
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
	return (value / 32512.0f) * VRanges[range];
}

dataHeader readHeader(std::ifstream &f)
{
	dataHeader d = {};
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

	getline(file, line); // title

	getline(file, line); // empty line
	getline(file, line); // variable names

	getline(file, line); // variable units
	// if (line != "\"\",\"\",\"�C\",\"%RH\",\"hPa\",\"\",\"\",\"\",\"\",\"\"") // XXX: idk if this will work
	// {
	// 	std::cout << "Incorrect unit line!" << std::endl;
	// 	std::cout << "file units: " << line << std::endl;
	// 	std::cout << "hardcoded : \"\",\"\",\"�C\",\"%RH\",\"hPa\",\"\",\"\",\"\",\"\",\"\"" << std::endl;
	// }

	while (getline(file, line)) // data lines
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

double chargeIntegrationFixed(const std::vector<sample> &data,
						 	  const double timebase,
						 	  const double baseline,
							  const uint32_t lowerWindow,
							  const uint32_t upperWindow)
{
	double integratedChargeRaw(0);
	int lowerEdge(std::max((uint32_t) 0,lowerWindow));
	int upperEdge(std::min((uint32_t) data.size() - 1,upperWindow));
	double totalBaseline(baseline * (upperEdge - lowerEdge + 1));

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
						   const uint32_t upperWindow,
						   const bool quickPreAnalysis)
{
	for (uint i0(0) ; i0 < dataChannel.size() ; ++i0)
	{
		gaussParams baseLineValue;
		if (quickPreAnalysis)
		{
			baseLineValue = baseLine(dataChannel.at(i0), g_quickBaselineLowerWindow, g_quickBaselineUpperWindow);
		}
		else
		{
			baseLineValue = baseLineLandau(dataChannel.at(i0), g_baselineLowerWindow, g_baselineUpperWindow);
		}
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
				getTimebase(header), lowerWindow, upperWindow, g_quickPreAnalysis || (i0 == 3));
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
	total += ampl0 * gaussDistribution(x, mu0, sigmaPe);
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

std::vector<int> findLocalMaxima(std::vector<double> &data, double height)
{
	std::vector<int> maxima;

	if (data.at(0) > data.at(1) && data.at(0) >= height) maxima.push_back(0);

	int i;
	for (i = 1 ; i < (int) data.size() - 1; i++)
	{
		// std::cout << "i: " << i;
		// if (data.at(i) > data.at(i - 1)) continue;
		// std::cout << " - 1" << std::endl;
		// if (data.at(i) > data.at(i + 1)) continue;
		// std::cout << " - 2" << std::endl;
		// if (data.at(i) >= height) continue;
		// std::cout << " - 3" << std::endl;
		if (data.at(i) > data.at(i - 1) && data.at(i) > data.at(i + 1)
				&& data.at(i) >= height) 
		maxima.push_back(i + 1);
	}

	if (data.at(i) > data.at(i - 1) && data.at(i) >= height) maxima.push_back(i);

	return maxima;
}

std::vector<int> findSeparateMaxima(std::vector<double> &data, 
		std::vector<int> &maxima, int separation)
{
	std::vector<bool> keepVec(maxima.size());
	std::vector<int> sortedIndices(maxima.size());
	std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
	std::sort(sortedIndices.begin(), sortedIndices.end(), 
			[&](int a, int b) {return data.at(a) < data.at(b);});

	for (int ind : sortedIndices)
	{
		int keep = true;
		for (int i = 0 ; i < ind ; i++)
		{
			if (std::abs(maxima.at(i) - maxima.at(ind)) < separation) keep = false;
		}
		keepVec.at(ind) = keep;
	} 

	std::vector<int> out;
	for (int i = 0 ; i < (int) keepVec.size() ; i++) 
	{
		if (keepVec.at(i)) out.push_back(maxima.at(i));
	}
	return out;
}

std::vector<int> findProminentMaxima(std::vector<double> &data, 
		std::vector<int> &maxima, double prominence)
{
	if (maxima.size() == 1) return maxima;

	std::vector<double> minVector(maxima.size() + 1);

	minVector.at(0) = DBL_MIN;
	for (int i = 1 ; i < (int) maxima.size() ; i++)
	{
		minVector.at(i) = (*std::min_element(data.begin() + maxima.at(i - 1), data.begin() + maxima.at(i)));
	}
	minVector.at(maxima.size()) = DBL_MIN;

	std::vector<int> out;

	for (int i = 0 ; i < (int) maxima.size() ; i++)
	{
		if (minVector.at(i) < prominence && minVector.at(i + 1) < prominence) out.push_back(maxima.at(i));
	}

	return out;
}

std::vector<int> findPeaks(TH1D* hist)
{
	std::vector<double> data(hist->GetNbinsX());
	for (int i = 0 ; i < hist->GetNbinsX() ; i++) data.push_back(hist->GetBinContent(i));
	std::vector<int> peaks = findLocalMaxima(data, 80);
	peaks = findSeparateMaxima(data, peaks, 30); // XXX: currently separated by bins, but maybe should change to real values?
	peaks = findProminentMaxima(data, peaks, 50);
	return peaks;
}

std::vector<double> initialGuesses(TH1D* hist)
{
	std::vector<int> peaks = findPeaks(hist);
	double binWidth = hist->GetXaxis()->GetBinWidth(0);

	if (peaks.size() < 2) return {};

	double meanDiff = (std::reduce(peaks.begin() + 1, peaks.end()) 
			- std::reduce(peaks.begin(), peaks.end() - 1)) / (peaks.size() - 1);
	double separation = meanDiff * binWidth * -1; // need it negative for our current implementation

	int index = *std::max_element(peaks.begin(), peaks.end(), [&](int a, int b) 
			{return hist->GetBinContent(a) < hist->GetBinContent(b);});
	double lambdaGuess = hist->GetBinCenter(index) / separation;

	std::vector<double> outVec = {separation, lambdaGuess};

	return outVec;
}

individualPeResult individualPeAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins, std::string title = "")
{
	TCanvas *c = new TCanvas();
	TH1D* hist = new TH1D("hist", (title + "Individual PE Signal").c_str(), nbins, xmin, xmax);

	TF1* fn = new TF1("poissGaus", pgdPtr, xmin, xmax, 9);
	fn->SetParNames("Amplitude", "\\lambda", "\\mu_{0}", "\\sigma_{PE}", "\\mu",
			"\\sigma", "n", "Amplitude_{0}", "\\sigma_{0}");
	// fn->SetParNames("Amplitude_Signal", "\\lambda", "Mean_Noise", "\\sigma_Signal", 
	// 		"Peak Separation", "\\sigma_nth Peak", "n", "Amplitude_Noise", "\\sigma_Noise");

	for (int i = 0 ; i < nWfs ; i++)
	{
		hist->Fill(data[i]);
	}

	std::vector<int> peaks = findPeaks(hist);
	std::vector<double> guesses = initialGuesses(hist);

	hist->Sumw2();

	fn->SetNpx(2 * nbins);
	individualPeResult bestRes;
	double bestChi2 = DBL_MAX;
	int bestIndex = 0;
	TH1D* bestHist = (TH1D*) hist->Clone();

	for (int i = 2 ; i < 10 ; i++)
	{
		if (guesses.size() == 0)
		{
			fn->SetParameter(1, 3);
			fn->SetParameter(4, -50);
		}
		else
		{
			fn->SetParameter(1, guesses.at(1));
			fn->SetParameter(4, guesses.at(0));
		}
		fn->SetParameter(0, nWfs);
		fn->SetParameter(2, 0);
		fn->SetParLimits(2, -20, 15);
		fn->SetParameter(3, 1);
		fn->SetParLimits(3, 0.1, 25);
		fn->SetParameter(5, 0.5);
		fn->SetParLimits(5, 0.1, 8);
		fn->FixParameter(6, i);
		fn->SetParameter(7, 500);
		fn->SetParLimits(7, 50, nWfs);
		fn->FixParameter(8, 2); //XXX: temp fix to test peak fitting
		// fn->SetParLimits(8, 0.1, 12);
		for (int j = 0 ; j < 9 ; j++) fn->SetParError(j, 0);

		TFitResultPtr res = hist->Fit(fn, "SWMQ");
		double tmpChi2 = res->Chi2();
		if (tmpChi2 < bestChi2)
		{
			bestHist->Delete();
			bestRes = {res->Parameter(0), res->ParError(0),
				   	   res->Parameter(1), res->ParError(1),
				   	   res->Parameter(2), res->ParError(2),
				   	   res->Parameter(3), res->ParError(3),
				   	   res->Parameter(4), res->ParError(4),
				   	   res->Parameter(5), res->ParError(5), 
					   i, tmpChi2};
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

	int j = 0;
	for (int i = 0 ; i < hist->GetNbinsX() ; i++)
	{
		if (i == peaks.at(j))
		{
			j++;
			if (j == (int) peaks.size()) j = 0;
		}
		else
		{
			bestHist->SetBinContent(i, 0);
		}
	}

	bestHist->Draw("same func p");

	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPoiss.pdf");

	delete hist;
	delete bestHist;
	delete c;
	delete fn;

	return bestRes;
}

highPeResult highPeAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins,
		std::string title = "")
{
	TCanvas *c = new TCanvas();
	TH1D* hist = new TH1D("hist", (title + "High PE Signal").c_str(), nbins, xmin, xmax);

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
					  res->Parameter(2), res->ParError(2),
					  res->Chi2()};

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

double skewedGaussDistribution(double x, double ampl, double loc, double scale,
		double alpha)
{
	return ampl * gaussDistribution(x, loc, scale) * 
			0.5 * (1 + TMath::Erf(alpha * (x - loc) / (scale * sqrt(2))));
}

double sgPtr(double *x, double *p)
{
	return skewedGaussDistribution(x[0], p[0], p[1], p[2], p[3]);
}

pmtResult pmtAnalysis(const double* data, const int nWfs,
		const double xmin, const double xmax, const int nbins)
{
	TCanvas *c = new TCanvas("cskewgauss", "c");
	TH1D* hist = new TH1D("hist", "PMT Signal", nbins, xmin, xmax);

	for (int i = 0 ; i < nWfs ; i++)
	{
		hist->Fill(data[i]);
	}

	hist->Sumw2();

	TF1* fn = new TF1("skewGauss", sgPtr, xmin, xmax, 4);
	fn->SetParNames("Amplitude", "Location", "Scale", "\\alpha");

	fn->SetParameter(0, nWfs);
	fn->SetParameter(1, 0);
	fn->SetParameter(2, 10);
	fn->SetParameter(3, -2);

	TFitResultPtr res = hist->Fit("skewGauss","SQ");
	// "S" returns resultptr, "Q" is to minimise printing at the end
	// XXX: Consider adding option "L", log likelihood method

	pmtResult a = {res->Parameter(0), res->ParError(0),
				   res->Parameter(1), res->ParError(1),
				   res->Parameter(2), res->ParError(2),
				   res->Parameter(3), res->ParError(3),
				   res->Chi2()};

	hist->Sumw2(kFALSE);
	hist->GetXaxis()->SetTitle("Integrated charge [mV ns]");
	hist->GetYaxis()->SetTitle("Counts");
	// hist->SetStats(0);
	hist->Draw();
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPmt.pdf");

	delete hist;
	delete c;

	return a;
}

environmentSample getSampleInterp(std::vector<environmentSample> envData, int32_t timestamp_jst)
{
	int32_t timestamp = timestamp_jst + g_jstOffset;
	environmentSample timestampEnv = {timestamp, 0.0, 0.0, 0.0};

	std::sort(envData.begin(), envData.end()); // ensure it is sorted

	environmentSample aboveSample = *std::upper_bound(envData.begin(), 
			envData.end(), timestampEnv);

	environmentSample belowSample = *std::lower_bound(envData.begin(), 
			envData.end(), timestampEnv);

	// interpolate between closest samples
	if (aboveSample == belowSample)
	{
		return aboveSample;
	}

	int32_t timestep = aboveSample.timestamp - belowSample.timestamp;
	double lowFrac = (timestamp - belowSample.timestamp) / timestep;
	double highFrac = 1 - lowFrac;

	environmentSample interpSample = {timestamp, 
		aboveSample.temperature * highFrac + belowSample.temperature * lowFrac,
		aboveSample.humidity * highFrac + belowSample.humidity * lowFrac,
		aboveSample.pressure * highFrac + belowSample.pressure * lowFrac
	};

	std::cout << "timestep: " << timestep << std::endl;
	std::cout << "lowFrac: " << lowFrac << std::endl;
	std::cout << "highFrac: " << highFrac << std::endl;
	std::cout << "timestamp: " << timestamp << std::endl;
	std::cout << "temp: " << interpSample.temperature << std::endl;
	std::cout << "humidity: " << interpSample.humidity << std::endl;
	std::cout << "press: " << interpSample.pressure << std::endl;
	std::cout << "aboveSample.time: " << aboveSample.timestamp << std::endl;
	std::cout << "belowSample.time: " << belowSample.timestamp << std::endl;

	return interpSample;
}

///////////////////////////////////////////////////////////////////////////////
///                            Saving functions                             ///
///////////////////////////////////////////////////////////////////////////////

TFitResultPtr saveGraph(std::string outputFile, std::string title, 
		std::vector<double> &x, std::vector<double> &y,
		std::string xLabel, std::string yLabel,
		std::vector<double> &uX, std::vector<double> &uY, bool linFit)
{
	std::cout << "### Creating multi-graph: " << title << std::endl;
	if (x.size() != y.size()
	|| (y.size() != uX.size() && uX.size() > 0)
	|| (y.size() != uY.size() && uY.size() > 0))
	{
		std::cout << "ERROR: Number of multi-graph entries inconsistent!" << std::endl;
		std::cout << "Size of 1st dimension of x array: " << x.size() << std::endl;
		if (uX.size())
		{
			std::cout << "Size of 1st dimension of x error array: " << uX.size() << std::endl;
		}
		std::cout << "Size of 1st dimension of y array: " << y.size() << std::endl;
		if (uY.size())
		{
			std::cout << "Size of 1st dimension of y error array: " << uY.size() << std::endl;
		}
		exit(1);
	}

	TCanvas *c = new TCanvas("gr1D", title.c_str());
	c->cd();

	double* uXPtr = uX.size() ? uX.data() : NULL;
	double* uYPtr = uY.size() ? uY.data() : NULL;

	TGraphErrors *gr = new TGraphErrors(x.size(), x.data(), y.data(), uXPtr, uYPtr);
	gr->SetLineColor(1);
	gr->SetMarkerColor(1);
	gr->SetMarkerSize(0.75);
	gr->SetMarkerStyle(8);
	gr->SetFillColor(1);
	gr->GetXaxis()->SetTitle(xLabel.c_str());
	gr->GetYaxis()->SetTitle(yLabel.c_str());
	gr->SetTitle(title.c_str());

	gStyle->SetOptFit(11);

	TFitResultPtr res;

	if (linFit) res = gr->Fit("pol1", "QS");

	gr->Draw("AP");
	c->Modified();
	c->Update();
	c->SaveAs(outputFile.c_str());
	delete gr; delete c;
	return res;
}

TFitResultPtr saveGraph(std::string outputFile, std::string title, 
		std::vector<double> &x, std::vector<double> &y,
		std::string xLabel, std::string yLabel, bool linFit)
{
	std::vector<double> uX;
	std::vector<double> uY;
	return saveGraph(outputFile, title, x, y, xLabel, yLabel, uX, uY, linFit);
}

void saveMultiGraph(std::string outputFile, std::string title,
		std::vector<std::vector<double>> &xArr, std::vector<std::vector<double>> &yArr,
		std::string xLabel, std::string yLabel, std::vector<std::string> &labels,
		std::vector<std::vector<double>> &uXArr, std::vector<std::vector<double>> &uYArr,
		TCanvas *c = NULL, TLegend *leg = NULL)
{
	std::cout << "### Creating multi-graph: " << title << std::endl;

	if ((xArr.size() != yArr.size())// && !xArr1D)
	||  (yArr.size() != labels.size() && labels.size() > 0)
	||  (yArr.size() != uXArr.size() && uXArr.size() > 0)
	||  (yArr.size() != uYArr.size() && uYArr.size() > 0))
	{
		std::cout << "ERROR: Number of multi-graph entries inconsistent!" << std::endl;
		std::cout << "Size of 1st dimension of x array: " << xArr.size() << std::endl;
		if (uXArr.size())
		{
			std::cout << "Size of 1st dimension of x error array: " << uXArr.size() << std::endl;
		}
		std::cout << "Size of 1st dimension of y array: " << yArr.size() << std::endl;
		if (uYArr.size())
		{
			std::cout << "Size of 1st dimension of y error array: " << uYArr.size() << std::endl;
		}
		std::cout << "Size of labels array: " << labels.size() << std::endl;
		exit(1);
	}
	bool deleteLeg = false; // only delete if it wasn't passed in
	bool deleteCan = false;

	if (c == NULL)
	{
		c = new TCanvas("cmg", title.c_str());
		deleteCan = true;
	}
	else
	{
		c->SetTitle(title.c_str());
	}
	c->cd();
	TMultiGraph *mg = new TMultiGraph("mg", title.c_str());
	if (leg == NULL && labels.size() > 0)
	{
		leg = new TLegend(0.13, 0.48, 0.23, 0.88);
		leg->SetFillStyle(0);
		deleteLeg = true;
	}

	for (int i = 0 ; i < (int) xArr.size() ; i++)
	{
		std::vector<double> x = xArr.at(i);
		std::vector<double> y = yArr.at(i);
		double* uX = uXArr.size() ? uXArr.at(i).data() : NULL;
		double* uY = uYArr.size() ? uYArr.at(i).data() : NULL;

		TGraphErrors *gr = new TGraphErrors(x.size(), x.data(), y.data(), uX, uY);
		gr->SetLineColor(i + 1);
		gr->SetMarkerColor(i + 1);
		gr->SetMarkerSize(0.75);
		gr->SetMarkerStyle(8);
		gr->SetFillColor(i + 1);
		mg->Add(gr, "PL");
		if (labels.size() > 0)
		{
			leg->AddEntry(gr, labels.at(i).c_str(), "lp");
		}
	}
	mg->GetXaxis()->SetTitle(xLabel.c_str());
	mg->GetYaxis()->SetTitle(yLabel.c_str());
	mg->Draw("ALP");
	if (labels.size() > 0)
	{
		leg->Draw();
	}
	c->Modified();
	c->Update();
	c->SaveAs(outputFile.c_str());

	if (deleteLeg) delete leg; 
	delete mg;
	if (deleteCan) {std::cout << "deleting c" << std::endl; delete c;}
	return;
}

void saveMultiGraph(std::string outputFile, std::string title,
		std::vector<std::vector<double>> &xArr, std::vector<std::vector<double>> &yArr,
		std::string xLabel, std::string yLabel, std::vector<std::string> labels,
		TCanvas *c = NULL, TLegend *leg = NULL)
{
	std::vector<std::vector<double>> uXArr;
	std::vector<std::vector<double>> uYArr;
	saveMultiGraph(outputFile, title, xArr, yArr, xLabel, yLabel, labels, uXArr, uYArr, c, leg);
}

///////////////////////////////////////////////////////////////////////////////
///                       Pre-Analysis mode functions                       ///
///////////////////////////////////////////////////////////////////////////////

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

				TBranch *b = forest.at(i0)->Branch(branchNameCharge.c_str(), 
							 outData + i0 * wfs, (const char *) leaflist);
				b->Fill();
			}
			free(outData);

			std::string branchNameTimestamp = combineComponents("_", {bias, "Dark", pico});

			TBranch *b = forest.at(4)->Branch(branchNameTimestamp.c_str(),
					&(header.timestamp), "unix/I");
			b->Fill();

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

	std::string branchNameTimestamp = combineComponents("_", {bias, led, pico});

	TBranch *b = forest.at(4)->Branch(branchNameTimestamp.c_str(),
			&(header.timestamp), "unix/I");
	b->Fill();

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

	// XXX: TO REMOVE OR INCORPORATE IN A BETTER WAY!!!!!!!!
	for (int i = 500; i < 676; i += 5)
	{
		ledFullVec.push_back(std::to_string(i));
		ledShortVec.push_back(std::to_string(i));
	}
	for (int i = 680; i < 726; i += 5)
	{
		ledFullVec.push_back(std::to_string(i));
	}
	g_dcp.ledFullVec = ledFullVec;
	g_dcp.ledShortVec = ledShortVec;

	// XXX: END TO REMOVE

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

	std::string filename;
	if (g_quickPreAnalysis)
	{
		filename = outputFile + "_quick.root";
	}
	else
	{
		filename = outputFile + ".root";
	}
	TFile *file = TFile::Open(filename.c_str(), "RECREATE");
	std::cout << "### Created output file: " << filename << std::endl;

	TTree *treeBack = new TTree("treeBack", mppcBack);
	TTree *treeMiddle = new TTree("treeMiddle", mppcMiddle);
	TTree *treeFront = new TTree("treeFront", mppcFront);
	TTree *treePmt = new TTree("treePmt", "Charateristics of PMT");
	TTree *treeTimestamps = new TTree("treeTimestamps", "Timestamps");
	// tree->Branch("channel", &channel, "channel/I");
	std::vector<TTree *> forest{treeBack, treeMiddle, treeFront, treePmt, treeTimestamps};

	// TODO: Header/metadata info

	file->WriteObject(&(g_dcp.biasFullVec), "biasFullVec");
	file->WriteObject(&(g_dcp.biasShortVec), "biasShortVec");
	file->WriteObject(&(g_dcp.ledFullVec), "ledFullVec");
	file->WriteObject(&(g_dcp.ledShortVec), "ledShortVec");
	file->WriteObject(&g_pmt, "pmtVoltage");
	file->WriteObject(&picoscopeNames, "picoscopeNames");
	file->WriteObject(&mppcVec, "mppcNumbers");


	// TCanvas *c = new TCanvas("ctmp");
	// c->SaveAs((g_tmpPdf + "[").c_str());
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	darkPreAnalysis(directory, date, mppcStr, forest);

	std::chrono::steady_clock::time_point endDark = std::chrono::steady_clock::now();
	int diffDark = std::chrono::duration_cast<std::chrono::seconds>(endDark-start).count();
	std::cout << "### Dark pre-analysis time: " << diffDark << "s" << std::endl;

	ledPreAnalysis(directory, date, mppcStr, forest);

	std::chrono::steady_clock::time_point endLed = std::chrono::steady_clock::now();
	int diffLed = std::chrono::duration_cast<std::chrono::seconds>(endLed-endDark).count();
	std::cout << "### Dark pre-analysis time: " << diffDark << "s" << std::endl;
	std::cout << "### LED pre-analysis time: " << diffLed << "s" << std::endl;
	std::cout << "### Total pre-analysis time: " << diffLed + diffDark << "s" << std::endl;

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
	treeTimestamps->Write();
	file->Close();

	std::cout << "### Closed output file: " << filename << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
///                         Analysis mode functions                         ///
///////////////////////////////////////////////////////////////////////////////

highPeResult singleSetGaussFitting(TTree *t, std::string bias, std::string led, 
		std::vector<std::string> pico)
{
	TTreeReader reader(t);
	std::string str1 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	std::string str2 = bias + "_" + led + "_" + pico.at(0) + "_Charge.data";
	TTreeReaderArray<Double_t> dataPico1(reader, str1.c_str());
	TTreeReaderArray<Double_t> dataPico2(reader, str2.c_str());
	reader.Next();

	// XXX: genuinely dislike myself for writing this
	std::string title = (t->GetName() + (" " + led) + "mV/" + bias + "V ").substr(4);

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

	return highPeAnalysis(fullData, n, chargeMin - padding, chargeMax + padding, 500, title);
	// XXX: global parameters for some of this should be used instead of hardcoded
}

std::vector<std::vector<std::vector<highPeResult>>> gaussFitting(
		dataCollectionParameters &dcp, std::vector<TTree*> forest,
		std::vector<std::string> pico)
{
	int numFits = forest.size() * (dcp.biasFullVec.size() * dcp.ledShortVec.size()
				  				 + dcp.biasShortVec.size() * dcp.ledFullVec.size());
	int current = 0;
	progressBar(current, numFits, "Gaussian Fitting");

	TCanvas *c = new TCanvas("cgauss");
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
				current++;
			}
			chGaussFits.push_back(biasGaussFits);
		}
		progressBar(current, numFits, "Gaussian Fitting");

		for (std::string bias : dcp.biasShortVec)
		{
			std::vector<highPeResult> biasGaussFits;
			for (std::string led : dcp.ledFullVec)
			{
				biasGaussFits.push_back(singleSetGaussFitting(t, bias, led, pico));
				current++;
			}
			chGaussFits.push_back(biasGaussFits);
		}
		gaussFits.push_back(chGaussFits);
		progressBar(current, numFits, "Gaussian Fitting");
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

	std::string title = (t->GetName() + (" " + led) + "mV/" + bias + "V ").substr(4);

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

	return individualPeAnalysis(fullData, n, chargeMin - padding, chargeMax + padding, g_nBins, title);
	// XXX: global parameters for some of this should be used instead of hardcoded
}

std::vector<std::vector<std::vector<individualPeResult>>> poissFitting(
		dataCollectionParameters &dcp, std::vector<TTree*> forest,
		std::vector<std::string> pico)
{
	int numFits = 3 //forest.size() 
			* (dcp.biasFullVec.size() * (int) std::count_if(dcp.ledShortVec.begin(), dcp.ledShortVec.end(), [](std::string s) {return std::stoi(s) <= g_highPeCutoff;})
			+ dcp.biasShortVec.size() * (int) std::count_if(dcp.ledFullVec.begin(), dcp.ledFullVec.end(), [](std::string s) {return std::stoi(s) <= g_highPeCutoff;}));
	int current = 0;
	progressBar(current, numFits, "Poissonian-Gaussian Fitting");

	std::vector<std::vector<std::vector<individualPeResult>>> poissFits;
	TCanvas *c = new TCanvas("cpoiss");
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
				current++;
			}
			chPoissFits.push_back(biasPoissFits);
			progressBar(current, numFits, "Poissonian-Gaussian Fitting");
		}

		for (std::string bias : dcp.biasShortVec)
		{
			std::vector<individualPeResult> biasPoissFits;
			for (std::string led : dcp.ledFullVec)
			{
				if (std::stoi(led) > g_highPeCutoff) continue;
				biasPoissFits.push_back(singleSetPoissFitting(t, bias, led, pico));
				current++;
			}
			chPoissFits.push_back(biasPoissFits);
			progressBar(current, numFits, "Poissonian-Gaussian Fitting");
		}
		poissFits.push_back(chPoissFits);
	}
	c->SaveAs("/home/amiles/Documents/PhD/mppc-qc/plots/tmpPoiss.pdf]");
	delete c;
	return poissFits;
}

std::vector<std::vector<int32_t>> timestampExtraction(
		dataCollectionParameters &dcp, TTree* t,
		std::vector<std::string> pico)
{
	std::vector<std::vector<int32_t>> timestamps;

	for (std::string bias : dcp.biasFullVec)
	{
		std::vector<int32_t> biasTimestamps;
		for (std::string led : dcp.ledShortVec)
		{
			std::string s1 = bias + "_" + led + "_" + pico.at(0) + ".unix";
			std::string s2 = bias + "_" + led + "_" + pico.at(1) + ".unix";
			TTreeReader reader(t);
			TTreeReaderValue<int32_t> t1(reader, s1.c_str());
			TTreeReaderValue<int32_t> t2(reader, s2.c_str());
			reader.Next();
			int32_t t = *t1 + (*t2 - *t1) / 2;
			if (t < 0) {std::cout << "timestamp is negative" << std::endl; throw "";}
			biasTimestamps.push_back(t);
		}
		timestamps.push_back(biasTimestamps);
	}

	for (std::string bias : dcp.biasShortVec)
	{
		std::vector<int32_t> biasTimestamps;
		for (std::string led : dcp.ledFullVec)
		{
			std::string s1 = bias + "_" + led + "_" + pico.at(0) + ".unix";
			std::string s2 = bias + "_" + led + "_" + pico.at(1) + ".unix";
			TTreeReader reader(t);
			TTreeReaderValue<int32_t> t1(reader, s1.c_str());
			TTreeReaderValue<int32_t> t2(reader, s2.c_str());
			reader.Next();
			int32_t t = *t1 + (*t2 - *t1) / 2;
			if (t < 0) {std::cout << "timestamp is negative" << std::endl; throw "";}
			biasTimestamps.push_back(t);
		}
		timestamps.push_back(biasTimestamps);
	}

	return timestamps;
}

fileResults genericAnalysis(std::string filePath, std::string outputDir, bool fit = true)
{
	TFile *file = TFile::Open(filePath.c_str(), "READ");

	std::vector<std::string>* biasFullVec = (std::vector<std::string>*)file->Get("biasFullVec");
	std::vector<std::string>* biasShortVec = (std::vector<std::string>*)file->Get("biasShortVec");
	std::vector<std::string>* ledFullVec = (std::vector<std::string>*)file->Get("ledFullVec");
	std::vector<std::string>* ledShortVec = (std::vector<std::string>*)file->Get("ledShortVec");
	std::vector<std::string>* picoscopeNames = (std::vector<std::string>*)file->Get("picoscopeNames");
	std::vector<std::string>* mppcNumbers = (std::vector<std::string>*)file->Get("mppcNumbers");

	std::string fileName = filePath.substr(filePath.find_last_of("/") + 1);
	std::string mppcStr = fileName.substr(0, fileName.size() - 5);
	std::cout << "\n### Beginning next file: " << fileName << std::endl;

	TTree *treeBack = (TTree*) file->Get("treeBack");
	TTree *treeMiddle = (TTree*) file->Get("treeMiddle");
	TTree *treeFront = (TTree*) file->Get("treeFront");
	TTree *treePmt = (TTree*) file->Get("treePmt");
	TTree *treeTimestamps = (TTree*) file->Get("treeTimestamps");

	std::vector<TTree*> forest = {treeBack, treeMiddle, treeFront, treePmt};

	std::vector<std::string> allBias(*biasFullVec);
	allBias.insert(allBias.end(), biasShortVec->begin(), biasShortVec->end());
	
	std::vector<double> allBiasDouble;
	for (std::string bias : allBias) allBiasDouble.push_back(std::stod(bias));

	std::vector<double> ledShortX;
	for (std::string led : *ledShortVec) ledShortX.push_back(std::stod(led));
	std::vector<double> ledFullX;
	for (std::string led : *ledFullVec) ledFullX.push_back(std::stod(led));

	// 3D vectors: num channels (4) x num bias voltages x num applicable LED voltages
	std::vector<std::vector<std::vector<highPeResult>>> gaussFits; // ALL LED V fits
	std::vector<std::vector<std::vector<individualPeResult>>> poissFits; // only low PE LED V'
	std::vector<std::vector<std::vector<darkResult>>> darkFits;
	std::vector<std::vector<int32_t>> timestamps;
	
	while (outputDir.back() == '/')
	{
		outputDir.pop_back();
	}

	std::string pdfFile = outputDir + "/" + mppcStr + ".pdf";

	dataCollectionParameters dcp = {*biasFullVec, *ledFullVec, *biasShortVec, *ledShortVec};

	ROOT::Math::MinimizerOptions* minOpt = new ROOT::Math::MinimizerOptions();
	minOpt->SetTolerance(0.001);

	gStyle->SetStatW(0.14);
	gStyle->SetStatH(0.14);
	gStyle->SetStatX(0.4);
	gStyle->SetStatY(0.85);
	gStyle->SetOptStat(0);
	gStyle->SetOptFit(1111);

	gaussFits = gaussFitting(dcp, forest, *picoscopeNames);
	std::cout << std::endl;
	std::cout << "###### Finished Gaussian Fitting" << std::endl;

	poissFits = poissFitting(dcp, forest, *picoscopeNames);
	std::cout << std::endl;
	std::cout << "###### Finished Poissonian-Gaussian Fitting" << std::endl;

	timestamps = timestampExtraction(dcp, treeTimestamps, *picoscopeNames);

	std::cout << "###### Finished Timestamp extraction" << std::endl;
	
	fileResults res = {*mppcNumbers, dcp, timestamps, gaussFits, poissFits, darkFits};

	if (!fit)
	{
		file->Close();
		delete minOpt;
		return res;
	}

	std::vector<std::vector<highPeResult>> pmtGauss = gaussFits.at(3);
	std::vector<std::vector<double>> pmtArr;
	std::vector<std::vector<double>> uPmtArr;
	std::vector<std::vector<double>> ledArr;
	std::vector<std::vector<double>> emptyArr;
	std::vector<double> emptyVec;

	for (int j = 0 ; j < (int) pmtGauss.size() ; j++)
	{
		std::string bias = allBias.at(j);
		std::vector<double> pmt; // mean pmt [mV ns]
		std::vector<double> uPmt;

		for (int k = 0 ; k < (int) pmtGauss.at(j).size() ; k++)
		{
			pmt.push_back(pmtGauss.at(j).at(k).mean * -1);
			uPmt.push_back(pmtGauss.at(j).at(k).sigma);
		}
		pmtArr.push_back(pmt);
		uPmtArr.push_back(uPmt);
		ledArr.push_back((j < (int) biasFullVec->size()) ? ledShortX : ledFullX);
	}

	std::string pmtLabel = "PMT Signal [mV ns]";
	std::string mppcLabel = "MPPC Signal [mV ns]";
	std::string mppcPeLabel = "MPPC Signal [PE]";
	std::string ledLabel = "LED Pulsed Voltage [mV]";
	std::string pmtTitle = "PMT Light Response";
	std::string peLabel = "Single PE Signal size [mV ns]";
	std::string gainLabel = "Gain";
	std::string biasLabel = "MPPC Bias Voltage [V]";
	std::string overLabel = "MPPC Overvoltage [V]";

	TCanvas *cLogY = new TCanvas();
	cLogY->SetLogy();
	TCanvas *cLogXY = new TCanvas();
	cLogXY->SetLogx();
	cLogXY->SetLogy();

	TCanvas *Ctmp = new TCanvas("temp", "temp");
	Ctmp->SaveAs((pdfFile + "[").c_str());
	// saveMultiGraph(pdfFile, pmtTitle, ledArr, pmtArr, ledLabel, pmtLabel, allBias, emptyArr, uPmtArr);
	saveMultiGraph(pdfFile, pmtTitle, ledArr, pmtArr, ledLabel, pmtLabel, allBias);
	saveMultiGraph(pdfFile, pmtTitle, ledArr, pmtArr, ledLabel, pmtLabel, allBias, cLogY);

	for (int i = 0 ; i < 3 ; i++)
	{
		std::string mppcN = mppcNumbers->at(i);
		std::string title = "MPPC " + mppcN + " Intensity Dependence";
		std::string titlePe = "MPPC " + mppcN + " PE Peak Separation";
		std::string titleGain1 = "MPPC " + mppcN + " Gain (V_{b} - V_{Br})";
		std::string titleGain2 = "MPPC " + mppcN + " Gain (SPE / q)";
		std::string titleGainRatio = "MPPC " + mppcN + " Gain (SPE / V_{b} - V_{Br})";

		std::vector<std::vector<highPeResult>> chGauss = gaussFits.at(i);
		std::vector<std::vector<individualPeResult>> chPoiss = poissFits.at(i);

		std::vector<std::vector<double>> mppcArr;
		std::vector<std::vector<double>> mppcPeArr;
		std::vector<double> peSepVec;

		for (int j = 0 ; j < (int) chGauss.size() ; j++)
		{
			std::string bias = allBias.at(j);
			std::vector<highPeResult> biasGauss = chGauss.at(j);
			std::vector<individualPeResult> biasPoiss = chPoiss.at(j);
			std::vector<double> mppc; // mean mppc [mV ns]
			std::vector<double> mppcPe; // mean mppc [PE]

			int ledIndex = getIndex((j < (int) biasFullVec->size()) 
					? *ledShortVec : *ledFullVec, g_pePlotLedV);
			
			double peSep = biasPoiss.at(ledIndex).pePeakSeparation;
			peSepVec.push_back(peSep * -1);

			for (int k = 0 ; k < (int) biasGauss.size() ; k++)
			{
				mppc.push_back(biasGauss.at(k).mean * -1);
				mppcPe.push_back(biasGauss.at(k).mean / peSep);
			}
			mppcArr.push_back(mppc);
			mppcPeArr.push_back(mppcPe);
		}

		saveMultiGraph(pdfFile, title, pmtArr, mppcArr, pmtLabel, mppcLabel, allBias);
		saveMultiGraph(pdfFile, title, pmtArr, mppcPeArr, pmtLabel, mppcPeLabel, allBias);
		saveMultiGraph(pdfFile, title + " Log XY Scale", pmtArr, mppcArr, pmtLabel, mppcLabel, allBias, cLogXY);
		saveMultiGraph(pdfFile, title + " Log XY Scale", pmtArr, mppcPeArr, pmtLabel, mppcPeLabel, allBias, cLogXY);
		saveMultiGraph(pdfFile, title, ledArr, mppcArr, ledLabel, mppcLabel, allBias);
		saveMultiGraph(pdfFile, title, ledArr, mppcPeArr, ledLabel, mppcPeLabel, allBias);
		saveMultiGraph(pdfFile, title + " Log Y Scale", ledArr, mppcArr, ledLabel, mppcLabel, allBias, cLogY);
		saveMultiGraph(pdfFile, title + " Log Y Scale", ledArr, mppcPeArr, ledLabel, mppcPeLabel, allBias, cLogY);
		TFitResultPtr res = saveGraph(pdfFile, titlePe, allBiasDouble, peSepVec, biasLabel, peLabel, true);
		double vBr = -1 * res->Parameter(0) / res->Parameter(1);
		std::cout << vBr << std::endl;
		
		std::vector<double> gainVec1;
		std::vector<double> gainVec2;
		std::vector<double> gainRatioVec;
		std::vector<double> allOverV;
		for (int j = 0 ; j < (int) allBiasDouble.size() ; j++)
		{
			double overV = (allBiasDouble.at(j) - vBr) / 2;
			allOverV.push_back(overV);
			gainVec1.push_back(g_terminalCap * overV
					/ (g_elementaryCharge * g_pixelsPerCh)); // shouldn't be here: g_ampGain * g_splitter * 
			gainVec2.push_back(peSepVec.at(j) * 1e-12
					/ (50 * g_ampGain * g_splitter * g_elementaryCharge));
			gainRatioVec.push_back(peSepVec.at(j) * 1e-12 * g_pixelsPerCh
					/ (g_terminalCap * overV * 50 * g_ampGain * g_splitter));
		}
		saveGraph(pdfFile, titleGain1, allBiasDouble, gainVec1, biasLabel, gainLabel, true);
		saveGraph(pdfFile, titleGain1, allOverV, gainVec1, overLabel, gainLabel, true);
		saveGraph(pdfFile, titleGain2, allBiasDouble, gainVec2, biasLabel, gainLabel, true);
		saveGraph(pdfFile, titleGain2, allOverV, gainVec2, overLabel, gainLabel, true);
		saveGraph(pdfFile, titleGainRatio, allOverV, gainRatioVec, overLabel, gainLabel, true);
	}
	Ctmp->SaveAs((pdfFile + "]").c_str());
	// std::cout << g_maxPeaks << std::endl;
	delete Ctmp; delete minOpt;

	delete cLogY; delete cLogXY;

	file->Close();
	return res;
}

void rotationAnalysis(std::string outputDir, std::string dataFile1, std::string dataFile2)
{
	// rotation looks at differences in top and bottom MPPC channels, since our generic analysis
	// combines the two picoscope readouts, it is more difficult to utilise like this
	// and needs to be reimplemented

}

void cycleAnalysis(std::string file1, std::string file2, std::string file3, 
		std::string outputDir)
{
	// We assume file 1 is a-b-c, file 2 c-a-b, file 3 is b-c-a

	fileResults r1 = genericAnalysis(file1, outputDir, false);
	fileResults r2 = genericAnalysis(file2, outputDir, false);
	fileResults r3 = genericAnalysis(file3, outputDir, false);

	while (outputDir.back() == '/')
	{
		outputDir.pop_back();
	}

	std::string pdfFile = outputDir + "/" + 
			combineComponents("-", r1.mppcNumbers) + "_CycleAnalysis.pdf";

	std::vector<std::string> allBias(r1.dcp.biasFullVec);
	allBias.insert(allBias.end(), r1.dcp.biasShortVec.begin(), r1.dcp.biasShortVec.end());

	std::string xLabel = "PMT Signal [mV ns]";
	std::string yLabelA = "Further signal / Centre signal";
	std::string yLabelC = "Closer signal / Centre signal";

	TCanvas *Ctmp = new TCanvas("ctmp");
	Ctmp->SaveAs((pdfFile + "[").c_str());
	std::cout << "### File " << pdfFile << " has been opened" << std::endl;

	for (int i = 0 ; i < 3 ; i++)
	{
		std::string mppcNumber = r1.mppcNumbers.at(i);
		int mppcIndex1 = i;
		int mppcIndex2;
		int mppcIndex3;
		std::vector<std::string>::iterator checkR2 = std::find(r2.mppcNumbers.begin(), r2.mppcNumbers.end(), mppcNumber);
		std::vector<std::string>::iterator checkR3 = std::find(r3.mppcNumbers.begin(), r3.mppcNumbers.end(), mppcNumber);

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

		std::vector<std::vector<highPeResult>> chGaussA = fVec.at(0).gaussFits.at(0);
		std::vector<std::vector<highPeResult>> chGaussB = fVec.at(1).gaussFits.at(1);
		std::vector<std::vector<highPeResult>> chGaussC = fVec.at(2).gaussFits.at(2);

		std::vector<std::vector<highPeResult>> chPmtA = fVec.at(0).gaussFits.at(3);
		std::vector<std::vector<highPeResult>> chPmtB = fVec.at(1).gaussFits.at(3);
		std::vector<std::vector<highPeResult>> chPmtC = fVec.at(2).gaussFits.at(3);
		// std::vector<std::vector<individualPeResult>> chPoiss = poissFits.at(i);

		std::vector<std::vector<double>> xArr;
		std::vector<std::vector<double>> yArrA;
		std::vector<std::vector<double>> yArrC;

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
				yA.push_back(biasGaussA.at(k).mean / biasGaussB.at(k).mean * 
						chPmtB.at(j).at(k).mean / chPmtA.at(j).at(k).mean);
				yC.push_back(biasGaussC.at(k).mean / biasGaussB.at(k).mean * 
						chPmtB.at(j).at(k).mean / chPmtC.at(j).at(k).mean);
				x.push_back(chPmtB.at(j).at(k).mean * -1);
			}

			xArr.push_back(x);
			yArrA.push_back(yA);
			yArrC.push_back(yC);
		}
		saveMultiGraph(pdfFile, titleA, xArr, yArrA, xLabel, yLabelA, allBias);
		saveMultiGraph(pdfFile, titleC, xArr, yArrC, xLabel, yLabelC, allBias);
	}
	Ctmp->SaveAs((pdfFile + "]").c_str());
	std::cout << "### File " << pdfFile << " has been closed" << std::endl;
	delete Ctmp;
	return;
}

void tempAnalysis(std::string outputDir, std::string envDataFile,
		std::vector<std::string> dataFiles)
{
	std::vector<environmentSample> envData = readEnvData(envDataFile);

	std::vector<fileResults> resVec;

	for (std::string data : dataFiles)
	{
		fileResults fr = genericAnalysis(data, outputDir, false);
		resVec.push_back(fr);
	}


	std::vector<std::string> allBias(resVec.at(0).dcp.biasFullVec);
	allBias.insert(allBias.end(), resVec.at(0).dcp.biasShortVec.begin(), resVec.at(0).dcp.biasShortVec.end());

	const int biasIndex = getIndex(allBias, std::string("81"));
	std::vector<std::string> labels = biasIndex < (int) resVec.at(0).dcp.biasFullVec.size() ?
			resVec.at(0).dcp.ledShortVec : resVec.at(0).dcp.ledFullVec;

	while (outputDir.back() == '/')
	{
		outputDir.pop_back();
	}

	std::string pmtTitle = "PMT Temperature Response";
	std::string pmtAxis = "PMT Signal [mV ns]";
	std::string tempAxis = "Temperature [#circ C]";
	std::string mppcAxis = "MPPC Signal [mV ns]";

	std::string pdfFile = outputDir + "/1-2-3_Temperature.pdf";

	std::vector<std::vector<double>> pmtArrTransposed;
	std::vector<std::vector<double>> uPmtArrTransposed;
	std::vector<std::vector<double>> tempArrTransposed;
	for (int j = 0 ; j < (int) resVec.size() ; j++)
	{
		fileResults res = resVec.at(j);
		std::vector<highPeResult> fPmt = res.gaussFits.at(3).at(biasIndex);
		std::vector<int32_t> fTimestamps = res.timestamps.at(biasIndex);
		std::vector<double> biasPmt;
		std::vector<double> uBiasPmt;
		std::vector<double> biasTemp;

		for (int k = 0 ; k < (int) fPmt.size() ; k++)
		{
			biasPmt.push_back(fPmt.at(k).mean * -1);
			uBiasPmt.push_back(fPmt.at(k).sigma);
			environmentSample env = getSampleInterp(envData, fTimestamps.at(k));
			biasTemp.push_back(env.temperature);
		}
		pmtArrTransposed.push_back(biasPmt);
		uPmtArrTransposed.push_back(uBiasPmt);
		tempArrTransposed.push_back(biasTemp);
	}
	std::vector<std::vector<double>> pmtArr = transpose2DVector(pmtArrTransposed);
	std::vector<std::vector<double>> uPmtArr = transpose2DVector(uPmtArrTransposed);
	std::vector<std::vector<double>> tempArr = transpose2DVector(tempArrTransposed);
	std::vector<std::vector<double>> uTempArr;

	TCanvas* Ctmp = new TCanvas("ctmp");
	Ctmp->SaveAs((pdfFile + "[").c_str());

	saveMultiGraph(pdfFile, pmtTitle, tempArr, pmtArr, tempAxis, pmtAxis, labels, uTempArr, uPmtArr);

	for (int i = 0 ; i < 3 ; i++)
	{
		std::string mppcN = resVec.at(0).mppcNumbers.at(i);
		std::string title = "MPPC " + mppcN + " Temperature Dependence";

		std::vector<std::vector<double>> mppcArrTransposed;
		std::vector<std::vector<double>> uMppcArrTransposed;

		for (int j = 0 ; j < (int) resVec.size() ; j++)
		{
			fileResults res = resVec.at(j);

			std::vector<highPeResult> fGauss = res.gaussFits.at(i).at(biasIndex);
			std::vector<individualPeResult> fPoiss = res.poissFits.at(i).at(biasIndex);

			std::string bias = allBias.at(biasIndex);
			std::vector<double> mppc;
			std::vector<double> uMppc;
			
			for (int k = 0 ; k < (int) fGauss.size() ; k++)
			{
				mppc.push_back(fGauss.at(k).mean * -1);
				uMppc.push_back(fGauss.at(k).sigma);
			}
			mppcArrTransposed.push_back(mppc);
			uMppcArrTransposed.push_back(uMppc);
		}
		std::vector<std::vector<double>> mppcArr = transpose2DVector(mppcArrTransposed);
		std::vector<std::vector<double>> uMppcArr = transpose2DVector(uMppcArrTransposed);
		saveMultiGraph(pdfFile, title, tempArr, mppcArr, tempAxis, mppcAxis, labels, uTempArr, uMppcArr);
	}

	Ctmp->SaveAs((pdfFile + "]").c_str());
	delete Ctmp;
	return;
}

void reproducibilityAnalysis(std::string outputDir, std::vector<std::string> dataFiles)
{
	// Designed to work for any number of data files, will compare the signal
	// intensity of each channel to the other files. Similar to channel dependence
	// but for same channels

	if (dataFiles.size() < 2)
	{
		std::cout << "ERROR: Can't measure reproducibility with one data file" << std::endl;
		exit(1);
	}

	std::vector<fileResults> resVec;
	std::vector<std::string> fileEndings;

	for (std::string data : dataFiles)
	{
		fileResults fr = genericAnalysis(data, outputDir, false);
		resVec.push_back(fr);

		std::string basename = data.substr(data.find_last_of("/") + 1);
		if (basename.find('_') != std::string::npos)
		{
			fileEndings.push_back(basename.substr(0, basename.find_first_of('.')));
		}
		else
		{
			fileEndings.push_back("");
		}
	}

	fileResults r0 = resVec.at(0);

	std::vector<std::string> allBias(r0.dcp.biasFullVec);
	allBias.insert(allBias.end(), r0.dcp.biasShortVec.begin(), r0.dcp.biasShortVec.end());

	std::string pmtAxis = "PMT Signal [mV ns]";
	std::string mppcAxis = "MPPC Signal Ratio";

	std::vector<std::vector<double>> refPmtArr;
	for (int j = 0 ; j < (int) r0.gaussFits.at(3).size() ; j++)
	{
		std::vector<highPeResult> pmtGauss = r0.gaussFits.at(3).at(j);
		std::vector<double> refPmt;

		for (int k = 0 ; k < (int) pmtGauss.size() ; k++)
		{
			refPmt.push_back(pmtGauss.at(k).mean * -1);
		}
		refPmtArr.push_back(refPmt);
	}

	while (outputDir.back() == '/')
	{
		outputDir.pop_back();
	}
	std::string pdfFile = outputDir + "/" + 
			combineComponents("-", r0.mppcNumbers) + "_Reproducibility.pdf";

	TCanvas* Ctmp = new TCanvas("ctmp");
	Ctmp->SaveAs((pdfFile + "[").c_str());

	for (int i = 0 ; i < 3 ; i++)
	{
		std::string mppcN = r0.mppcNumbers.at(i);

		std::vector<std::vector<double>> refMppcArr;

		for (int j = 0 ; j < (int) r0.gaussFits.at(i).size() ; j++)
		{
			std::vector<highPeResult> mppcGauss = r0.gaussFits.at(i).at(j);
			std::vector<double> refMppc;

			for (int k = 0 ; k < (int) mppcGauss.size() ; k++)
			{
				refMppc.push_back(mppcGauss.at(k).mean * -1);
			}
			refMppcArr.push_back(refMppc);
		}

		for (int j = 1 ; j < (int) resVec.size() ; j++)
		{
			fileResults res = resVec.at(j);

			std::string title = "MPPC " + mppcN + " " + fileEndings.at(j)
					+ " Reproducibility";

			std::vector<std::vector<double>> mppcArr;
			std::vector<std::vector<highPeResult>> chGauss = res.gaussFits.at(i);
			std::vector<std::vector<highPeResult>> chPmt = res.gaussFits.at(3);

			for (int k = 0 ; k < (int) chGauss.size() ; k++)
			{
				std::vector<highPeResult> biasGauss = chGauss.at(k);
				std::vector<highPeResult> biasPmt = chPmt.at(k);
				std::vector<double> mppc;

				for (int l = 0 ; l < (int) biasGauss.size() ; l++)
				{
					mppc.push_back(biasGauss.at(l).mean / refMppcArr.at(k).at(l)
							* (biasPmt.at(l).mean / refPmtArr.at(k).at(l)));
				}
				mppcArr.push_back(mppc);
			}
			saveMultiGraph(pdfFile, title, refPmtArr, mppcArr, pmtAxis, mppcAxis, allBias);
		}
	}
	Ctmp->SaveAs((pdfFile + "]").c_str());
}

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
	ROOT::EnableImplicitMT(2);
	col->SetRGB(0.5,0.5,0.5);
	gErrorIgnoreLevel = 1001;
	if (argc < 2)
	{
		std::cerr << "ERROR: you need at least 1 argument: 'pre-analyse' or 'analyse'..." << std::endl;
		return 1;
	}
	std::string analysisType(argv[1]);
	if (analysisType == "pre-analyse")
	{
		if (argc == 6 && std::string(argv[5]) == std::string("-f"))
		{
			g_quickPreAnalysis = true;
		}
		else if (argc != 5)
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
		if (argc != 4)
		{
			std::cerr << "ERROR: you should have 4 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string inputFile(argv[3]);
		genericAnalysis(inputFile, outputDir);
	}
	else if (analysisType == "cycle-analyse")
	{
		if (argc != 6)
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
	else if (analysisType == "temperature-analyse")
	{
		if (argc < 6)
		{
			std::cerr << "ERROR: you should have at least 6 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string envDataFile(argv[3]);
		std::vector<std::string> dataFiles(argv + 4, argv + argc);
		tempAnalysis(outputDir, envDataFile, dataFiles);
	}
	else if (analysisType == "reproducibility-analyse")
	{
		if (argc < 5)
		{
			std::cerr << "ERROR: you should have at least 4 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::vector<std::string> dataFiles(argv + 3, argv + argc);
		reproducibilityAnalysis(outputDir, dataFiles); 
	}
	else if (analysisType == "rotation-analyse")
	{
		if (argc != 5)
		{
			std::cerr << "ERROR: you should have 4 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string dataFile1(argv[3]);
		std::string dataFile2(argv[4]);
		rotationAnalysis(outputDir, dataFile1, dataFile2);
	}
	else if (analysisType == "check-hist")
	{
		if (argc != 5)
		{
			std::cerr << "ERROR: you should have 4 parameters, please look in 'launch_analysis.sh'..." << std::endl;
			return 1;
		}
		std::string outputDir(argv[2]);
		std::string inputFile(argv[3]);
		std::string mppcNum(argv[4]); // The mppc number you want to check
		std::string bias(argv[5]);
		std::string led(argv[6]);
		std::cout << "Still unimplemented" << std::endl;
	}
	return 0;
}

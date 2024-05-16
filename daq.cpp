#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <bitset>
#include <vector>
#include <assert.h>


#ifndef PS6000API
#include <libps6000/ps6000Api.h>
#endif

#ifndef PICO_STATUS
#include <libps6000/PicoStatus.h>
#endif

#ifndef PS6000WRAPPER
#include "libps6000/ps6000Wrapper.h"
#endif

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

using namespace std;
namespace py = pybind11;
// constexpr auto byref = py::return_value_policy::reference_internal;

// typedef enum enBOOL{FALSE,TRUE} BOOL;

uint8_t N;
uint32_t numberWaveforms;
uint32_t samplesPreTrigger;

uint32_t	timebase = 8;
int16_t		oversample = 1;
int32_t      scaleVoltages = TRUE;

class dataCollectionConfig 
{
public:
    ofstream ostream;
    UNIT *unit;
    bitset<4> activeChannels;
    bitset<5> activeTriggers;
    bitset<4> timebase;
    bitset<16> chVoltageRanges;
    vector<int16_t> chTriggerThresholdADC;
    int16_t auxTriggerThresholdADC;
    vector<uint16_t> chPostSamplesPerWaveform; // NOTE: Excludes pre trigger samples
    uint16_t maxPostSamples;
    uint16_t samplesPreTrigger;
    uint32_t numWaveforms;
    vector<vector<int16_t*>> dataBuffers;
    char serial[16];
    dataCollectionConfig(UNIT *unit, char *serial)
    {
        this->unit = unit;
        strcpy(this->serial, serial);
    }
};

void setActiveChannels(dataCollectionConfig &dcc, 
                       int16_t aChVoltage,
                       int16_t bChVoltage,
                       int16_t cChVoltage,
                       int16_t dChVoltage)
{
    int16_t chVoltage[4] = {aChVoltage, bChVoltage, cChVoltage, dChVoltage};

    for (int i = 0; i < 4; i++)
    {
        if (chVoltage[i] != 99) 
        {
            dcc.activeChannels.set(i);
        }
    }

    cout << "after setting dcc.active" << endl;

    SetVoltages(dcc.unit, chVoltage);

    cout << "passed setvoltages" << endl;
    return;
}

void setTriggerConfig(dataCollectionConfig &dcc, 
                      int16_t aTrigVoltageMv,
                      int16_t bTrigVoltageMv,
                      int16_t cTrigVoltageMv,
                      int16_t dTrigVoltageMv,
                      int16_t auxTrigVoltageMv)
{

    int16_t trigVoltageMv[5] = {aTrigVoltageMv,
                                bTrigVoltageMv,
                                cTrigVoltageMv,
                                dTrigVoltageMv,
                                auxTrigVoltageMv
                                };

    for (int i = 0; i < 4; i++)
    {
        if (trigVoltageMv[i] != 0 && dcc.activeChannels.test(i))
        {
            dcc.activeTriggers.set(i + 1);
            dcc.chTriggerThresholdADC.push_back(mv_to_adc(trigVoltageMv[i], 
            dcc.unit->channelSettings[PS6000_CHANNEL_A + i].range));
        }
        else
        {
            dcc.chTriggerThresholdADC.push_back(0);
        }
    }
    if (trigVoltageMv[4] != 0)
    {
        dcc.activeTriggers.set(0);
        dcc.auxTriggerThresholdADC = mv_to_adc(trigVoltageMv[4], 
        dcc.unit->channelSettings[PS6000_TRIGGER_AUX].range);
    }
    else
    {
        dcc.auxTriggerThresholdADC = 0;
    }

    SetTriggers(dcc.unit, dcc.activeTriggers, dcc.chTriggerThresholdADC, dcc.auxTriggerThresholdADC);

    return;
}

void setDataConfig(dataCollectionConfig &dcc, uint8_t *timebase,
    uint32_t *numWaveforms, uint16_t *samplesPreTrigger, uint16_t *chAWfSamples,
    uint16_t *chBWfSamples , uint16_t *chCWfSamples , uint16_t *chDWfSamples)
{
    dcc.timebase = *timebase;
    dcc.numWaveforms = *numWaveforms;
    dcc.samplesPreTrigger = *samplesPreTrigger;

    vector<uint16_t> samplesPerWaveform = {*chAWfSamples, *chBWfSamples, *chCWfSamples, *chDWfSamples};
    dcc.chPostSamplesPerWaveform = samplesPerWaveform;
    dcc.maxPostSamples = *max_element(  dcc.chPostSamplesPerWaveform.begin(),
                                        dcc.chPostSamplesPerWaveform.end());

    dcc.dataBuffers = SetDataBuffers(dcc.unit, dcc.activeChannels, samplesPerWaveform, 
            *samplesPreTrigger, *numWaveforms, dcc.maxPostSamples);

    return;
}

void collectRapidBlockData(dataCollectionConfig &dcc)
{
    uint16_t maxPostTrigger = *max_element( dcc.chPostSamplesPerWaveform.begin(),
                                            dcc.chPostSamplesPerWaveform.end());
    StartRapidBlock(dcc.unit, dcc.samplesPreTrigger, maxPostTrigger, 
            dcc.timebase.to_ulong(), dcc.numWaveforms);
    return;
}

void setDataOutput(dataCollectionConfig &dcc, char *outputFileName)
{
    dcc.ostream.open(outputFileName, ios::out | ios::binary);
    return;
}

bool isLittleEndian()
{
    uint32_t i = 1;
    char *c = (char*)&i;
    return bool(*c);
}

int16_t bswap16(int16_t n)
{
    if (isLittleEndian()) {return __builtin_bswap16(n);}
    else {return n;}
}

uint16_t bswapu16(uint16_t n)
{
    if (isLittleEndian()) {return __builtin_bswap16(n);}
    else {return n;}
}

int32_t bswap32(int32_t n)
{
    if (isLittleEndian()) {return __builtin_bswap32(n);}
    else {return n;}
}

uint32_t bswapu32(uint32_t n)
{
    if (isLittleEndian()) {return __builtin_bswap32(n);}
    else {return n;}
}

template<std::size_t N>
void bitset_reverse(std::bitset<N> &b) 
{
    for(std::size_t i = 0; i < N/2; ++i) 
    {
        bool t = b[i];
        b[i] = b[N-i-1];
        b[N-i-1] = t;
    }
}


void writeDataHeader(dataCollectionConfig &dcc)
{
    // Bit layout, in order
    // 4 bits: timebase (from 0-4 for ps6000)
    // 4 bits: ch1-4 active
    // 3 bits: padding
    // 5 bits: ch1-4, aux trigger active
    // 8 bits: aux trigger threshold
    // 64 (16*4) bits: trigger threshold (ch1-4)
    // 16 (4*4): ch1-4 voltage ranges (aux is always +-1V range)
    // 64 (4*16) bits: number of TOTAL samples per waveform (including pretrigger)
    // 16 bits: number of samples before trigger
    // 32 bits: number of waveforms
    // total above bits: 216 (27 bytes)
    // 8 bytes: model string
    // 8 bytes (16 chars): serial number?
    // MODEL STRING AND SERIAL AREN'T ZERO PADDED, NEED TO ENSURE 
    // DATA DISTANCE IS CONSISTENT AND ADD THAT. OR GO 0 TERMINATED IN PY SIDE

    int16_t o16;
    uint16_t ou16;
    int32_t o32;
    uint32_t ou32;

    bitset_reverse(dcc.activeChannels);
    bitset_reverse(dcc.activeTriggers);

    uint8_t timebaseActiveCh =  (uint8_t) dcc.timebase.to_ullong() << 4 |
                                (uint8_t) dcc.activeChannels.to_ullong();
    dcc.ostream.write((const char *) &timebaseActiveCh, sizeof(uint8_t));

    uint8_t activeTriggers = (uint8_t) dcc.activeTriggers.to_ullong();
    dcc.ostream.write((const char *) &activeTriggers, sizeof(uint8_t));

    o16 = bswap16(dcc.auxTriggerThresholdADC);
    dcc.ostream.write((const char *) &o16, sizeof(int16_t));

    for (int i = 0; i < 4; i++)
    {
        o16 = bswap16(dcc.chTriggerThresholdADC.at(i));
        dcc.ostream.write((const char *) &o16, sizeof(int16_t));
    }

    o16 = bswap16((int16_t) dcc.chVoltageRanges.to_ullong());
    dcc.ostream.write((const char *) &o16, sizeof(int16_t));

    for (int i = 0; i < 4; i++)
    {
        o16 = bswap16(dcc.chPostSamplesPerWaveform.at(i) + dcc.samplesPreTrigger);
        dcc.ostream.write((const char *) &o16, sizeof(int16_t));
    }

    ou16 = bswapu16(dcc.samplesPreTrigger);
    dcc.ostream.write((const char *) &ou16, sizeof(uint16_t));

    o32 = bswap32(dcc.numWaveforms);
    dcc.ostream.write((const char *) &o32, sizeof(int32_t));

    dcc.ostream.write((const char *) &dcc.unit->modelString, sizeof(dcc.unit->modelString));

    dcc.ostream.write((const char *) &dcc.serial, sizeof(&dcc.serial));

    return;
}

void writeDataOut(dataCollectionConfig &dcc)
{
    uint16_t maxSamples = *max_element( dcc.chPostSamplesPerWaveform.begin(), 
                                        dcc.chPostSamplesPerWaveform.end());
    
    for (int i = 0; i < dcc.activeChannels.count(); i++)
    {
        uint64_t bufferSize = (dcc.chPostSamplesPerWaveform.at(i) 
                             + dcc.samplesPreTrigger) * sizeof(int16_t);
        for (int j = 0; j < dcc.numWaveforms; j++)
        {
            dcc.ostream.write((const char*)dcc.dataBuffers.at(i).at(j), bufferSize);
            free(dcc.dataBuffers.at(i).at(j));
        }
    }
    dcc.ostream.close();
}

// to be run from python side
int runDAQ(char *outputFile,
            int16_t chATrigger, int16_t chAVRange, uint16_t chAWfSamples,
            int16_t chBTrigger, int16_t chBVRange, uint16_t chBWfSamples,
            int16_t chCTrigger, int16_t chCVRange, uint16_t chCWfSamples,
            int16_t chDTrigger, int16_t chDVRange, uint16_t chDWfSamples,
            int16_t auxTrigger, uint8_t timebase,
            uint32_t numWaveforms, uint16_t samplesPreTrigger, char *serial)
{
    UNIT *unit;
    if (serial == "") {serial = NULL;}
    findUnit(unit, (int8_t*) serial);
    try
    {
        dataCollectionConfig dcc(unit, serial);
        setActiveChannels(dcc, chAVRange, chBVRange, chCVRange, chDVRange);
        setTriggerConfig(dcc, chATrigger, chBTrigger, chCTrigger, chDTrigger, auxTrigger);
        setDataConfig(dcc, &timebase, &numWaveforms, &samplesPreTrigger, 
                    &chAWfSamples, &chBWfSamples, &chCWfSamples, &chDWfSamples);

        collectRapidBlockData(dcc);
        setDataOutput(dcc, outputFile);
        writeDataHeader(dcc);
        writeDataOut(dcc);
        printf("wrote data\n");
        CloseDevice(unit);
        printf("end\n");
    }
    catch (exception e)
    {
        printf("Final Catch\n");
        cout << "Caught: " << e.what() << endl;
        CloseDevice(unit);
        throw e;
    }
    return 1;
}

char* getSerials()
{
    int16_t *serialLth;
    *serialLth = 128;
    char* out = new char[*serialLth];
    int16_t *count;

    enumUnits(count, out, serialLth);

    return out;
}

PYBIND11_MODULE(daq, m)
{
    m.doc() = "Picoscope DAQ System";

    m.def("runDAQ", &runDAQ, py::return_value_policy::copy); //, py::arg("outputFile"), 
    // py::arg("chATrigger"), py::arg("chAVRange"), py::arg("chAWfSamples"),
    // py::arg("chBTrigger"), py::arg("chBVRange"), py::arg("chBWfSamples"),
    // py::arg("chCTrigger"), py::arg("chCVRange"), py::arg("chCWfSamples"),
    // py::arg("chDTrigger"), py::arg("chDVRange"), py::arg("chDWfSamples"), 
    // py::arg("auxTrigger"), py::arg("timebase"), py::arg("numWaveforms"), 
    // py::arg("samplesPreTrigger"), py::arg("serials"));
    m.def("getSerials", &getSerials, py::return_value_policy::copy);
}


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
    vector<int8_t> chTriggerThresholdADC;
    int8_t auxTriggerThresholdADC;
    vector<uint16_t> chPostSamplesPerWaveform; // NOTE: Excludes pre trigger samples
    uint16_t samplesPreTrigger;
    uint32_t numWaveforms;
    vector<vector<int16_t*>> dataBuffers;
    dataCollectionConfig(UNIT *unit)
    {
        this->unit = unit;
    }
};


void openFile(const char * b, const char * c)
{
    ofstream outputFile;
    outputFile.open(b, ios::in | ios::binary);
    return;
}

void setActiveChannels(dataCollectionConfig &dcc, 
                       uint16_t *aChVoltage,
                       uint16_t *bChVoltage,
                       uint16_t *cChVoltage,
                       uint16_t *dChVoltage)
{
    bitset<4> activeChannels;
    uint16_t chVoltage[4];
    if (*aChVoltage != 99) 
    {
        chVoltage[0] = *aChVoltage;
        activeChannels.set(3);
    }
    if (*bChVoltage != 99) 
    {
        chVoltage[1] = *bChVoltage;
        activeChannels.set(2);
    }
    if (*cChVoltage != 99) 
    {
        chVoltage[2] = *cChVoltage;
        activeChannels.set(1);
    }
    if (*dChVoltage != 99) 
    {
        chVoltage[3] = *dChVoltage;
        activeChannels.set(0);
    }
    dcc.activeChannels = activeChannels;

    SetVoltages(dcc.unit, chVoltage);
    return;
}

void setTriggerConfig(dataCollectionConfig &dcc, 
                      int16_t *aTrigVoltageMv,
                      int16_t *bTrigVoltageMv,
                      int16_t *cTrigVoltageMv,
                      int16_t *dTrigVoltageMv,
                      int16_t *auxTrigVoltageMv)
{
    bitset<5> activeTriggers;
    if ((  aTrigVoltageMv != NULL) && (dcc.activeChannels[3] == 1)) 
    {
        activeTriggers.set(4);
        dcc.chTriggerThresholdADC.at(0) = mv_to_adc(*aTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_A].range);
    }

    if ((  bTrigVoltageMv != NULL) && (dcc.activeChannels[2] == 1)) 
    {
        activeTriggers.set(3);
        dcc.chTriggerThresholdADC.at(1) = mv_to_adc(*bTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_B].range);
    }

    if ((  cTrigVoltageMv != NULL) && (dcc.activeChannels[1] == 1)) 
    {
        activeTriggers.set(2);
        dcc.chTriggerThresholdADC.at(2) = mv_to_adc(*cTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_C].range);
    }

    if ((  dTrigVoltageMv != NULL) && (dcc.activeChannels[0] == 1)) 
    {
        activeTriggers.set(1);
        dcc.chTriggerThresholdADC.at(3) = mv_to_adc(*dTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_D].range);
    }

    if (auxTrigVoltageMv != NULL) 
    {
        activeTriggers.set(0);
        dcc.auxTriggerThresholdADC = mv_to_adc(*dTrigVoltageMv, 
            dcc.unit->channelSettings[PS6000_CHANNEL_D].range);
    }
    dcc.activeTriggers = activeTriggers;

    SetTriggers(dcc.unit, activeTriggers, dcc.chTriggerThresholdADC, dcc.auxTriggerThresholdADC);

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

    SetNumWaveforms(dcc.unit, *numWaveforms);
    dcc.dataBuffers = SetDataBuffers(dcc.unit, dcc.activeChannels, samplesPerWaveform, 
            *samplesPreTrigger, *numWaveforms);

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
    dcc.ostream.open(outputFileName, ofstream::out | ofstream::binary);
    return;
}

void writeDataHeader(dataCollectionConfig &dcc)
{
    // Bit layout, in order
    // 4 bits: timebase (from 0-4 for ps6000)
    // 4 bits: ch1-4 active
    // 3 bits: padding
    // 5 bits: ch1-4, aux trigger active
    // 8 bits: aux trigger threshold
    // 32 (8*4) bits: trigger threshold (ch1-4)
    // 16 (4*4): ch1-4 voltage ranges (aux is always +-1V range)
    // 64 (4*16) bits: number of TOTAL samples per waveform (including pretrigger)
    // 16 bits: number of samples before trigger
    // 32 bits: number of waveforms
    // total above bits: 184 (23 bytes)
    // 8 bytes: model string
    // 16 bytes (16 chars): serial number?
    // 

    uint8_t timebaseActiveCh =  dcc.timebase.to_ullong() << 4 |
                                dcc.activeChannels.to_ullong();
    dcc.ostream.write((const char *) &timebaseActiveCh, sizeof(uint8_t));

    uint8_t activeTriggers = (uint8_t) dcc.activeTriggers.to_ullong();
    dcc.ostream.write((const char *) &activeTriggers, sizeof(uint8_t));

    dcc.ostream.write((const char *) &dcc.auxTriggerThresholdADC, sizeof(uint8_t));

    for (int i = 0; i < 4; i++)
    {
        dcc.ostream.write((const char *) &dcc.chTriggerThresholdADC.at(i), sizeof(int8_t));
    }

    uint16_t voltRanges = (uint16_t) dcc.chVoltageRanges.to_ullong();
    dcc.ostream.write((const char *) &voltRanges, sizeof(uint16_t));

    for (int i = 0; i < 4; i++)
    {
        uint16_t nSamples = dcc.chPostSamplesPerWaveform.at(i) + dcc.samplesPreTrigger;
        dcc.ostream.write((const char *) &nSamples, sizeof(int16_t));
    }

    dcc.ostream.write((const char *) &dcc.samplesPreTrigger, sizeof(int16_t));

    dcc.ostream.write((const char *) &dcc.numWaveforms, sizeof(int32_t));

    dcc.ostream.write((const char *) &dcc.unit->modelString, sizeof(int8_t) * 8);

    dcc.ostream.write((const char *) 0, sizeof(int8_t) * 16);

    return;
}

void writeDataOut(dataCollectionConfig &dcc)
{
    uint16_t maxSamples = *max_element( dcc.chPostSamplesPerWaveform.begin(), 
                                        dcc.chPostSamplesPerWaveform.end());
    
    for (int i = 0; i < dcc.activeChannels.count(); i++)
    {
        uint64_t bufferSize = (dcc.chPostSamplesPerWaveform.at(i) + dcc.samplesPreTrigger) 
                                * dcc.numWaveforms * sizeof(int16_t);
        for (int j = 0; j < dcc.numWaveforms; j++)
        {
            dcc.ostream.write((const char*)dcc.dataBuffers.at(i).at(j), bufferSize);
        }
    }
}

// to be run from python side
void runDAQ(char *outputFile,
            int16_t *chATrigger, uint16_t *chAVRange, uint16_t *chAWfSamples,
            int16_t *chBTrigger, uint16_t *chBVRange, uint16_t *chBWfSamples,
            int16_t *chCTrigger, uint16_t *chCVRange, uint16_t *chCWfSamples,
            int16_t *chDTrigger, uint16_t *chDVRange, uint16_t *chDWfSamples,
            int16_t *auxTrigger, uint8_t *timebase,
            uint32_t *numWaveforms, uint16_t *samplesPreTrigger, 
            char *serial = (char*) "")
{
    UNIT *unit;
    if (serial == "") {serial = NULL;}
    pp(serial);
    findUnit(unit, (int8_t*) serial);
    try
    {
        dataCollectionConfig dcc(unit);
        setActiveChannels(dcc, chAVRange, chBVRange, chCVRange, chDVRange);
        setTriggerConfig(dcc, chATrigger, chBTrigger, chCTrigger, chDTrigger, auxTrigger);
        setDataConfig(dcc, timebase, numWaveforms, samplesPreTrigger, 
                    chAWfSamples, chBWfSamples, chCWfSamples, chDWfSamples);

        setDataOutput(dcc, outputFile);
        collectRapidBlockData(dcc);
        writeDataHeader(dcc);
        writeDataOut(dcc);
        CloseDevice(unit);
    }
    catch (exception e)
    {
        CloseDevice(unit);
    }

    return;
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

    m.def("runDAQ", &runDAQ, py::arg("outputFile"), 
    py::arg("chATrigger"), py::arg("chAVRange"), py::arg("chAWfSamples"),
    py::arg("chBTrigger"), py::arg("chBVRange"), py::arg("chBWfSamples"),
    py::arg("chCTrigger"), py::arg("chCVRange"), py::arg("chCWfSamples"),
    py::arg("chDTrigger"), py::arg("chDVRange"), py::arg("chDWfSamples"), 
    py::arg("auxTrigger"), py::arg("timebase"), py::arg("numWaveforms"), 
    py::arg("samplesPreTrigger"), py::arg("serials") = "");
    m.def("getSerials", &getSerials, py::return_value_policy::copy);
}

